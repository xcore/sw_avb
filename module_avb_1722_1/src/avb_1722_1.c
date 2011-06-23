#include "xccompat.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_1722_1.h"
#include "avb_1722_1_protocol.h"
#include "avb_api.h"
#include "ethernet_tx_client.h"
#include "nettypes.h"
#include "misc_timer.h"
#include <print.h>
#include <string.h>
#include "gptp.h"

typedef union {
	unsigned long long l;
	char c[8];
} guid_t;

typedef union {
	unsigned long long l;
	char c[8];
} stream_t;

static unsigned char my_mac_addr[6];

static unsigned char avb_1722_1_adp_dest_addr[6]  =  {0x91, 0xe0, 0xf0, 0x00, 0xff, 0x01};
static unsigned char avb_1722_1_acmp_dest_addr[6] =  {0x91, 0xe0, 0xf0, 0x00, 0xff, 0x01};
static unsigned char avb_1722_1_aecp_dest_addr[6] =  {0x91, 0xe0, 0xf0, 0x00, 0xff, 0x01};

//! Buffer for constructing 1722.1 transmit packets
static unsigned int avb_1722_1_buf[(sizeof(avb_1722_1_packet_t)+sizeof(ethernet_hdr_t)+3)/4];

//! Timers for various parts of the state machines
static avb_timer adp_advertise_timer;
static avb_timer adp_readvertise_timer;
static avb_timer adp_discovery_timer;
static avb_timer ptp_monitor_timer;

//! Counts two second intervals
static unsigned adp_two_second_counter = 0;

//! The GUID of this device
static guid_t my_guid;

//! The GUID whose information we are currently trying to discover
static guid_t discover_guid;

//! Thr GUID for the PTP grandmaster server
static guid_t as_grandmaster_id;

// External function for SEC parsing
extern unsigned int avb_1722_1_walk_tree(char* address, unsigned set, char* data);

//! The ADP available index counter
static unsigned long avb_1722_1_available_index = 0;

static unsigned char avb_1722_1_acmp_default_format_channel_counts[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24
};

static unsigned int avb_1722_1_acmp_default_format_frequency[] = {
	44100, 48000, 88200, 96000, 176400, 192000
};

static unsigned char avb_1722_1_aecp_parameter_length[] = {
0, 2, 2, 8, 2, 2, 2, 4, 1, 1, 2, 2, 4, 4, 4, 6, 8, 8, 8, 8, 8,
4, 8, 6, 4, 16, 8, 16, 4, 32, 64, 128, 4, 4, 4, 4, 4, 1, 1, 184,
4, 4, 8, 16, 4, 4, 8, 8, 142, 8, 84, 84, 2, 2, 38, 40, 40, 40,
42, 46, 46, 52, 52, 52, 64, 64, 64, 38, 12
};

//! Enumerations for state variables
enum { ADP_ADVERTISE_IDLE,
	   ADP_ADVERTISE_ADVERTISE_0,
	   ADP_ADVERTISE_ADVERTISE_1,
	   ADP_ADVERTISE_ADVERTISE_2,
	   ADP_ADVERTISE_WAITING,
	   ADP_ADVERTISE_DEPARTING_1,
	   ADP_ADVERTISE_DEPARTING_2
} adp_advertise_state = ADP_ADVERTISE_IDLE;

enum { ADP_DISCOVERY_IDLE,
	   ADP_DISCOVERY_WAITING,
	   ADP_DISCOVERY_DISCOVER,
	   ADP_DISCOVERY_ADDED,
	   ADP_DISCOVERY_REMOVED
} adp_discovery_state = ADP_DISCOVERY_IDLE;

enum { ACMP_TALKER_IDLE,
	   ACMP_TALKER_WAITING,
	   ACMP_TALKER_CONNECT,
	   ACMP_TALKER_DISCONNECT,
	   ACMP_TALKER_GET_STATE,
	   ACMP_TALKER_GET_CONNECTION,
	   ACMP_TALKER_WAITING_FOR_CONNECT,
	   ACMP_TALKER_WAITING_FOR_DISCONNECT
} acmp_talker_state = ACMP_TALKER_IDLE;

enum { ACMP_LISTENER_IDLE,
	   ACMP_LISTENER_WAITING,
	   ACMP_LISTENER_CONNECT_RX_COMMAND,
	   ACMP_LISTENER_DISCONNECT_RX_COMMAND,
	   ACMP_LISTENER_CONNECT_TX_RESPONSE,
	   ACMP_LISTENER_DISCONNECT_TX_RESPONSE,
	   ACMP_LISTENER_GET_STATE,
	   ACMP_LISTENER_WAITING_FOR_CONNECT,
	   ACMP_LISTENER_WAITING_FOR_DISCONNECT
} acmp_listener_state = ACMP_LISTENER_IDLE;

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
	unsigned int default_format;
	short talker_unique_id;
	short listener_unique_id;
	short connection_count;
	short sequence_id;
	short flags;
	char stream_dest_mac[6];
	char message_type;
	char status;
} avb_1722_1_acmp_rcvd_cmd_resp;

typedef struct {
	guid_t guid;
	short unique_id;
	short padding;
} avb_1722_1_acmp_listener_pair;

typedef struct {
    stream_t stream_id;
	short connection_count;
	char destination_mac[6];
	avb_1722_1_acmp_listener_pair listeners[AVB_1722_1_MAX_LISTENERS_PER_TALKER];
} avb_1722_1_acmp_talker_stream_info;

typedef struct {
	guid_t talker_guid;
	short talker_unique_id;
	short connected;
	stream_t stream_id;
	char destination_mac[6];
	short padding;
} avb_1722_1_acmp_listener_stream_info;

typedef struct {
	unsigned timeout;
	short retried;
	short command;
	short original_sequence_id;
	short dummy;
} avb_1722_1_acmp_inflight_command;


//! Entity database
avb_1722_1_entity_record entities[AVB_1722_1_MAX_ENTITIES];

//! Listener stream database
avb_1722_1_acmp_listener_stream_info acmp_listener_streams[AVB_1722_1_MAX_LISTENERS];

//! Talker stream database
avb_1722_1_acmp_talker_stream_info acmp_talker_streams[AVB_1722_1_MAX_TALKERS];

//! Listener in-flight command database
avb_1722_1_acmp_inflight_command acmp_inflight_commands[AVB_1722_1_MAX_INFLIGHT_COMMANDS];

//! Talker's rcvdCmdResp
avb_1722_1_acmp_rcvd_cmd_resp acmp_talker_rcvd_cmd_resp;

//! Listener's rcvdCmdResp
avb_1722_1_acmp_rcvd_cmd_resp acmp_listener_rcvd_cmd_resp;

static int qlog2(unsigned n)
{
	int l=0;
	if (n==0) return -1;
	while ((n & 1) == 0)
	{
		n >>= 1;
		l++;
	}
	if ((n >> 1) != 0) return -1;
	return l;
}

void avb_1722_1_init(unsigned char macaddr[6], unsigned char serial_number[2])
{
  for (int i=0;i<6;i++)
    my_mac_addr[i] = macaddr[i];

	my_guid.c[0] = macaddr[5];
	my_guid.c[1] =  macaddr[4];
	my_guid.c[2] = macaddr[3];
	my_guid.c[3] = 0xfe;
	my_guid.c[4] = 0xff;
	my_guid.c[5] = macaddr[2];
	my_guid.c[6] = macaddr[1];
	my_guid.c[7] = macaddr[0];

	init_avb_timer(&adp_advertise_timer, 1);
	init_avb_timer(&adp_readvertise_timer, 100);
	init_avb_timer(&adp_discovery_timer, 200);
	init_avb_timer(&ptp_monitor_timer, 100);

	adp_discovery_state = ADP_DISCOVERY_WAITING;
	start_avb_timer(&adp_discovery_timer, 1);

	acmp_talker_state = ACMP_TALKER_WAITING;
	acmp_listener_state = ACMP_LISTENER_WAITING;
}



static void avb_1722_1_entity_database_add(avb_1722_1_adp_packet_t* pkt)
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
		entities[found_slot_index].timeout = GET_1722_1_VALID_TIME(&pkt->header) + adp_two_second_counter;
		return;
	}
}

static void avb_1722_1_entity_database_remove(avb_1722_1_adp_packet_t* pkt)
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

		if (entities[i].timeout < adp_two_second_counter) {
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


static char* avb_1722_1_create_aecp_packet(int message_type, avb_1722_1_aecp_packet_t* cmd_pkt)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + 1);

	avb_1722_1_create_1722_1_header(avb_1722_1_aecp_dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, message_type, 0, 40, hdr);

	memcpy(pkt->target_guid, cmd_pkt->target_guid, (pkt->data.payload - pkt->target_guid));

	return pkt->data.payload;
}

static avb_status_t process_avb_1722_1_aecp_packet(avb_1722_1_aecp_packet_t* pkt, chanend c_tx)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));
	if (compare_guid(pkt->target_guid, &my_guid)==0) return AVB_1722_1_OK;

	switch (message_type) {
	case AECP_CMD_AVDECC_MSG_COMMAND: {
		unsigned ret;

		if (pkt->data.avdecc.oui_sect_flags[0]&0x8)
		{
			avb_1722_1_aecp_avdecc_msg_t* payload = (avb_1722_1_aecp_avdecc_msg_t*)avb_1722_1_create_aecp_packet(AECP_CMD_AVDECC_MSG_RESPONSE, pkt);

			unsigned type_code = (pkt->data.avdecc.type_code_flags[0]&0x7f)<<8 |
					              pkt->data.avdecc.type_code_flags[1];
			unsigned length_of_data = avb_1722_1_aecp_parameter_length[type_code];

			// Copy address into the output
			memcpy(payload->oui, pkt->data.avdecc.oui, 16);

			// Write data into the output
			ret = avb_1722_1_walk_tree(pkt->data.avdecc.oui, pkt->data.avdecc.oui_sect_flags[0]&0x2, payload->mode_specific_data);
			if (ret) {
				// set as positive acknoledgement
				payload->oui_sect_flags[0] |= 0x4;

				// zero off rest of data
				if (ret < length_of_data)
					memset(&payload->mode_specific_data[ret], 0, length_of_data-ret);
				else
					length_of_data = ret;
			} else {
				if (pkt->data.avdecc.oui_sect_flags[0]&0x4) {
					// set as negative acknowledgement
					payload->oui_sect_flags[0] &= ~0x4;
					ret = 1;
				}
			}

			if (ret) {

				// set as an acknowledgement
				payload->oui_sect_flags[0] &= ~0x8;

				// copy source address to destination
				// add our address as source
				// transmit result
				mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+36+length_of_data, ETH_BROADCAST);
			}
		}
		break;
	}
	case AECP_CMD_ADDRESS_ACCESS_COMMAND: {
		break;
	}
	case AECP_CMD_AVC_COMMAND: {
		break;
	}
	case AECP_CMD_VENDOR_UNIQUE_COMMAND: {
		break;
	}
	case AECP_CMD_EXTENDED_COMMAND: {
		break;
	}
	default:
		// This node is not expecting a response
		break;
	}
	return AVB_1722_1_OK;
}

//----------------------------------------------------------------------------------------

static unsigned acmp_listener_valid_listener_unique()
{
	return acmp_listener_rcvd_cmd_resp.listener_unique_id < AVB_1722_1_MAX_LISTENERS;
}

static unsigned acmp_listener_listener_is_connected()
{
	enum avb_sink_state_t state;
	get_avb_sink_state(acmp_listener_rcvd_cmd_resp.listener_unique_id, &state);
	return state != AVB_SINK_STATE_DISABLED;
}

static avb_1722_1_acmp_status_type acmp_listener_get_state()
{
	return ACMP_STATUS_SUCCESS;
}

static unsigned acmp_talker_valid_talker_unique()
{
	return acmp_talker_rcvd_cmd_resp.listener_unique_id < AVB_1722_1_MAX_TALKERS;
}

static avb_1722_1_acmp_status_type acmp_talker_get_state()
{
	return ACMP_STATUS_SUCCESS;
}

static avb_1722_1_acmp_status_type acmp_talker_get_connection()
{
	return ACMP_STATUS_SUCCESS;
}

static void avb_1722_1_create_acmp_packet(avb_1722_1_acmp_rcvd_cmd_resp* rcr)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_acmp_packet_t *pkt = (avb_1722_1_acmp_packet_t*) (hdr + 1);

	avb_1722_1_create_1722_1_header(avb_1722_1_acmp_dest_addr, DEFAULT_1722_1_ACMP_SUBTYPE, rcr->message_type, rcr->status, 40, hdr);

	SET_LONG_WORD(pkt->stream_id, rcr->stream_id);
	SET_LONG_WORD(pkt->controller_guid, rcr->controller_guid);
	SET_LONG_WORD(pkt->listener_guid, rcr->listener_guid);
	SET_LONG_WORD(pkt->talker_guid, rcr->talker_guid);
	SET_WORD_CONST(pkt->default_format, rcr->default_format);
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

static void acmp_listener_tx_command(unsigned message_type, unsigned timeout_ms, chanend c_tx)
{
	acmp_listener_rcvd_cmd_resp.message_type = message_type;
	acmp_listener_rcvd_cmd_resp.status = ACMP_STATUS_SUCCESS;
	avb_1722_1_create_acmp_packet(&acmp_listener_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_acmp_packet_t), ETH_BROADCAST);

	// set up inflight command
}

static void acmp_listener_tx_response(unsigned type, unsigned error_code, chanend c_tx)
{
	acmp_listener_rcvd_cmd_resp.message_type = type;
	acmp_listener_rcvd_cmd_resp.status = error_code;
	avb_1722_1_create_acmp_packet(&acmp_listener_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_acmp_packet_t), ETH_BROADCAST);
}

static void acmp_talker_tx_response(unsigned type, unsigned error_code, chanend c_tx)
{
	unsigned talker = acmp_talker_rcvd_cmd_resp.talker_unique_id;
	acmp_talker_rcvd_cmd_resp.message_type = type;
	acmp_talker_rcvd_cmd_resp.status = error_code;

	acmp_talker_rcvd_cmd_resp.stream_id = acmp_talker_streams[talker].stream_id;
	for (unsigned c=0; c<6; ++c)
		acmp_talker_rcvd_cmd_resp.stream_dest_mac[c] = acmp_talker_streams[talker].destination_mac[c];

	avb_1722_1_create_acmp_packet(&acmp_talker_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_acmp_packet_t), ETH_BROADCAST);
}

static void acmp_listener_remove_inflight()
{
	for (unsigned int i=0; i<AVB_1722_1_MAX_INFLIGHT_COMMANDS; ++i) {
		//inflight_commands[i];
	}
}

static void acmp_listener_check_inflight_command_timeouts()
{
	for (unsigned int i=0; i<AVB_1722_1_MAX_INFLIGHT_COMMANDS; ++i) {
		//inflight_commands[i];
	}
}

static void store_rcvd_cmd_resp(avb_1722_1_acmp_rcvd_cmd_resp* store, avb_1722_1_acmp_packet_t* pkt)
{
	GET_LONG_WORD(store->stream_id, pkt->stream_id);
	GET_LONG_WORD(store->controller_guid, pkt->controller_guid);
	GET_LONG_WORD(store->listener_guid, pkt->listener_guid);
	GET_LONG_WORD(store->talker_guid, pkt->talker_guid);
	GET_WORD(store->default_format, pkt->default_format);
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

static avb_status_t process_avb_1722_1_acmp_talker_packet(unsigned message_type, avb_1722_1_acmp_packet_t* pkt)
{
	if (compare_guid(pkt->talker_guid, &my_guid)==0) return AVB_1722_1_OK;
	if (acmp_talker_state!=ACMP_TALKER_WAITING) { return AVB_1722_1_OK; }

	store_rcvd_cmd_resp(&acmp_talker_rcvd_cmd_resp, pkt);

	switch (message_type)
	{
	case ACMP_CMD_CONNECT_TX_COMMAND:
		acmp_talker_state = ACMP_TALKER_CONNECT;
		break;
	case ACMP_CMD_DISCONNECT_TX_COMMAND:
		acmp_talker_state = ACMP_TALKER_DISCONNECT;
		break;
	case ACMP_CMD_GET_TX_STATE_COMMAND:
		acmp_talker_state = ACMP_TALKER_GET_STATE;
		break;
	case ACMP_CMD_GET_TX_CONNECTION_COMMAND:
		acmp_talker_state = ACMP_TALKER_GET_CONNECTION;
		break;
	}

	return AVB_1722_1_OK;
}

static avb_status_t process_avb_1722_1_acmp_listener_packet(unsigned message_type, avb_1722_1_acmp_packet_t* pkt)
{
	if (compare_guid(pkt->listener_guid, &my_guid)==0) return AVB_1722_1_OK;
	if (acmp_listener_state!=ACMP_LISTENER_WAITING) { return AVB_1722_1_OK; }

	store_rcvd_cmd_resp(&acmp_listener_rcvd_cmd_resp, pkt);

	switch (message_type)
	{
	case ACMP_CMD_CONNECT_TX_RESPONSE:
		acmp_listener_state = ACMP_LISTENER_CONNECT_TX_RESPONSE;
		break;
	case ACMP_CMD_DISCONNECT_TX_RESPONSE:
		acmp_listener_state = ACMP_LISTENER_DISCONNECT_TX_RESPONSE;
		break;
	case ACMP_CMD_CONNECT_RX_COMMAND:
		acmp_listener_state = ACMP_LISTENER_CONNECT_RX_COMMAND;
		break;
	case ACMP_CMD_DISCONNECT_RX_COMMAND:
		acmp_listener_state = ACMP_LISTENER_DISCONNECT_RX_COMMAND;
		break;
	case ACMP_CMD_GET_RX_STATE_COMMAND:
		acmp_listener_state = ACMP_LISTENER_GET_STATE;
		break;
	}

	return AVB_1722_1_OK;
}

static avb_status_t process_avb_1722_1_acmp_packet(avb_1722_1_acmp_packet_t* pkt, chanend c_tx)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

	switch (message_type) {

	// Talker messages
	case ACMP_CMD_CONNECT_TX_COMMAND:
	case ACMP_CMD_DISCONNECT_TX_COMMAND:
	case ACMP_CMD_GET_TX_STATE_COMMAND:
	case ACMP_CMD_GET_TX_CONNECTION_COMMAND:
		return process_avb_1722_1_acmp_talker_packet(message_type, pkt);

	// Listener messages
	case ACMP_CMD_CONNECT_TX_RESPONSE:
	case ACMP_CMD_DISCONNECT_TX_RESPONSE:
	case ACMP_CMD_CONNECT_RX_COMMAND:
	case ACMP_CMD_DISCONNECT_RX_COMMAND:
	case ACMP_CMD_GET_RX_STATE_COMMAND:
		return process_avb_1722_1_acmp_listener_packet(message_type, pkt);
	}

	return AVB_1722_1_OK;
}

static avb_status_t avb_1722_1_acmp_talker_periodic(chanend c_tx)
{
	switch (acmp_talker_state) {
	case ACMP_TALKER_IDLE:
	case ACMP_TALKER_WAITING:
	case ACMP_TALKER_WAITING_FOR_CONNECT:
	case ACMP_TALKER_WAITING_FOR_DISCONNECT:
		return AVB_NO_STATUS;

	case ACMP_TALKER_CONNECT:
		if (!acmp_talker_valid_talker_unique(1)) {
			acmp_talker_tx_response(ACMP_CMD_CONNECT_TX_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
			acmp_talker_state = ACMP_TALKER_WAITING;
			return AVB_NO_STATUS;
		} else {
			acmp_talker_state = ACMP_TALKER_WAITING_FOR_CONNECT;
			return AVB_1722_1_CONNECT_TALKER;
		}
		break;
	case ACMP_TALKER_DISCONNECT:
		if (!acmp_talker_valid_talker_unique(1)) {
			acmp_talker_tx_response(ACMP_CMD_DISCONNECT_TX_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
			acmp_talker_state = ACMP_TALKER_WAITING;
			return AVB_NO_STATUS;
		} else {
			acmp_talker_state = ACMP_TALKER_WAITING_FOR_DISCONNECT;
			return AVB_1722_1_DISCONNECT_TALKER;
		}
		break;
	case ACMP_TALKER_GET_STATE:
		if (!acmp_talker_valid_talker_unique(1)) {
			acmp_talker_tx_response(ACMP_CMD_GET_TX_STATE_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
		} else {
			acmp_talker_tx_response(ACMP_CMD_GET_TX_STATE_RESPONSE, acmp_talker_get_state(), c_tx);
		}
		acmp_talker_state = ACMP_TALKER_WAITING;
		return AVB_NO_STATUS;
	case ACMP_TALKER_GET_CONNECTION:
		if (!acmp_talker_valid_talker_unique(1)) {
			acmp_talker_tx_response(ACMP_CMD_GET_TX_CONNECTION_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
		} else {
			acmp_talker_tx_response(ACMP_CMD_GET_TX_CONNECTION_RESPONSE, acmp_talker_get_connection(), c_tx);
		}
		acmp_talker_state = ACMP_TALKER_WAITING;
		return AVB_NO_STATUS;
	}

	return AVB_NO_STATUS;
}

static avb_status_t avb_1722_1_acmp_listener_periodic(chanend c_tx)
{
	switch (acmp_listener_state) {
	case ACMP_LISTENER_IDLE:
	case ACMP_LISTENER_WAITING_FOR_CONNECT:
	case ACMP_LISTENER_WAITING_FOR_DISCONNECT:
		break;
	case ACMP_LISTENER_WAITING:
		acmp_listener_check_inflight_command_timeouts();
		break;
	case ACMP_LISTENER_CONNECT_RX_COMMAND:
		if (!acmp_listener_valid_listener_unique()) {
			acmp_listener_tx_response(ACMP_CMD_CONNECT_RX_RESPONSE, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
		} else {
			{
				acmp_listener_tx_command(ACMP_CMD_CONNECT_TX_COMMAND, 2000, c_tx);
			}
		}
		acmp_listener_state = ACMP_LISTENER_WAITING;
		break;
	case ACMP_LISTENER_DISCONNECT_RX_COMMAND:
		if (!acmp_listener_valid_listener_unique()) {
			acmp_listener_tx_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
		} else {
			if (acmp_listener_listener_is_connected()) {
				acmp_listener_tx_command(ACMP_CMD_DISCONNECT_TX_COMMAND, 200, c_tx);
			} else {
				acmp_listener_tx_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, ACMP_STATUS_NOT_CONNECTED, c_tx);
			}
		}
		acmp_listener_state = ACMP_LISTENER_WAITING;
		break;
	case ACMP_LISTENER_CONNECT_TX_RESPONSE:
		if (acmp_listener_valid_listener_unique()) {
			acmp_listener_tx_response(ACMP_CMD_CONNECT_RX_RESPONSE, 0, c_tx);
			acmp_listener_remove_inflight();
			acmp_listener_state = ACMP_LISTENER_WAITING_FOR_CONNECT;
			return AVB_1722_1_CONNECT_LISTENER;
		}
		break;
	case ACMP_LISTENER_DISCONNECT_TX_RESPONSE:
		if (acmp_listener_valid_listener_unique()) {
			acmp_listener_tx_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, 0, c_tx);
			acmp_listener_remove_inflight();
			acmp_listener_state = ACMP_LISTENER_WAITING_FOR_DISCONNECT;
			return AVB_1722_1_DISCONNECT_LISTENER;
		}
		break;
	case ACMP_LISTENER_GET_STATE:
		if (!acmp_listener_valid_listener_unique()) {
			acmp_listener_tx_response(ACMP_CMD_GET_RX_STATE_RESPONSE, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
		} else {
			acmp_listener_tx_response(ACMP_CMD_GET_RX_STATE_RESPONSE, acmp_listener_get_state(), c_tx);
		}
		acmp_listener_state = ACMP_LISTENER_WAITING;
		break;
	}

	return AVB_NO_STATUS;
}

unsigned avb_1722_1_acmp_get_talker_connection_info(short *talker)
{
	*talker = acmp_talker_rcvd_cmd_resp.talker_unique_id;
	return 1;
}

unsigned avb_1722_1_acmp_get_listener_connection_info(short *listener, char address[6], unsigned streamId[2], unsigned* vlan)
{
	*listener = acmp_listener_rcvd_cmd_resp.listener_unique_id;
	for (unsigned c=0; c<6; ++c) address[c] = acmp_listener_rcvd_cmd_resp.stream_dest_mac[c];
	streamId[1] = (unsigned)(acmp_listener_rcvd_cmd_resp.stream_id.l >> 0);
	streamId[0] = (unsigned)(acmp_listener_rcvd_cmd_resp.stream_id.l >> 32);
	*vlan = 2;
	return 1;
}

void avb_1722_1_acmp_talker_connection_complete(short code, chanend c_tx)
{
	switch (acmp_talker_state)
	{
	case ACMP_TALKER_WAITING_FOR_CONNECT:
		acmp_talker_tx_response(ACMP_CMD_CONNECT_TX_RESPONSE, code, c_tx);
		acmp_talker_state = ACMP_TALKER_WAITING;
		break;
	case ACMP_TALKER_WAITING_FOR_DISCONNECT:
		acmp_talker_tx_response(ACMP_CMD_DISCONNECT_TX_RESPONSE, code, c_tx);
		acmp_talker_state = ACMP_TALKER_WAITING;
		break;
	default:
		break;
	}
}


void avb_1722_1_acmp_listener_connection_complete(short code, chanend c_tx)
{
	switch (acmp_listener_state)
	{
	case ACMP_LISTENER_WAITING_FOR_CONNECT:
		acmp_listener_rcvd_cmd_resp.status = code;
		acmp_listener_state = ACMP_LISTENER_WAITING;
		break;
	case ACMP_LISTENER_WAITING_FOR_DISCONNECT:
		acmp_listener_rcvd_cmd_resp.status = code;
		acmp_listener_state = ACMP_LISTENER_WAITING;
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
			acmp_talker_streams[talker_unique_id].destination_mac[i] = macaddr[i];
	}
}

void avb_1722_1_talker_set_stream_id(unsigned talker_unique_id, unsigned streamId[2])
{
	if (talker_unique_id < AVB_1722_1_MAX_TALKERS)
	{
		acmp_talker_streams[talker_unique_id].stream_id.l = (unsigned long long)streamId[1] + (((unsigned long long)streamId[0]) << 32);
	}
}

short avb_1722_1_acmp_configure_source(unsigned talker_unique_id, unsigned int default_format)
{
	unsigned number_of_channels;
	unsigned sample_rate;
	unsigned state=0;
	int associated_clock=0;

	int channel_index = qlog2(default_format & 0xFFFF);
	int rate_index = qlog2((default_format&0xFC000000)>>26);
	if (channel_index==-1 || rate_index==-1) return ACMP_STATUS_TALKER_DEFAULT_FORMAT_INVALID;

	number_of_channels = avb_1722_1_acmp_default_format_channel_counts[channel_index];
	sample_rate = avb_1722_1_acmp_default_format_frequency[rate_index];

	if (talker_unique_id > AVB_NUM_LISTENER_UNITS) return ACMP_STATUS_TALKER_UNKNOWN_ID;

	if (number_of_channels > AVB_NUM_MEDIA_INPUTS)
	{
		return ACMP_STATUS_TALKER_DEFAULT_FORMAT_INVALID;
	}

	get_avb_source_sync(talker_unique_id, &associated_clock);

	get_avb_source_state(talker_unique_id, &state);
	if (state != AVB_SOURCE_STATE_DISABLED)
	{
		int current_number_of_channels=0;
		int current_sample_rate=0;
		get_device_media_clock_rate(associated_clock, &current_sample_rate);
		get_avb_source_channels(talker_unique_id, &current_number_of_channels);
		if (sample_rate != current_sample_rate) return ACMP_STATUS_DEFAULT_SET_DIFFERENT;
		if (number_of_channels != current_number_of_channels) return ACMP_STATUS_DEFAULT_SET_DIFFERENT;
	}
	else
	{
		int channel_map[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		set_avb_source_channels(talker_unique_id, number_of_channels);
		set_avb_source_format(talker_unique_id, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);
		set_avb_source_map(talker_unique_id, channel_map, number_of_channels);
		set_device_media_clock_rate(associated_clock, sample_rate);
	}
	return ACMP_STATUS_SUCCESS;
}

short avb_1722_1_acmp_configure_sink(unsigned listener_unique_id, unsigned int default_format)
{
	unsigned number_of_channels;
	unsigned sample_rate;
	unsigned state=0;
	int associated_clock=0;

	int channel_index = qlog2(default_format & 0xFFFF);
	int rate_index = qlog2((default_format&0xFC000000)>>26);
	if (channel_index==-1 || rate_index==-1) return ACMP_STATUS_LISTENER_DEFAULT_FORMAT_INVALID;

	number_of_channels = avb_1722_1_acmp_default_format_channel_counts[channel_index];
	sample_rate = avb_1722_1_acmp_default_format_frequency[rate_index];

	if (listener_unique_id > AVB_NUM_LISTENER_UNITS) return ACMP_STATUS_LISTENER_UNKNOWN_ID;

	if (number_of_channels > AVB_NUM_MEDIA_OUTPUTS)
	{
		return ACMP_STATUS_LISTENER_DEFAULT_FORMAT_INVALID;
	}

	get_avb_sink_sync(listener_unique_id, &associated_clock);

	get_avb_source_state(listener_unique_id, &state);
	if (state != AVB_SINK_STATE_DISABLED)
	{
		int current_number_of_channels=0;
		int current_sample_rate=0;
		get_device_media_clock_rate(associated_clock, &current_sample_rate);
		get_avb_sink_channels(listener_unique_id, &current_number_of_channels);
		if (sample_rate != current_sample_rate) return ACMP_STATUS_DEFAULT_SET_DIFFERENT;
		if (number_of_channels != current_number_of_channels) return ACMP_STATUS_DEFAULT_SET_DIFFERENT;
	}
	else
	{
		int channel_map[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		set_avb_sink_channels(listener_unique_id, number_of_channels);
		set_avb_sink_map(listener_unique_id, channel_map, number_of_channels);
		set_device_media_clock_rate(associated_clock, sample_rate);
	}
	return ACMP_STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------

avb_status_t process_avb_1722_1_adp_packet(avb_1722_1_adp_packet_t* pkt, chanend c_tx)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));
	guid_t zero_guid = { 0 };

	switch (message_type) {
	case ENTITY_DISCOVER: {
		if ( compare_guid(pkt->entity_guid, &my_guid) || compare_guid(pkt->entity_guid, &zero_guid) )
		{
			if (adp_advertise_state == ADP_ADVERTISE_WAITING)
				adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
		}
		return AVB_1722_1_OK;
	}

	case ENTITY_AVAILABLE: {
		avb_1722_1_entity_database_add(pkt);
		adp_discovery_state = ADP_DISCOVERY_ADDED;
		return AVB_1722_1_OK;
	}

	case ENTITY_DEPARTING: {
		avb_1722_1_entity_database_remove(pkt);
		adp_discovery_state = ADP_DISCOVERY_REMOVED;
		return AVB_1722_1_OK;
	}

	}

	return AVB_1722_1_OK;
}

static void avb_1722_1_create_adp_packet(int message_type, guid_t guid)
{
	  ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	  avb_1722_1_adp_packet_t *pkt = (avb_1722_1_adp_packet_t*) (hdr + 1);

	  memset(pkt, 0, sizeof(avb_1722_1_adp_packet_t));

	  avb_1722_1_create_1722_1_header(avb_1722_1_adp_dest_addr, DEFAULT_1722_1_ADP_SUBTYPE, message_type,
			  (message_type==ENTITY_AVAILABLE)?AVB_1722_1_ADP_VALID_TIME:0, 40, hdr);

	  SET_LONG_WORD(pkt->entity_guid, guid);

	  if (message_type!=ENTITY_DISCOVER) {
		  SET_WORD_CONST(pkt->vendor_id, AVB_1722_1_ADP_VENDOR_ID);
		  SET_WORD_CONST(pkt->model_id, AVB_1722_1_ADP_MODEL_ID);
		  SET_WORD_CONST(pkt->entity_capabilities, AVB_1722_1_ADP_ENTITY_CAPABILITIES);
		  pkt->talker_stream_sources = AVB_1722_1_ADP_TALKER_STREAM_SOURCES;
		  pkt->talker_capabilities = AVB_1722_1_ADP_TALKER_CAPABILITIES;
		  pkt->listener_stream_sinks = AVB_1722_1_ADP_LISTENER_STREAM_SINKS;
		  pkt->listener_capabilites = AVB_1722_1_ADP_LISTENER_CAPABILITIES;
		  SET_WORD_CONST(pkt->controller_capabilities, AVB_1722_1_ADP_CONTROLLER_CAPABILITIES);
		  SET_WORD_CONST(pkt->available_index, avb_1722_1_available_index);
		  SET_LONG_WORD(pkt->as_grandmaster_id, as_grandmaster_id);
		  SET_WORD_CONST(pkt->default_audio_format, AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT);
		  SET_WORD_CONST(pkt->default_video_format, AVB_1722_1_ADP_DEFAULT_VIDEO_FORMAT);
	  }
}

static avb_status_t avb_1722_1_adp_discovery_periodic(chanend c_tx)
{
	switch (adp_discovery_state) {
	case ADP_DISCOVERY_IDLE:
		break;

	case ADP_DISCOVERY_WAITING: {
			unsigned lost=0;
			if (avb_timer_expired(&adp_discovery_timer)) {
				adp_two_second_counter++;
				lost = avb_1722_1_entity_database_check_timeout();
				start_avb_timer(&adp_discovery_timer, 1);
			}
			return (lost > 0) ? AVB_1722_1_ENTITY_REMOVED : AVB_NO_STATUS;
		}

	case ADP_DISCOVERY_DISCOVER: {
			avb_1722_1_create_adp_packet(ENTITY_DISCOVER, discover_guid);
			mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_adp_packet_t), ETH_BROADCAST);
			adp_discovery_state = ADP_DISCOVERY_WAITING;
		}
		break;

	case ADP_DISCOVERY_ADDED:
		adp_discovery_state = ADP_DISCOVERY_WAITING;
		return AVB_1722_1_ENTITY_ADDED;

	case ADP_DISCOVERY_REMOVED:
		adp_discovery_state = ADP_DISCOVERY_WAITING;
		return AVB_1722_1_ENTITY_REMOVED;
	}

	return AVB_NO_STATUS;
}

static avb_status_t avb_1722_1_adp_advertising_periodic(chanend c_tx, chanend ptp)
{
	switch (adp_advertise_state) {
	case ADP_ADVERTISE_IDLE:
		break;

	case ADP_ADVERTISE_ADVERTISE_0:
		{
			guid_t ptp_current;
			avb_1722_1_adp_change_ptp_grandmaster(ptp_current.c);
			start_avb_timer(&ptp_monitor_timer, 1); //Every second
			adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
		}
		//We just immediatley fall through so it will send immediatley

	case ADP_ADVERTISE_ADVERTISE_1:
		avb_1722_1_create_adp_packet(ENTITY_AVAILABLE, my_guid);
		mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_adp_packet_t), ETH_BROADCAST);
		start_avb_timer(&adp_advertise_timer, 3); // 3 centiseconds
		adp_advertise_state = ADP_ADVERTISE_ADVERTISE_2;
		break;

	case ADP_ADVERTISE_ADVERTISE_2:
		if (avb_timer_expired(&adp_advertise_timer)) {
			avb_1722_1_create_adp_packet(ENTITY_AVAILABLE, my_guid);
			mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_adp_packet_t), ETH_BROADCAST);
			start_avb_timer(&adp_readvertise_timer, AVB_1722_1_ADP_REPEAT_TIME);
			adp_advertise_state = ADP_ADVERTISE_WAITING;
			avb_1722_1_available_index++;
		}
		break;

	case ADP_ADVERTISE_WAITING:
		if (avb_timer_expired(&adp_readvertise_timer)) {
			adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
		}
		break;

	case ADP_ADVERTISE_DEPARTING_1:
		avb_1722_1_create_adp_packet(ENTITY_DEPARTING, my_guid);
		mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_adp_packet_t), ETH_BROADCAST);
		start_avb_timer(&adp_advertise_timer, 3); // 3 centiseconds
		adp_advertise_state = ADP_ADVERTISE_DEPARTING_2;
		break;

	case ADP_ADVERTISE_DEPARTING_2:
		if (avb_timer_expired(&adp_advertise_timer)) {
			avb_1722_1_create_adp_packet(ENTITY_DEPARTING, my_guid);
			mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_adp_packet_t), ETH_BROADCAST);
			adp_advertise_state = ADP_ADVERTISE_IDLE;
			avb_1722_1_available_index=0;
		}
		break;

	default:
		break;
	}

	if(ADP_ADVERTISE_IDLE != adp_advertise_state)
	{
		if(avb_timer_expired(&ptp_monitor_timer))
		{
			guid_t ptp_current;
			ptp_get_current_grandmaster(ptp, ptp_current.c);
			if(as_grandmaster_id.l != ptp_current.l)
			{
				avb_1722_1_adp_change_ptp_grandmaster(ptp_current.c);
				adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
			}
			start_avb_timer(&ptp_monitor_timer, 1); //Every second
		}
	}

	return AVB_NO_STATUS;
}

void avb_1722_1_adp_announce()
{
	if (adp_advertise_state == ADP_ADVERTISE_IDLE) adp_advertise_state = ADP_ADVERTISE_ADVERTISE_0;
}


void avb_1722_1_adp_depart()
{
	if (adp_advertise_state == ADP_ADVERTISE_IDLE) adp_advertise_state = ADP_ADVERTISE_DEPARTING_1;
}

void avb_1722_1_adp_discover(unsigned guid[])
{
	if (adp_discovery_state == ADP_DISCOVERY_WAITING) {
		adp_discovery_state = ADP_DISCOVERY_DISCOVER;
		discover_guid.l = (unsigned long long)guid[0] + ((unsigned long long)guid[1] << 32);
	}
}

void avb_1722_1_adp_discover_all()
{
	unsigned guid[2] = {0,0};
	avb_1722_1_adp_discover(guid);
}

void avb_1722_1_adp_change_ptp_grandmaster(char grandmaster[8])
{
	int i;
	for(i = 0; i < 8; i++)
	{
		as_grandmaster_id.c[i] = grandmaster[i];
	}
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
		  case DEFAULT_1722_1_ADP_SUBTYPE:
			  return process_avb_1722_1_adp_packet((avb_1722_1_adp_packet_t*)pkt, c_tx);
		  case DEFAULT_1722_1_AECP_SUBTYPE:
			  return process_avb_1722_1_aecp_packet((avb_1722_1_aecp_packet_t*)pkt, c_tx);
		  case DEFAULT_1722_1_ACMP_SUBTYPE:
			  return process_avb_1722_1_acmp_packet((avb_1722_1_acmp_packet_t*)pkt, c_tx);
		  default:
			  return AVB_NO_STATUS;
		  }
	  }

	  return AVB_NO_STATUS;
}

avb_status_t avb_1722_1_periodic(chanend c_tx, chanend c_ptp)
{
	avb_status_t res;
	res = avb_1722_1_adp_advertising_periodic(c_tx, c_ptp);
	if (res != AVB_NO_STATUS) return res;
	res = avb_1722_1_adp_discovery_periodic(c_tx);
	if (res != AVB_NO_STATUS) return res;
	res = avb_1722_1_acmp_listener_periodic(c_tx);
	if (res != AVB_NO_STATUS) return res;
	return avb_1722_1_acmp_talker_periodic(c_tx);
}



