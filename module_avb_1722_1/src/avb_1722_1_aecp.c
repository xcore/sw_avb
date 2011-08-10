#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include <string.h>
#include "xccompat.h"

extern unsigned int avb_1722_1_buf[];
extern guid_t my_guid;
extern unsigned char my_mac_addr[6];

// The length of each parameter type defined by AECP
static const unsigned char avb_1722_1_aecp_parameter_length[] = {
0, 2, 2, 8, 2, 2, 2, 4, 1, 1, 2, 2, 4, 4, 4, 6, 8, 8, 8, 8, 8,
4, 8, 6, 4, 16, 8, 16, 4, 32, 64, 128, 4, 4, 4, 4, 4, 1, 1, 184,
4, 4, 8, 16, 4, 4, 8, 8, 142, 8, 84, 84, 2, 2, 38, 40, 40, 40,
42, 46, 46, 52, 52, 52, 64, 64, 64, 38, 12
};

static const unsigned char avb_1722_1_aecp_dest_addr[6] = AVB_1722_1_AECP_DEST_MAC;

static unsigned char* avb_1722_1_create_aecp_packet(int message_type, avb_1722_1_aecp_packet_t* cmd_pkt)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + 1);

	avb_1722_1_create_1722_1_header(avb_1722_1_aecp_dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, message_type, 0, 40, hdr);

	memcpy(pkt->target_guid, cmd_pkt->target_guid, (pkt->data.payload - pkt->target_guid));

	return pkt->data.payload;
}

avb_status_t process_avb_1722_1_aecp_packet(avb_1722_1_aecp_packet_t* pkt, chanend c_tx)
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
