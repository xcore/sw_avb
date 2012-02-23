#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include <string.h>
#include "xccompat.h"

#if AVB_1722_1_USE_AVC
#include "avc_commands.h"
#endif
extern unsigned int avb_1722_1_buf[];
extern guid_t my_guid;
extern unsigned char my_mac_addr[6];


static unsigned char *avb_1722_1_create_aecp_packet(unsigned char dest_addr[6], int message_type, avb_1722_1_aecp_packet_t* cmd_pkt)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

	avb_1722_1_create_1722_1_header(dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, message_type, 0, 40, hdr);

	memcpy(pkt->target_guid, cmd_pkt->target_guid, (pkt->data.payload - pkt->target_guid));

	return pkt->data.payload;
}

static void process_avb_1722_1_aecp_avdecc_msg(avb_1722_1_aecp_avdecc_msg_t *msg)
{
	int mode_code = GET_AECP_AVDECC_MSG_MODE(msg);
	int length = GET_AECP_AVDECC_MSG_LENGTH(msg);

	switch (mode_code)
	{
		case AECP_MODE_AVDECC_MSG_GET:
		{
			break;
		}
		case AECP_MODE_AVDECC_MSG_SET:
		{
			break;
		}
		default:
		{
			return;
		}
	}
}

void process_avb_1722_1_aecp_packet(avb_status_t *status, unsigned char dest_addr[6], avb_1722_1_aecp_packet_t *pkt, chanend c_tx)
{
	int message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

	status->type = AVB_1722_1;
	status->info.a1722_1.msg = AVB_1722_1_OK;

	if (compare_guid(pkt->target_guid, &my_guid)==0) return;

	switch (message_type)
	{
		case AECP_CMD_AVDECC_MSG_COMMAND:
		{
			avb_1722_1_aecp_avdecc_msg_t *msg = &(pkt->data.avdecc);
			process_avb_1722_1_aecp_avdecc_msg(msg);
			break;
		}
		case AECP_CMD_ADDRESS_ACCESS_COMMAND:
		{
			break;
		}
		case AECP_CMD_AVC_COMMAND:
		{
# if AVB_1722_1_USE_AVC
			uint32_t length = 0 ;
			struct ethernet_hdr_t *eth_hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
			avb_1722_1_packet_header_t *pkt_hdr = (avb_1722_1_packet_header_t*) (eth_hdr + 1);
			avb_1722_1_aecp_avc_t* payload = (avb_1722_1_aecp_avc_t*)avb_1722_1_create_aecp_packet(dest_addr, AECP_CMD_AVC_RESPONSE, pkt);
			bcopy(pkt->data.payload, payload, 514);
			length = ((uint32_t)payload->avc_length[0] << 8) | payload->avc_length[1];
			processAVCCommand(payload->avc_command_response, &length);
			payload->avc_length[0] = (length >> 8) & 0xff;
			payload->avc_length[1] = length & 0xff;
			SET_1722_1_DATALENGTH(pkt_hdr, length+12);
			mac_tx(c_tx, avb_1722_1_buf, sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+36+length, ETH_BROADCAST);
#endif
			break;
		}
		case AECP_CMD_VENDOR_UNIQUE_COMMAND:
		{
			break;
		}
		case AECP_CMD_EXTENDED_COMMAND:
		{
			break;
		}
		default:
			// This node is not expecting a response
			break;
	}
	return;
}
