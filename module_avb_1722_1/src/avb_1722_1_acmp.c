#include "avb_1722_common.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_acmp.h"
#include "avb_api.h"

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

extern unsigned int avb_1722_1_buf[];
extern guid_t my_guid;

static const unsigned char avb_1722_1_acmp_dest_addr[6] = AVB_1722_1_ACMP_DEST_MAC;

// The channel counts defined by each bit in the ACMP default audio format word
static const unsigned char avb_1722_1_acmp_default_format_channel_counts[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24
};

// The sample rates defined by each bit in the ACMP default audio format word
static const unsigned int avb_1722_1_acmp_default_format_frequency[] = {
	44100, 48000, 88200, 96000, 176400, 192000
};

// Listener stream database
avb_1722_1_acmp_listener_stream_info acmp_listener_streams[AVB_1722_1_MAX_LISTENERS];

// Talker stream database
avb_1722_1_acmp_talker_stream_info acmp_talker_streams[AVB_1722_1_MAX_TALKERS];

// Listener in-flight command database
avb_1722_1_acmp_inflight_command acmp_inflight_commands[AVB_1722_1_MAX_INFLIGHT_COMMANDS];

// Talker's rcvdCmdResp
avb_1722_1_acmp_rcvd_cmd_resp acmp_talker_rcvd_cmd_resp;

// Listener's rcvdCmdResp
avb_1722_1_acmp_rcvd_cmd_resp acmp_listener_rcvd_cmd_resp;

void avb_1722_1_acmp_init()
{
	acmp_talker_state = ACMP_TALKER_WAITING;
	acmp_listener_state = ACMP_LISTENER_WAITING;
}

static unsigned acmp_listener_valid_listener_unique()
{
	return acmp_listener_rcvd_cmd_resp.listener_unique_id < AVB_1722_1_MAX_LISTENERS;
}

static unsigned acmp_listener_is_connected()
{
	enum avb_sink_state_t state;
	get_avb_sink_state(acmp_listener_rcvd_cmd_resp.listener_unique_id, &state);
	return state != AVB_SINK_STATE_DISABLED;
}

static avb_1722_1_acmp_status_t acmp_listener_get_state()
{
	return ACMP_STATUS_SUCCESS;
}

static unsigned acmp_talker_valid_talker_unique()
{
	return acmp_talker_rcvd_cmd_resp.listener_unique_id < AVB_1722_1_MAX_TALKERS;
}

static avb_1722_1_acmp_status_t acmp_talker_get_state()
{
	return ACMP_STATUS_SUCCESS;
}

static avb_1722_1_acmp_status_t acmp_talker_get_connection()
{
	return ACMP_STATUS_SUCCESS;
}

static void avb_1722_1_create_acmp_packet(avb_1722_1_acmp_rcvd_cmd_resp* rcr)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_acmp_packet_t *pkt = (avb_1722_1_acmp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

	avb_1722_1_create_1722_1_header(avb_1722_1_acmp_dest_addr, DEFAULT_1722_1_ACMP_SUBTYPE, rcr->message_type, rcr->status, AVB_1722_1_ACMP_CD_LENGTH, hdr);

	SET_LONG_WORD(pkt->stream_id, rcr->stream_id);
	SET_LONG_WORD(pkt->controller_guid, rcr->controller_guid);
	SET_LONG_WORD(pkt->listener_guid, rcr->listener_guid);
	SET_LONG_WORD(pkt->talker_guid, rcr->talker_guid);
	HTON_U32(pkt->default_format, rcr->default_format);
	HTON_U16(pkt->talker_unique_id, rcr->talker_unique_id);
	HTON_U16(pkt->listener_unique_id, rcr->listener_unique_id);
	HTON_U16(pkt->connection_count, rcr->connection_count);
	HTON_U16(pkt->sequence_id, rcr->sequence_id);
	HTON_U16(pkt->flags, rcr->flags);
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
	mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ACMP_PACKET_SIZE, ETH_BROADCAST);

	// set up inflight command
}

static void acmp_listener_tx_response(unsigned type, unsigned error_code, chanend c_tx)
{
	acmp_listener_rcvd_cmd_resp.message_type = type;
	acmp_listener_rcvd_cmd_resp.status = error_code;
	avb_1722_1_create_acmp_packet(&acmp_listener_rcvd_cmd_resp);
	mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ACMP_PACKET_SIZE, ETH_BROADCAST);
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
	mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ACMP_PACKET_SIZE, ETH_BROADCAST);
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
	store->talker_unique_id = NTOH_U16(pkt->talker_unique_id);
	store->listener_unique_id = NTOH_U16(pkt->listener_unique_id);
	store->connection_count = NTOH_U16(pkt->connection_count);
	store->sequence_id = NTOH_U16(pkt->sequence_id);
	store->flags = NTOH_U16(pkt->flags);
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

avb_status_t process_avb_1722_1_acmp_packet(avb_1722_1_acmp_packet_t* pkt, chanend c_tx)
{
	unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

	switch (message_type)
	{
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

avb_status_t avb_1722_1_acmp_talker_periodic(chanend c_tx)
{
	switch (acmp_talker_state)
	{
		case ACMP_TALKER_IDLE:
		case ACMP_TALKER_WAITING:
		case ACMP_TALKER_WAITING_FOR_CONNECT:
		case ACMP_TALKER_WAITING_FOR_DISCONNECT:
			return AVB_NO_STATUS;

		case ACMP_TALKER_CONNECT:
			if (!acmp_talker_valid_talker_unique())
			{
				acmp_talker_tx_response(ACMP_CMD_CONNECT_TX_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
				acmp_talker_state = ACMP_TALKER_WAITING;
				return AVB_NO_STATUS;
			}
			else
			{
				acmp_talker_state = ACMP_TALKER_WAITING_FOR_CONNECT;
				return AVB_1722_1_CONNECT_TALKER;
			}
			break;
		case ACMP_TALKER_DISCONNECT:
			if (!acmp_talker_valid_talker_unique())
			{
				acmp_talker_tx_response(ACMP_CMD_DISCONNECT_TX_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
				acmp_talker_state = ACMP_TALKER_WAITING;
				return AVB_NO_STATUS;
			}
			else
			{
				acmp_talker_state = ACMP_TALKER_WAITING_FOR_DISCONNECT;
				return AVB_1722_1_DISCONNECT_TALKER;
			}
			break;
		case ACMP_TALKER_GET_STATE:
			if (!acmp_talker_valid_talker_unique())
			{
				acmp_talker_tx_response(ACMP_CMD_GET_TX_STATE_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
			}
			else
			{
				acmp_talker_tx_response(ACMP_CMD_GET_TX_STATE_RESPONSE, acmp_talker_get_state(), c_tx);
			}
			acmp_talker_state = ACMP_TALKER_WAITING;
			return AVB_NO_STATUS;
		case ACMP_TALKER_GET_CONNECTION:
			if (!acmp_talker_valid_talker_unique())
			{
				acmp_talker_tx_response(ACMP_CMD_GET_TX_CONNECTION_RESPONSE, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
			}
			else
			{
				acmp_talker_tx_response(ACMP_CMD_GET_TX_CONNECTION_RESPONSE, acmp_talker_get_connection(), c_tx);
			}
			acmp_talker_state = ACMP_TALKER_WAITING;
			return AVB_NO_STATUS;
	}

	return AVB_NO_STATUS;
}

avb_status_t avb_1722_1_acmp_listener_periodic(chanend c_tx)
{
	switch (acmp_listener_state)
	{
		case ACMP_LISTENER_IDLE:
		case ACMP_LISTENER_WAITING_FOR_CONNECT:
		case ACMP_LISTENER_WAITING_FOR_DISCONNECT:
			break;
		case ACMP_LISTENER_WAITING:
			acmp_listener_check_inflight_command_timeouts();
			break;
		case ACMP_LISTENER_CONNECT_RX_COMMAND:
		{
			if (!acmp_listener_valid_listener_unique())
			{
				acmp_listener_tx_response(ACMP_CMD_CONNECT_RX_RESPONSE, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
			}
			else
			{
				if (acmp_listener_is_connected())
				{
					acmp_listener_tx_response(ACMP_CMD_CONNECT_RX_RESPONSE, ACMP_STATUS_LISTENER_EXCLUSIVE, c_tx);
				}
				else
				{
					acmp_listener_tx_command(ACMP_CMD_CONNECT_TX_COMMAND, 2000, c_tx);
				}
			}
			acmp_listener_state = ACMP_LISTENER_WAITING;
			break;
		}
		case ACMP_LISTENER_DISCONNECT_RX_COMMAND:
		{
			if (!acmp_listener_valid_listener_unique())
			{
				acmp_listener_tx_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
			}
			else
			{
				if (acmp_listener_is_connected())
				{
					acmp_listener_tx_command(ACMP_CMD_DISCONNECT_TX_COMMAND, 200, c_tx);
				}
				else
				{
					acmp_listener_tx_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, ACMP_STATUS_NOT_CONNECTED, c_tx);
				}
			}
			acmp_listener_state = ACMP_LISTENER_WAITING;
			break;
		}
		case ACMP_LISTENER_CONNECT_TX_RESPONSE:
		{
			if (acmp_listener_valid_listener_unique())
			{
				acmp_listener_tx_response(ACMP_CMD_CONNECT_RX_RESPONSE, 0, c_tx);
				acmp_listener_remove_inflight();
				acmp_listener_state = ACMP_LISTENER_WAITING_FOR_CONNECT;
				return AVB_1722_1_CONNECT_LISTENER;
			}
			break;
		}
		case ACMP_LISTENER_DISCONNECT_TX_RESPONSE:
		{
			if (acmp_listener_valid_listener_unique())
			{
				acmp_listener_tx_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, 0, c_tx);
				acmp_listener_remove_inflight();
				acmp_listener_state = ACMP_LISTENER_WAITING_FOR_DISCONNECT;
				return AVB_1722_1_DISCONNECT_LISTENER;
			}
			break;
		}
		case ACMP_LISTENER_GET_STATE:
		{
			if (!acmp_listener_valid_listener_unique())
			{
				acmp_listener_tx_response(ACMP_CMD_GET_RX_STATE_RESPONSE, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
			}
			else
			{
				acmp_listener_tx_response(ACMP_CMD_GET_RX_STATE_RESPONSE, acmp_listener_get_state(), c_tx);
			}
			acmp_listener_state = ACMP_LISTENER_WAITING;
			break;
		}
	}

	return AVB_NO_STATUS;
}

unsigned avb_1722_1_acmp_get_talker_connection_info(short *talker)
{
	*talker = acmp_talker_rcvd_cmd_resp.talker_unique_id;
	return 1;
}

unsigned avb_1722_1_acmp_get_listener_connection_info(short *listener, unsigned char address[6], unsigned streamId[2], unsigned* vlan)
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

void avb_1722_1_talker_set_mac_address(unsigned talker_unique_id, unsigned char macaddr[])
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

avb_1722_1_acmp_status_t avb_1722_1_acmp_configure_source(unsigned talker_unique_id, unsigned int default_format)
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

avb_1722_1_acmp_status_t avb_1722_1_acmp_configure_sink(unsigned listener_unique_id, unsigned int default_format)
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
