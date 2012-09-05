#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include <string.h>
#include <print.h>
#include "xccompat.h"

#if AVB_1722_1_USE_AVC
#include "avc_commands.h"
#endif
#if AVB_1722_1_AEM_ENABLED
#include "aem_descriptor_types.h"
#include "aem_descriptors.h"
#endif

extern unsigned int avb_1722_1_buf[];
extern guid_t my_guid;
extern unsigned char my_mac_addr[6];

static int global_entity_acquired;

// Called on startup to initialise certain static descriptor fields
void avb_1722_1_aem_descriptors_init()
{
	// entity_guid in Entity Descriptor
	for (int i=0; i < 8; i++)
	{
		desc_entity[4+i] = my_guid.c[7-i];
	}

	for (int i=0; i < 6; i++)
	{
		// mac_address in AVB Interface Descriptor
		desc_avb_interface_0[70+i] = my_mac_addr[i];
		// clock_source_identifier in clock source descriptor
		desc_clock_source_0[74+i] = my_mac_addr[i];
	}

	// TODO: Should be stored centrally, possibly query PTP for ID per interface
	desc_avb_interface_0[78+0] = my_mac_addr[0];
	desc_avb_interface_0[78+1] = my_mac_addr[1];
	desc_avb_interface_0[78+2] = my_mac_addr[2];
	desc_avb_interface_0[78+3] = 0xff;
	desc_avb_interface_0[78+4] = 0xfe;
	desc_avb_interface_0[78+5] = my_mac_addr[3];
	desc_avb_interface_0[78+6] = my_mac_addr[4];
	desc_avb_interface_0[78+7] = my_mac_addr[5];
	desc_avb_interface_0[78+8] = 0;
	desc_avb_interface_0[78+9] = 1;
}

#if 0
void avb_1722_1_aem_set_grandmaster_id(unsigned char as_grandmaster_id[])
{
#if AVB_1722_1_AEM_ENABLED
	for (int i=0; i < 8; i++)
	{
		// AVB Interface Descriptor
		// desc_avb_interface_0[78+i] = as_grandmaster_id[7-i];
	}
#endif
}
#endif

// TODO: Set available_index on entity descriptor tx

static unsigned char *avb_1722_1_create_aecp_packet(unsigned char dest_addr[6], int message_type, char status, unsigned int data_len, avb_1722_1_aecp_packet_t* cmd_pkt)
{
	struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
	avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

	avb_1722_1_create_1722_1_header(dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, message_type, status, data_len, hdr);

	// FIXME: This memcpy is terrible. Blindly copying the maximum size payload?
	memcpy(pkt->target_guid, cmd_pkt->target_guid, (pkt->data.payload - pkt->target_guid));

	return pkt->data.payload;
}

static int create_aem_read_descriptor_response(unsigned short read_type, unsigned short read_id, unsigned char dest_addr[6], avb_1722_1_aecp_packet_t *pkt)
{
	int desc_size_bytes = 0, i = 0;
	unsigned char *descriptor;
	int found_descriptor = 0;

	/* Search for the descriptor */
	while (aem_descriptor_list[i] <= read_type)
	{
		int num_descriptors = aem_descriptor_list[i+1];

		if (aem_descriptor_list[i] == read_type)
		{
			for (int j=0, k=2; j < num_descriptors; j++, k += 2)
			{
				desc_size_bytes = aem_descriptor_list[i+k];
				descriptor = (unsigned char *)aem_descriptor_list[i+k+1];
				
				#if 0
				// Generate audio clusters on the fly (to reduce memory)
				if (read_type == AEM_AUDIO_CLUSTER_TYPE)
				{
					char chan_id;
					// The descriptor id is also the channel number
					descriptor[3] = (unsigned char)read_id;
					if (read_id >= AVB_NUM_MEDIA_INPUTS)
					{
						// This is nasty for non power of 2 inputs - will do a DIV!
						chan_id = (char)read_id % AVB_NUM_MEDIA_INPUTS;
					}
					else
					{
						chan_id = (char)read_id;
					}
					descriptor[19] = chan_id + 0x30;
					found_descriptor = 1;
				}
				else
				#endif
				{
					// TODO: Write macros for descriptor fields (or cast to structs??)
					if (( ((unsigned)descriptor[2] << 8) | ((unsigned)descriptor[3]) ) == read_id)
					{
						found_descriptor = 1;
						break;
					}
				}
			}

		}

		i += ((num_descriptors*2)+2);
		if (i >= (sizeof(aem_descriptor_list)>>2)) break;
	}


	if (found_descriptor)
	{
		int packet_size = sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+24+desc_size_bytes;

		if (packet_size < 64) packet_size = 64;

		avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_packet(dest_addr, AECP_CMD_AEM_RESPONSE, AECP_AEM_STATUS_SUCCESS, desc_size_bytes+16, pkt);

		memcpy(aem, pkt->data.payload, 48);
		memcpy(&(aem->payload[4]), descriptor, desc_size_bytes+40);

		return packet_size;
	}
	else // Descriptor not found, send NO_SUCH_DESCRIPTOR reply
	{
		int packet_size = sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+52;

		avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_packet(dest_addr, AECP_CMD_AEM_COMMAND, AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR, 40, pkt);

		memcpy(aem, pkt->data.payload, 52);

		return packet_size;
	}
}

static void process_avb_1722_1_aecp_aem_msg(avb_1722_1_aecp_packet_t *pkt, unsigned char dest_addr[6], int num_pkt_bytes, chanend c_tx)
{
	avb_1722_1_aecp_aem_msg_t *msg = &(pkt->data.avdecc);
	unsigned char u_flag = AEM_MSG_U_FLAG(msg);
	unsigned short command_type = AEM_MSG_COMMAND_TYPE(msg);

	// Check/do something with the u_flag?

	switch (command_type)
	{
		case AECP_AEM_CMD_READ_DESCRIPTOR:
		{
			unsigned short desc_read_type, desc_read_id;
			int num_tx_bytes;
			avb_1722_1_aem_read_descriptor_command_t *cmd = (avb_1722_1_aem_read_descriptor_command_t *)msg;

			desc_read_type = NTOH_U16(cmd->descriptor_type);
			desc_read_id = NTOH_U16(cmd->descriptor_id);

			printstr("READ_DESCRIPTOR: "); printint(desc_read_type); printchar(','); printintln(desc_read_id);

			num_tx_bytes = create_aem_read_descriptor_response(desc_read_type, desc_read_id, dest_addr, pkt);

			mac_tx(c_tx, avb_1722_1_buf, num_tx_bytes, ETH_BROADCAST);

			break;
		}
		
		/*
		case AECP_AEM_CMD_LOCK_ENTITY:
		{
			avb_1722_1_aem_lock_entity_command_t *cmd = (avb_1722_1_aem_lock_entity_command_t *)msg;

			if (NTOH_U32(cmd->flags) == 1) // Unlock
			{
				global_entity_locked = 0;
			}

			if (!global_entity_locked)
			{
				for (int i=0; i < 8; i++)
				{
					cmd->locked_guid[i] = pkt->controller_guid[i];
				}
				avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_packet(dest_addr, AECP_CMD_AEM_RESPONSE, AECP_AEM_STATUS_SUCCESS, 20, pkt);
				global_entity_locked = 1;

				mac_tx(c_tx, avb_1722_1_buf, 68, ETH_BROADCAST);
			}
			
			break;
		}
		*/
		case AECP_AEM_CMD_ACQUIRE_ENTITY:
		{
			/*
			avb_1722_1_aem_acquire_entity_command_t *cmd = (avb_1722_1_aem_acquire_entity_command_t *)msg;

			// if (!global_entity_acquired)
			{
				for (int i=0; i < 8; i++)
				{
					cmd->owner_guid[i] = pkt->controller_guid[i];
				}

				avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_packet(dest_addr, AECP_CMD_AEM_RESPONSE, AECP_AEM_STATUS_SUCCESS, 38, pkt);
				memcpy(aem, pkt->data.payload, 18);

				global_entity_acquired = 1;

				mac_tx(c_tx, avb_1722_1_buf, 64, ETH_BROADCAST);
			}
			*/

			break;
		}
		
		default:
		{
			// AECP_AEM_STATUS_NOT_IMPLEMENTED
			return;
		}
	}
}

void process_avb_1722_1_aecp_packet(unsigned char dest_addr[6], avb_1722_1_aecp_packet_t *pkt, int num_pkt_bytes, chanend c_tx)
{
	int message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

	if (compare_guid(pkt->target_guid, &my_guid)==0) return;

	switch (message_type)
	{
		case AECP_CMD_AEM_COMMAND:
		{
#if AVB_1722_1_AEM_ENABLED
			process_avb_1722_1_aecp_aem_msg(pkt, dest_addr, num_pkt_bytes, c_tx);
#endif
			break;
		}
		case AECP_CMD_ADDRESS_ACCESS_COMMAND:
		{
			break;
		}
		case AECP_CMD_AVC_COMMAND:
		{
#if AVB_1722_1_USE_AVC
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
