#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include <string.h>
#include "xccompat.h"

extern unsigned int avb_1722_1_buf[];
extern guid_t my_guid;
extern unsigned char my_mac_addr[6];

static const unsigned char avb_1722_1_aecp_dest_addr[6] = AVB_1722_1_AECP_DEST_MAC;

static unsigned char *avb_1722_1_create_aecp_packet(int message_type, avb_1722_1_aecp_packet_t* cmd_pkt)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

	avb_1722_1_create_1722_1_header(avb_1722_1_aecp_dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, message_type, 0, 40, hdr);

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

avb_status_t process_avb_1722_1_aecp_packet(avb_1722_1_aecp_packet_t *pkt, chanend c_tx)
{
	int message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));
	if (compare_guid(pkt->target_guid, &my_guid)==0) return AVB_1722_1_OK;

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
	return AVB_1722_1_OK;
}
