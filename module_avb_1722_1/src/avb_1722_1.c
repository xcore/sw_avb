#include "xccompat.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_1722_1.h"
#include "avb_1722_1_protocol.h"
#include "avb_api.h"
#include "ethernet_tx_client.h"
#include "nettypes.h"
#include "misc_timer.h"
#include "simple_printf.h"
#include <print.h>

typedef union {
	unsigned long long l;
	char c[8];
} guid_t;

typedef union {
	unsigned long long l;
	char c[8];
} stream_t;

static unsigned char my_mac_addr[6];

static unsigned char avb_1722_1_sdp_dest_addr[6] =  {0x01, 0x50, 0x43, 0xff, 0x00, 0x00};
static unsigned char avb_1722_1_scm_dest_addr[6] =  {0x01, 0x50, 0x43, 0xff, 0x00, 0x00};
//static unsigned char avb_1722_1_scm_dest_addr[6] =  {0x91, 0xe0, 0xf0, 0x00, 0xff, 0x01};
//static unsigned char avb_1722_1_sec_dest_addr[6] =  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

//! Buffer for constructing 1722.1 transmit packets
static unsigned int avb_1722_1_buf[(sizeof(avb_1722_1_packet_t)+sizeof(ethernet_hdr_t)+3)/4];

//! Timers for various parts of the state machines
static avb_timer sdp_advertise_timer;
static avb_timer sdp_readvertise_timer;
static avb_timer sdp_discovery_timer;

//! Counts two second intervals
static unsigned sdp_two_second_counter = 0;

//! The GUID of this device
static guid_t my_guid;

//! The GUID whose information we are currently trying to discover
static guid_t discover_guid;

//! Enumerations for state variables
enum { SDP_ADVERTISE_IDLE,
	   SDP_ADVERTISE_ADVERTISE_1,
	   SDP_ADVERTISE_ADVERTISE_2,
	   SDP_ADVERTISE_WAITING,
	   SDP_ADVERTISE_DEPARTING_1,
	   SDP_ADVERTISE_DEPARTING_2
} sdp_advertise_state = SDP_ADVERTISE_IDLE;

enum { SDP_DISCOVERY_IDLE,
	   SDP_DISCOVERY_WAITING,
	   SDP_DISCOVERY_DISCOVER,
	   SDP_DISCOVERY_ADDED,
	   SDP_DISCOVERY_REMOVED
} sdp_discovery_state = SDP_DISCOVERY_IDLE;

enum { SCM_TALKER_IDLE,
	   SCM_TALKER_WAITING,
	   SCM_TALKER_CONNECT,
	   SCM_TALKER_DISCONNECT,
	   SCM_TALKER_GET_STATE,
	   SCM_TALKER_GET_CONNECTION,
	   SCM_TALKER_WAITING_FOR_CONNECT,
	   SCM_TALKER_WAITING_FOR_DISCONNECT
} scm_talker_state = SCM_TALKER_IDLE;

enum { SCM_LISTENER_IDLE,
	   SCM_LISTENER_WAITING,
	   SCM_LISTENER_CONNECT_RX_COMMAND,
	   SCM_LISTENER_DISCONNECT_RX_COMMAND,
	   SCM_LISTENER_CONNECT_TX_RESPONSE,
	   SCM_LISTENER_DISCONNECT_TX_RESPONSE,
	   SCM_LISTENER_GET_STATE,
	   SCM_LISTENER_WAITING_FOR_CONNECT,
	   SCM_LISTENER_WAITING_FOR_DISCONNECT
} scm_listener_state = SCM_LISTENER_IDLE;

//! Record for entries in the SRP entity database
typedef struct {
	guid_t guid;
	unsigned timeout;
} avb_1722_1_entity_record;

//! Data structures from the 1722.1 SCM section

typedef struct {
	stream_t stream_id;
	guid_t controller_guid;
	guid_t listener_guid;
	guid_t talker_guid;
	short talker_unique_id;
	short listener_unique_id;
	short connection_count;
	short sequence_id;
	short flags;
	char stream_dest_mac[6];
	char message_type;
	char status;
} avb_1722_1_scm_rcvd_cmd_resp;

typedef struct {
	guid_t guid;
	short unique_id;
	short padding;
} avb_1722_1_scm_listener_pair;

typedef struct {
    stream_t stream_id;
	short connection_count;
	char destination_mac[6];
	avb_1722_1_scm_listener_pair listeners[AVB_1722_1_MAX_LISTENERS_PER_TALKER];
} avb_1722_1_scm_talker_stream_info;

typedef struct {
	guid_t talker_guid;
	short talker_unique_id;
	short connected;
	stream_t stream_id;
	char destination_mac[6];
	short padding;
} avb_1722_1_scm_listener_stream_info;

typedef struct {
	unsigned timeout;
	short retried;
	short command;
	short original_sequence_id;
	short dummy;
} avb_1722_1_scm_inflight_command;


//! Entity database
avb_1722_1_entity_record entities[AVB_1722_1_MAX_ENTITIES];

//! Listener stream database
avb_1722_1_scm_listener_stream_info scm_listener_streams[AVB_1722_1_MAX_LISTENERS];

//! Talker stream database
avb_1722_1_scm_talker_stream_info scm_talker_streams[AVB_1722_1_MAX_TALKERS];

//! Listener in-flight command database
avb_1722_1_scm_inflight_command scm_inflight_commands[AVB_1722_1_MAX_INFLIGHT_COMMANDS];

//! Talker's rcvdCmdResp
avb_1722_1_scm_rcvd_cmd_resp scm_talker_rcvd_cmd_resp;

//! Listener's rcvdCmdResp
avb_1722_1_scm_rcvd_cmd_resp scm_listener_rcvd_cmd_resp;


void print_guid()
{
	simple_printf("1722.1: GUID=%x.%x.%x.%x.%x.%x.%x.%x\n",
			my_guid.c[0],
			my_guid.c[1],
			my_guid.c[2],
			my_guid.c[3],
			my_guid.c[4],
			my_guid.c[5],
			my_guid.c[6],
			my_guid.c[7]);
}

void avb_1722_1_init(unsigned char macaddr[6], unsigned char serial_number[2])
{
  for (int i=0;i<6;i++)
    my_mac_addr[i] = macaddr[i];

	my_guid.c[0] = serial_number[1];
	my_guid.c[1] = serial_number[0];
	my_guid.c[2] = macaddr[5];
	my_guid.c[3] = macaddr[4];
	my_guid.c[4] = macaddr[3];
	my_guid.c[5] = macaddr[2];
	my_guid.c[6] = macaddr[1];
	my_guid.c[7] = macaddr[0];

	print_guid(my_guid);

	init_avb_timer(&sdp_advertise_timer, 1);
	init_avb_timer(&sdp_readvertise_timer, 100);
	init_avb_timer(&sdp_discovery_timer, 200);

	sdp_discovery_state = SDP_DISCOVERY_WAITING;
	start_avb_timer(&sdp_discovery_timer, 1);

	scm_talker_state = SCM_TALKER_WAITING;
	scm_listener_state = SCM_LISTENER_WAITING;
}



static void avb_1722_1_entity_database_add(avb_1722_1_sdp_packet_t* pkt)
{
	guid_t guid;
	unsigned int found_slot_index = AVB_1722_1_MAX_ENTITIES;

	GET_LONG_WORD(guid, pkt->entity_guid);

	for (unsigned i=0; i<AVB_1722_1_MAX_ENTITIES; ++i) {
		if (entities[i].guid.l==0) found_slot_index=i;
		if (entities[i].guid.l==guid.l) {
			found_slot_index=i;
			break;
		}
	}

	if (found_slot_index != AVB_1722_1_MAX_ENTITIES) {
		entities[found_slot_index].guid.l = guid.l;
		entities[found_slot_index].timeout = GET_1722_1_VALID_TIME(&pkt->header) + sdp_two_second_counter;
		return;
	}
}

static void avb_1722_1_entity_database_remove(avb_1722_1_sdp_packet_t* pkt)
{
	guid_t guid;
	GET_LONG_WORD(guid, pkt->entity_guid);

	for (unsigned i=0; i<AVB_1722_1_MAX_ENTITIES; ++i) {
		if (entities[i].guid.l==guid.l) {
			entities[i].guid.l=0;
		}
	}
}

static unsigned avb_1722_1_entity_database_check_timeout()
{
	for (unsigned i=0; i<AVB_1722_1_MAX_ENTITIES; ++i) {
		if (entities[i].guid.l==0) continue;

		if (entities[i].timeout < sdp_two_second_counter) {
			entities[i].guid.l=0;
			return 1;
		}
	}
	return 0;
}


//----------------------------------------------------------------------------------------


static unsigned compare_guid(char* a, guid_t* b)
{
	return (a[0]==b->c[7] &&
			a[1]==b->c[6] &&
			a[2]==b->c[5] &&
			a[3]==b->c[4] &&
			a[4]==b->c[3] &&
			a[5]==b->c[2] &&
			a[6]==b->c[1] &&
			a[7]==b->c[0]);
}

static void avb_1722_1_create_1722_1_header(unsigned char* dest_addr, int subtype, int message_type, char valid_time_status, unsigned data_len, ethernet_hdr_t *hdr)
{
	avb_1722_1_packet_header_t *pkt = (avb_1722_1_packet_header_t *) (hdr + 1);

	for (int i=0;i<6;i++) {
		hdr->dest_addr[i] = dest_addr[i];
		hdr->src_addr[i] = my_mac_addr[i];
	}

	hdr->ethertype[0] = AVB_ETYPE >> 8;
	hdr->ethertype[1] = AVB_ETYPE & 0xff;

	SET_1722_1_CD_FLAG(pkt, 1);
	SET_1722_1_SUBTYPE(pkt, subtype);
	SET_1722_1_SV(pkt, 0);
	SET_1722_1_AVB_VERSION(pkt, 0);
	SET_1722_1_MSG_TYPE(pkt, message_type);
	SET_1722_1_VALID_TIME(pkt, valid_time_status);
	SET_1722_1_DATALENGTH(pkt, data_len);
}

//----------------------------------------------------------------------------------------

/*
static void avb_1722_1_create_sec_packet(int message_type)
{
	  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  struct avb_1722_1_sec_packet_t *pkt = (avb_1722_1_sec_packet_t*) (hdr + 1);

	  avb_1722_1_create_1722_1_header(avb_1722_1_sec_dest_addr, DEFAULT_1722_1_SEC_SUBTYPE, message_type, 0, 40, hdr);
}
*/

static avb_status_t process_avb_1722_1_sec_packet(avb_1722_1_sec_packet_t* pkt)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));
	switch (message_type) {
	}
	return AVB_1722_1_OK;
}

//----------------------------------------------------------------------------------------

static unsigned scm_listener_valid_listener_unique()
{
	return scm_listener_rcvd_cmd_resp.listener_unique_id < AVB_1722_1_MAX_LISTENERS;
}

static unsigned scm_listener_listener_is_connected()
{
	enum avb_sink_state_t state;
	get_avb_sink_state(scm_listener_rcvd_cmd_resp.listener_unique_id, &state);
	return state != AVB_SINK_STATE_DISABLED;
}

static avb_1722_1_scm_status_type scm_listener_get_state()
{
	return SCM_STATUS_SUCCESS;
}

static unsigned scm_talker_valid_talker_unique()
{
	return scm_talker_rcvd_cmd_resp.listener_unique_id < AVB_1722_1_MAX_TALKERS;
}

static avb_1722_1_scm_status_type scm_talker_get_state()
{
	return SCM_STATUS_SUCCESS;
}

static avb_1722_1_scm_status_type scm_talker_get_connection()
{
	return SCM_STATUS_SUCCESS;
}

static void avb_1722_1_create_scm_packet(avb_1722_1_scm_rcvd_cmd_resp* rcr)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_scm_packet_t *pkt = (avb_1722_1_scm_packet_t*) (hdr + 1);

	avb_1722_1_create_1722_1_header(avb_1722_1_scm_dest_addr, DEFAULT_1722_1_SCM_SUBTYPE, rcr->message_type, rcr->status, 40, hdr);

	SET_LONG_WORD(pkt->stream_id, rcr->stream_id);
	SET_LONG_WORD(pkt->controller_guid, rcr->controller_guid);
	SET_LONG_WORD(pkt->listener_guid, rcr->listener_guid);
	SET_LONG_WORD(pkt->talker_guid, rcr->talker_guid);
	pkt->talker_unique_id = rcr->talker_unique_id;
	pkt->listener_unique_id = rcr->listener_unique_id;
	pkt->connection_count = rcr->connection_count;
	pkt->sequence_id = rcr->sequence_id;
	pkt->flags = rcr->flags;
	pkt->dest_mac[0] = rcr->stream_dest_mac[0];
	pkt->dest_mac[1] = rcr->stream_dest_mac[1];
	pkt->dest_mac[2] = rcr->stream_dest_mac[2];
	pkt->dest_mac[3] = rcr->stream_dest_mac[3];
	pkt->dest_mac[4] = rcr->stream_dest_mac[4];
	pkt->dest_mac[5] = rcr->stream_dest_mac[5];

}

static void scm_listener_tx_command(unsigned message_type, unsigned timeout_ms, chanend c_tx)
{
	scm_listener_rcvd_cmd_resp.message_type = message_type;
	scm_listener_rcvd_cmd_resp.status = SCM_STATUS_SUCCESS;
	avb_1722_1_create_scm_packet(&scm_listener_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, 66, ETH_BROADCAST);

	// set up inflight command
}

static void scm_listener_tx_response(unsigned type, unsigned error_code, chanend c_tx)
{
	scm_listener_rcvd_cmd_resp.message_type = type;
	scm_listener_rcvd_cmd_resp.status = error_code;
	avb_1722_1_create_scm_packet(&scm_listener_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, 66, ETH_BROADCAST);
}

static void scm_talker_tx_response(unsigned type, unsigned error_code, chanend c_tx)
{
	unsigned talker = scm_talker_rcvd_cmd_resp.talker_unique_id;
	scm_talker_rcvd_cmd_resp.message_type = type;
	scm_talker_rcvd_cmd_resp.status = error_code;

	scm_talker_rcvd_cmd_resp.stream_id = scm_talker_streams[talker].stream_id;
	for (unsigned c=0; c<6; ++c)
		scm_talker_rcvd_cmd_resp.stream_dest_mac[c] = scm_talker_streams[talker].destination_mac[c];

	avb_1722_1_create_scm_packet(&scm_talker_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, 66, ETH_BROADCAST);
}

static void scm_listener_remove_inflight()
{
	for (unsigned int i=0; i<AVB_1722_1_MAX_INFLIGHT_COMMANDS; ++i) {
		//inflight_commands[i];
	}
}

static void scm_listener_check_inflight_command_timeouts()
{
	for (unsigned int i=0; i<AVB_1722_1_MAX_INFLIGHT_COMMANDS; ++i) {
		//inflight_commands[i];
	}
}

static void store_rcvd_cmd_resp(avb_1722_1_scm_rcvd_cmd_resp* store, avb_1722_1_scm_packet_t* pkt)
{
	GET_LONG_WORD(store->stream_id, pkt->stream_id);
	GET_LONG_WORD(store->controller_guid, pkt->controller_guid);
	GET_LONG_WORD(store->listener_guid, pkt->listener_guid);
	GET_LONG_WORD(store->talker_guid, pkt->talker_guid);
	store->talker_unique_id = pkt->talker_unique_id;
	store->listener_unique_id = pkt->listener_unique_id;
	store->connection_count = pkt->connection_count;
	store->sequence_id = pkt->sequence_id;
	store->flags = pkt->flags;
	store->stream_dest_mac[0] = pkt->dest_mac[0];
	store->stream_dest_mac[1] = pkt->dest_mac[1];
	store->stream_dest_mac[2] = pkt->dest_mac[2];
	store->stream_dest_mac[3] = pkt->dest_mac[3];
	store->stream_dest_mac[4] = pkt->dest_mac[4];
	store->stream_dest_mac[5] = pkt->dest_mac[5];
	store->message_type = GET_1722_1_MSG_TYPE(&(pkt->header));
	store->status = GET_1722_1_VALID_TIME(&(pkt->header));
}

static avb_status_t process_avb_1722_1_scm_talker_packet(unsigned message_type, avb_1722_1_scm_packet_t* pkt)
{
	if (compare_guid(pkt->talker_guid, &my_guid)==0) return AVB_1722_1_OK;
	if (scm_talker_state!=SCM_TALKER_WAITING) { return AVB_1722_1_OK; }

	store_rcvd_cmd_resp(&scm_talker_rcvd_cmd_resp, pkt);

	switch (message_type)
	{
	case SCM_CMD_CONNECT_TX_COMMAND:
		scm_talker_state = SCM_TALKER_CONNECT;
		break;
	case SCM_CMD_DISCONNECT_TX_COMMAND:
		scm_talker_state = SCM_TALKER_DISCONNECT;
		break;
	case SCM_CMD_GET_TX_STATE_COMMAND:
		scm_talker_state = SCM_TALKER_GET_STATE;
		break;
	case SCM_CMD_GET_TX_CONNECTION_COMMAND:
		scm_talker_state = SCM_TALKER_GET_CONNECTION;
		break;
	}

	return AVB_1722_1_OK;
}

static avb_status_t process_avb_1722_1_scm_listener_packet(unsigned message_type, avb_1722_1_scm_packet_t* pkt)
{
	if (compare_guid(pkt->listener_guid, &my_guid)==0) return AVB_1722_1_OK;
	if (scm_talker_state!=SCM_LISTENER_WAITING) { return AVB_1722_1_OK; }

	store_rcvd_cmd_resp(&scm_listener_rcvd_cmd_resp, pkt);

	switch (message_type)
	{
	case SCM_CMD_CONNECT_TX_RESPONSE:
		scm_listener_state = SCM_LISTENER_CONNECT_TX_RESPONSE;
		break;
	case SCM_CMD_DISCONNECT_TX_RESPONSE:
		scm_listener_state = SCM_LISTENER_DISCONNECT_TX_RESPONSE;
		break;
	case SCM_CMD_CONNECT_RX_COMMAND:
		scm_listener_state = SCM_LISTENER_CONNECT_RX_COMMAND;
		break;
	case SCM_CMD_DISCONNECT_RX_COMMAND:
		scm_listener_state = SCM_LISTENER_DISCONNECT_RX_COMMAND;
		break;
	case SCM_CMD_GET_RX_STATE_COMMAND:
		scm_listener_state = SCM_LISTENER_GET_STATE;
		break;
	}

	return AVB_1722_1_OK;
}

static avb_status_t process_avb_1722_1_scm_packet(avb_1722_1_scm_packet_t* pkt)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

	switch (message_type) {

	// Talker messages
	case SCM_CMD_CONNECT_TX_COMMAND:
	case SCM_CMD_DISCONNECT_TX_COMMAND:
	case SCM_CMD_GET_TX_STATE_COMMAND:
	case SCM_CMD_GET_TX_CONNECTION_COMMAND:
		return process_avb_1722_1_scm_talker_packet(message_type, pkt);

	// Listener messages
	case SCM_CMD_CONNECT_TX_RESPONSE:
	case SCM_CMD_DISCONNECT_TX_RESPONSE:
	case SCM_CMD_CONNECT_RX_COMMAND:
	case SCM_CMD_DISCONNECT_RX_COMMAND:
	case SCM_CMD_GET_RX_STATE_COMMAND:
		return process_avb_1722_1_scm_listener_packet(message_type, pkt);
	}

	return AVB_1722_1_OK;
}

static avb_status_t avb_1722_1_scm_talker_periodic(chanend c_tx)
{
	switch (scm_talker_state) {
	case SCM_TALKER_IDLE:
	case SCM_TALKER_WAITING:
	case SCM_TALKER_WAITING_FOR_CONNECT:
	case SCM_TALKER_WAITING_FOR_DISCONNECT:
		return AVB_NO_STATUS;

	case SCM_TALKER_CONNECT:
		if (!scm_talker_valid_talker_unique(1)) {
			scm_talker_tx_response(SCM_CMD_CONNECT_TX_RESPONSE, SCM_STATUS_TALKER_UNKNOWN_ID, c_tx);
			scm_talker_state = SCM_TALKER_WAITING;
			return AVB_NO_STATUS;
		} else {
			scm_talker_state = SCM_TALKER_WAITING_FOR_CONNECT;
			return AVB_1722_1_CONNECT_TALKER;
		}
		break;
	case SCM_TALKER_DISCONNECT:
		if (!scm_talker_valid_talker_unique(1)) {
			scm_talker_tx_response(SCM_CMD_DISCONNECT_TX_RESPONSE, SCM_STATUS_TALKER_UNKNOWN_ID, c_tx);
			scm_talker_state = SCM_TALKER_WAITING;
			return AVB_NO_STATUS;
		} else {
			scm_talker_state = SCM_TALKER_WAITING_FOR_DISCONNECT;
			return AVB_1722_1_DISCONNECT_TALKER;
		}
		break;
	case SCM_TALKER_GET_STATE:
		if (!scm_talker_valid_talker_unique(1)) {
			scm_talker_tx_response(SCM_CMD_GET_TX_STATE_RESPONSE, SCM_STATUS_TALKER_UNKNOWN_ID, c_tx);
		} else {
			scm_talker_tx_response(SCM_CMD_GET_TX_STATE_RESPONSE, scm_talker_get_state(), c_tx);
		}
		scm_talker_state = SCM_TALKER_WAITING;
		return AVB_NO_STATUS;
	case SCM_TALKER_GET_CONNECTION:
		if (!scm_talker_valid_talker_unique(1)) {
			scm_talker_tx_response(SCM_CMD_GET_TX_CONNECTION_RESPONSE, SCM_STATUS_TALKER_UNKNOWN_ID, c_tx);
		} else {
			scm_talker_tx_response(SCM_CMD_GET_TX_CONNECTION_RESPONSE, scm_talker_get_connection(), c_tx);
		}
		scm_talker_state = SCM_TALKER_WAITING;
		return AVB_NO_STATUS;
	}

	return AVB_NO_STATUS;
}

static avb_status_t avb_1722_1_scm_listener_periodic(chanend c_tx)
{
	switch (scm_listener_state) {
	case SCM_LISTENER_IDLE:
	case SCM_LISTENER_WAITING_FOR_CONNECT:
	case SCM_LISTENER_WAITING_FOR_DISCONNECT:
		break;
	case SCM_LISTENER_WAITING:
		scm_listener_check_inflight_command_timeouts();
		break;
	case SCM_LISTENER_CONNECT_RX_COMMAND:
		if (!scm_listener_valid_listener_unique()) {
			scm_listener_tx_response(SCM_CMD_CONNECT_RX_RESPONSE, SCM_STATUS_LISTENER_UNKNOWN_ID, c_tx);
		} else {
			if (!scm_listener_listener_is_connected()) {
				scm_listener_tx_command(SCM_CMD_CONNECT_TX_COMMAND, 2000, c_tx);
			} else {
				scm_listener_tx_response(SCM_CMD_CONNECT_RX_RESPONSE, SCM_STATUS_LISTENER_EXCLUSIVE, c_tx);
			}
		}
		scm_listener_state = SCM_LISTENER_WAITING;
		break;
	case SCM_LISTENER_DISCONNECT_RX_COMMAND:
		if (!scm_listener_valid_listener_unique()) {
			scm_listener_tx_response(SCM_CMD_DISCONNECT_RX_RESPONSE, SCM_STATUS_LISTENER_UNKNOWN_ID, c_tx);
		} else {
			if (scm_listener_listener_is_connected()) {
				scm_listener_tx_command(SCM_CMD_DISCONNECT_TX_COMMAND, 200, c_tx);
			} else {
				scm_listener_tx_response(SCM_CMD_DISCONNECT_RX_RESPONSE, SCM_STATUS_NOT_CONNECTED, c_tx);
			}
		}
		scm_listener_state = SCM_LISTENER_WAITING;
		break;
	case SCM_LISTENER_CONNECT_TX_RESPONSE:
		if (scm_listener_valid_listener_unique()) {
			scm_listener_tx_response(SCM_CMD_CONNECT_RX_RESPONSE, 0, c_tx);
			scm_listener_remove_inflight();
			scm_listener_state = SCM_LISTENER_WAITING_FOR_CONNECT;
			return AVB_1722_1_CONNECT_LISTENER;
		}
		break;
	case SCM_LISTENER_DISCONNECT_TX_RESPONSE:
		if (scm_listener_valid_listener_unique()) {
			scm_listener_tx_response(SCM_CMD_DISCONNECT_RX_RESPONSE, 0, c_tx);
			scm_listener_remove_inflight();
			scm_listener_state = SCM_LISTENER_WAITING_FOR_DISCONNECT;
			return AVB_1722_1_DISCONNECT_LISTENER;
		}
		break;
	case SCM_LISTENER_GET_STATE:
		if (!scm_listener_valid_listener_unique()) {
			scm_listener_tx_response(SCM_CMD_GET_RX_STATE_RESPONSE, SCM_STATUS_LISTENER_UNKNOWN_ID, c_tx);
		} else {
			scm_listener_tx_response(SCM_CMD_GET_RX_STATE_RESPONSE, scm_listener_get_state(), c_tx);
		}
		scm_listener_state = SCM_LISTENER_WAITING;
		break;
	}

	return AVB_NO_STATUS;
}

unsigned avb_1722_1_scm_get_talker_connection_info(short *talker)
{
	*talker = scm_talker_rcvd_cmd_resp.talker_unique_id;
	return 1;
}

unsigned avb_1722_1_scm_get_listener_connection_info(short *listener, char address[6], unsigned streamId[2], unsigned* vlan)
{
	*listener = scm_listener_rcvd_cmd_resp.listener_unique_id;
	for (unsigned c=0; c<6; ++c) address[c] = scm_listener_rcvd_cmd_resp.stream_dest_mac[c];
	streamId[0] = (unsigned)(scm_listener_rcvd_cmd_resp.stream_id.l >> 0);
	streamId[1] = (unsigned)(scm_listener_rcvd_cmd_resp.stream_id.l >> 32);
	*vlan = 2;
	return 1;
}

void avb_1722_1_scm_talker_connection_complete(short code, chanend c_tx)
{
	switch (scm_talker_state)
	{
	case SCM_TALKER_WAITING_FOR_CONNECT:
		scm_talker_tx_response(SCM_CMD_CONNECT_TX_RESPONSE, code, c_tx);
		scm_talker_state = SCM_TALKER_WAITING;
		break;
	case SCM_TALKER_WAITING_FOR_DISCONNECT:
		scm_talker_tx_response(SCM_CMD_DISCONNECT_TX_RESPONSE, code, c_tx);
		scm_talker_state = SCM_TALKER_WAITING;
		break;
	default:
		break;
	}
}


void avb_1722_1_scm_listener_connection_complete(short code, chanend c_tx)
{
	switch (scm_listener_state)
	{
	case SCM_LISTENER_WAITING_FOR_CONNECT:
		scm_listener_rcvd_cmd_resp.status = code;
		scm_listener_state = SCM_LISTENER_WAITING;
		break;
	case SCM_LISTENER_WAITING_FOR_DISCONNECT:
		scm_listener_rcvd_cmd_resp.status = code;
		scm_listener_state = SCM_LISTENER_WAITING;
		break;
	default:
		break;
	}
}

void avb_1722_1_talker_set_mac_address(unsigned talker_unique_id, char macaddr[])
{
	if (talker_unique_id < AVB_1722_1_MAX_TALKERS)
	{
		for (unsigned i=0; i<6; ++i)
			scm_talker_streams[talker_unique_id].destination_mac[i] = macaddr[i];
	}
}

void avb_1722_1_talker_set_stream_id(unsigned talker_unique_id, unsigned streamId[2])
{
	if (talker_unique_id < AVB_1722_1_MAX_TALKERS)
	{
		scm_talker_streams[talker_unique_id].stream_id.l = (unsigned long long)streamId[0] + (((unsigned long long)streamId[1]) << 32);
	}
}


//----------------------------------------------------------------------------------------

avb_status_t process_avb_1722_1_sdp_packet(avb_1722_1_sdp_packet_t* pkt)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));
	guid_t zero_guid = { 0 };

	switch (message_type) {
	case ENTITY_DISCOVER: {
		if ( compare_guid(pkt->entity_guid, &my_guid) || compare_guid(pkt->entity_guid, &zero_guid) )
		{
			if (sdp_advertise_state == SDP_ADVERTISE_WAITING)
				sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_1;
		}
		return AVB_1722_1_OK;
	}

	case ENTITY_AVAILABLE: {
		avb_1722_1_entity_database_add(pkt);
		sdp_discovery_state = SDP_DISCOVERY_ADDED;
		return AVB_1722_1_OK;
	}

	case ENTITY_DEPARTING: {
		avb_1722_1_entity_database_remove(pkt);
		sdp_discovery_state = SDP_DISCOVERY_REMOVED;
		return AVB_1722_1_OK;
	}

	}

	return AVB_1722_1_OK;
}

static void avb_1722_1_create_sdp_packet(int message_type, guid_t guid)
{
	  ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  avb_1722_1_sdp_packet_t *pkt = (avb_1722_1_sdp_packet_t*) (hdr + 1);

	  avb_1722_1_create_1722_1_header(avb_1722_1_sdp_dest_addr, DEFAULT_1722_1_SDP_SUBTYPE, message_type,
			  (message_type==ENTITY_AVAILABLE)?AVB_1722_1_SDP_VALID_TIME:0, 40, hdr);

	  SET_LONG_WORD(pkt->entity_guid, guid);

	  if (message_type==ENTITY_DISCOVER) {
		  pkt->vendor_id[0] = 0;
		  pkt->vendor_id[1] = 0;
		  pkt->model_id[0] = 0;
		  pkt->model_id[1] = 0;
		  pkt->entity_capabilities[0] = 0;
		  pkt->entity_capabilities[1] = 0;
		  pkt->talker_stream_sources = 0;
		  pkt->talker_capabilities = 0;
		  pkt->listener_stream_sinks = 0;
		  pkt->listener_capabilites = 0;
		  pkt->controller_capabilities[0] = 0;
		  pkt->controller_capabilities[1] = 0;
		  pkt->boot_id[0] = 0;
		  pkt->boot_id[1] = 0;
	  } else {
		  SET_WORD_CONST(pkt->vendor_id, AVB_1722_1_SDP_VENDOR_ID);
		  SET_WORD_CONST(pkt->model_id, AVB_1722_1_SDP_MODEL_ID);
		  SET_WORD_CONST(pkt->entity_capabilities, AVB_1722_1_SDP_ENTITY_CAPABILITIES);
		  pkt->talker_stream_sources = AVB_1722_1_SDP_TALKER_STREAM_SOURCES;
		  pkt->talker_capabilities = AVB_1722_1_SDP_TALKER_CAPABILITIES;
		  pkt->listener_stream_sinks = AVB_1722_1_SDP_LISTENER_STREAM_SINKS;
		  pkt->listener_capabilites = AVB_1722_1_SDP_LISTENER_CAPABILITIES;
		  SET_WORD_CONST(pkt->controller_capabilities, AVB_1722_1_SDP_CONTROLLER_CAPABILITIES);
		  SET_WORD_CONST(pkt->boot_id, AVB_1722_1_SDP_BOOT_ID);
	  }
	  pkt->reserved[0] = 0;
	  pkt->reserved[1] = 0;
	  pkt->reserved[2] = 0;
	  pkt->reserved[3] = 0;
	  pkt->reserved[4] = 0;
	  pkt->reserved[5] = 0;
}

static avb_status_t avb_1722_1_sdp_discovery_periodic(chanend c_tx)
{
	switch (sdp_discovery_state) {
	case SDP_DISCOVERY_IDLE:
		break;

	case SDP_DISCOVERY_WAITING: {
			unsigned lost=0;
			if (avb_timer_expired(&sdp_discovery_timer)) {
				sdp_two_second_counter++;
				lost = avb_1722_1_entity_database_check_timeout();
				start_avb_timer(&sdp_discovery_timer, 1);
			}
			return (lost > 0) ? AVB_1722_1_ENTITY_REMOVED : AVB_NO_STATUS;
		}

	case SDP_DISCOVERY_DISCOVER: {
			avb_1722_1_create_sdp_packet(ENTITY_DISCOVER, discover_guid);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			sdp_discovery_state = SDP_DISCOVERY_WAITING;
		}
		break;

	case SDP_DISCOVERY_ADDED:
		sdp_discovery_state = SDP_DISCOVERY_WAITING;
		return AVB_1722_1_ENTITY_ADDED;

	case SDP_DISCOVERY_REMOVED:
		sdp_discovery_state = SDP_DISCOVERY_WAITING;
		return AVB_1722_1_ENTITY_REMOVED;
	}

	return AVB_NO_STATUS;
}

static avb_status_t avb_1722_1_sdp_advertising_periodic(chanend c_tx)
{
	switch (sdp_advertise_state) {
	case SDP_ADVERTISE_IDLE:
		break;

	case SDP_ADVERTISE_ADVERTISE_1:
		avb_1722_1_create_sdp_packet(ENTITY_AVAILABLE, my_guid);
		mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
		start_avb_timer(&sdp_advertise_timer, 3); // 3 centiseconds
		sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_2;
		break;

	case SDP_ADVERTISE_ADVERTISE_2:
		if (avb_timer_expired(&sdp_advertise_timer)) {
			avb_1722_1_create_sdp_packet(ENTITY_AVAILABLE, my_guid);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			start_avb_timer(&sdp_readvertise_timer, AVB_1722_1_SDP_VALID_TIME);
			sdp_advertise_state = SDP_ADVERTISE_WAITING;
		}
		break;

	case SDP_ADVERTISE_WAITING:
		if (avb_timer_expired(&sdp_readvertise_timer)) {
			sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_1;
		}
		break;

	case SDP_ADVERTISE_DEPARTING_1:
		avb_1722_1_create_sdp_packet(ENTITY_DEPARTING, my_guid);
		mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
		start_avb_timer(&sdp_advertise_timer, 3); // 3 centiseconds
		sdp_advertise_state = SDP_ADVERTISE_DEPARTING_2;
		break;

	case SDP_ADVERTISE_DEPARTING_2:
		if (avb_timer_expired(&sdp_advertise_timer)) {
			avb_1722_1_create_sdp_packet(ENTITY_DEPARTING, my_guid);
			mac_tx(c_tx, avb_1722_1_buf, 60, ETH_BROADCAST);
			sdp_advertise_state = SDP_ADVERTISE_IDLE;
		}
		break;

	default:
		break;
	}

	return AVB_NO_STATUS;
}

void avb_1722_1_sdp_announce()
{
	if (sdp_advertise_state == SDP_ADVERTISE_IDLE) sdp_advertise_state = SDP_ADVERTISE_ADVERTISE_1;
}


void avb_1722_1_sdp_depart()
{
	if (sdp_advertise_state == SDP_ADVERTISE_IDLE) sdp_advertise_state = SDP_ADVERTISE_DEPARTING_1;
}

void avb_1722_1_sdp_discover(unsigned guid[])
{
	if (sdp_discovery_state == SDP_DISCOVERY_WAITING) {
		sdp_discovery_state = SDP_DISCOVERY_DISCOVER;
		discover_guid.l = (unsigned long long)guid[0] + ((unsigned long long)guid[1] << 32);
	}
}

void avb_1722_1_sdp_discover_all()
{
	unsigned guid[2] = {0,0};
	avb_1722_1_sdp_discover(guid);
}

//----------------------------------------------------------------------------------------

avb_status_t avb_1722_1_process_packet(unsigned int buf0[], int len, chanend c_tx)
{
	  unsigned char *buf = (unsigned char *) buf0;

	  struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf[0];
	  struct tagged_ethernet_hdr_t *tagged_ethernet_hdr = (tagged_ethernet_hdr_t *) &buf[0];

	  int has_qtag = ethernet_hdr->ethertype[1]==0x18;
	  int ethernet_pkt_size = has_qtag ? 18 : 14;

	  struct avb_1722_1_packet_header_t *pkt =
	    (struct avb_1722_1_packet_header_t *) &buf[ethernet_pkt_size];

	  if (has_qtag) {
	    if (tagged_ethernet_hdr->ethertype[1] != (AVB_ETYPE & 0xff) ||
	        tagged_ethernet_hdr->ethertype[0] != (AVB_ETYPE >> 8)) {
	        // not a 1722 packet
	        return AVB_NO_STATUS;
	      }
	  } else {
	    if (ethernet_hdr->ethertype[1] != (AVB_ETYPE & 0xff) ||
	        ethernet_hdr->ethertype[0] != (AVB_ETYPE >> 8)) {
	        // not a 1722 packet
	        return AVB_NO_STATUS;
	      }
	  }

	  if (GET_1722_1_CD_FLAG(pkt) != 1)
		    // not a 1722.1 packet
		    return AVB_NO_STATUS;

	  {
		  unsigned subtype = GET_1722_1_SUBTYPE(pkt);

		  switch (subtype) {
		  case DEFAULT_1722_1_SDP_SUBTYPE:
			  return process_avb_1722_1_sdp_packet((avb_1722_1_sdp_packet_t*)pkt);
		  case DEFAULT_1722_1_SEC_SUBTYPE:
			  return process_avb_1722_1_sec_packet((avb_1722_1_sec_packet_t*)pkt);
		  case DEFAULT_1722_1_SCM_SUBTYPE:
			  return process_avb_1722_1_scm_packet((avb_1722_1_scm_packet_t*)pkt);
		  default:
			  return AVB_NO_STATUS;
		  }
	  }

	  return AVB_NO_STATUS;
}

avb_status_t avb_1722_1_periodic(chanend c_tx)
{
	avb_status_t res;
	res = avb_1722_1_sdp_advertising_periodic(c_tx);
	if (res != AVB_NO_STATUS) return res;
	res = avb_1722_1_sdp_discovery_periodic(c_tx);
	if (res != AVB_NO_STATUS) return res;
	res = avb_1722_1_scm_listener_periodic(c_tx);
	if (res != AVB_NO_STATUS) return res;
	return avb_1722_1_scm_talker_periodic(c_tx);
}



