#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include "misc_timer.h"
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

static avb_timer aecp_aem_lock_timer;

static enum {
  AEM_ENTITY_NOT_ACQUIRED,
  AEM_ENTITY_ACQUIRED,
  AEM_ENTITY_ACQUIRED_AND_PERSISTENT,
  AEM_ENTITY_ACQUIRED_BUT_PENDING = 0x80000001
} entity_acquired_status = AEM_ENTITY_NOT_ACQUIRED;

static guid_t pending_controller_guid;
static guid_t acquired_controller_guid;
static unsigned char acquired_controller_mac[6];


static enum { 
    AECP_AEM_IDLE,
    AECP_AEM_WAITING,
    AECP_AEM_CONTROLLER_AVAILABLE_TIMEOUT,
    AECP_AEM_LOCK_TIMEOUT
} aecp_aem_state = AECP_AEM_IDLE;

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

void avb_1722_1_aecp_aem_init()
{
  avb_1722_1_aem_descriptors_init();
  init_avb_timer(&aecp_aem_lock_timer, 100);

  aecp_aem_state = AECP_AEM_WAITING;
}

// TODO: Set available_index on entity descriptor tx


static void avb_1722_1_create_controller_available_packet(unsigned char dest_addr[6], guid_t target_guid)
{
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

  avb_1722_1_create_1722_1_header(dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, AECP_CMD_AEM_COMMAND, AECP_AEM_STATUS_SUCCESS, 10, hdr);

  SET_LONG_WORD(pkt->target_guid, target_guid);
  for (int i=0; i < 6; i++) pkt->controller_guid[i] = 0;
  HTON_U16(pkt->sequence_id, 0);
}

static unsigned char *avb_1722_1_create_aecp_response_header(unsigned char dest_addr[6], char status, unsigned int data_len, avb_1722_1_aecp_packet_t* cmd_pkt)
{
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

  avb_1722_1_create_1722_1_header(dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, AECP_CMD_AEM_RESPONSE, status, data_len, hdr);

  // Copy the target guid, controller guid and sequence ID into the response header
  memcpy(pkt->target_guid, cmd_pkt->target_guid, (pkt->data.payload - pkt->target_guid));

  return pkt->data.payload;
}

/* data_len: number of bytes of command_specific_data (9.2.1.2 Figure 9.2)
*/
static void avb_1722_1_create_aecp_aem_response(unsigned char src_addr[6], unsigned char status, unsigned int command_data_len, avb_1722_1_aecp_packet_t* cmd_pkt)
{
  /* 9.2.1.1.7: "control_data_length field for AECP is the number of octets following the target_guid,
  but is limited to a maximum of 524" 

  control_data_length = payload_specific_data + sequence_id + controller_guid
  payload_specific_data (for AEM) = command_specific_data + command_type + u

  = command_specific_data + 2 + 2 + 8
  */
  avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_response_header(src_addr, status, command_data_len+12, cmd_pkt);

  /* Copy payload_specific_data into the response */
  memcpy(aem, cmd_pkt->data.payload, command_data_len + 2);
}

static int create_aem_read_descriptor_response(unsigned short read_type, unsigned short read_id, unsigned char src_addr[6], avb_1722_1_aecp_packet_t *pkt)
{
  int desc_size_bytes = 0, i = 0;
  unsigned char *descriptor;
  int found_descriptor = 0;

#if AEM_GENERATE_CLUSTERS_MAP_ON_FLY
  // Generate audio clusters on the fly (to reduce memory)
  if (read_type == AEM_AUDIO_CLUSTER_TYPE)
  {
    if (read_id < (AVB_NUM_MEDIA_OUTPUTS+AVB_NUM_MEDIA_INPUTS))
    {
      char chan_id;
      descriptor = &desc_audio_cluster_template[0];
      // The descriptor id is also the channel number
      descriptor[3] = (unsigned char)read_id;
      if (read_id < AVB_NUM_MEDIA_OUTPUTS)
      {
        chan_id = (char)read_id;
        strcpy((char*)&descriptor[4], "Output ");
        descriptor[11] = chan_id + 0x30;
      }
      else
      {
        chan_id = (char)read_id - AVB_NUM_MEDIA_OUTPUTS;
        strcpy((char*)&descriptor[4], "Input ");
        descriptor[10] = chan_id + 0x30;
        descriptor[11] = '\0'; // NUL
      }
      desc_size_bytes = sizeof(desc_audio_cluster_template);
      found_descriptor = 1;
    }
  }
  else if (read_type == AEM_AUDIO_MAP_TYPE)
  {
    if (read_id < (AVB_NUM_SINKS+AVB_NUM_SOURCES))
    {
      int num_mappings = (read_id == 0) ? AVB_NUM_MEDIA_OUTPUTS : AVB_NUM_MEDIA_INPUTS;

      /* Since the map descriptors aren't constant size, unlike the clusters, and 
       * dependent on the number of channels, we don't use a template */
      
      struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
      avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
      avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)(pkt->data.payload);
      unsigned char *pktptr = (unsigned char *)&(aem->command.read_descriptor_resp.descriptor);

      desc_size_bytes = 8+(num_mappings*8);

      memset(pktptr, 0, desc_size_bytes);
 
      pktptr[0*2+1] = AEM_AUDIO_MAP_TYPE;
      pktptr[1*2+1] = read_id;
      pktptr[2*2+1] = 8;
      pktptr[3*2+1] = num_mappings;

      for (int i=0; i < num_mappings; i++)
      {
        pktptr[11+(8*i)] = i;
        pktptr[13+(8*i)] = i;
      }

      found_descriptor = 2; // 2 signifies do not copy descriptor below
    }
  }
  else
#endif
  {
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
          
          // TODO: Write macros for descriptor fields (or cast to structs??)
          if (( ((unsigned)descriptor[2] << 8) | ((unsigned)descriptor[3]) ) == read_id)
          {
            found_descriptor = 1;
            break;
          }
        }

      }

      i += ((num_descriptors*2)+2);
      if (i >= (sizeof(aem_descriptor_list)>>2)) break;
    }
  }


  if (found_descriptor)
  {
    int packet_size = sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+24+desc_size_bytes;

    if (packet_size < 64) packet_size = 64;

    avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_response_header(src_addr, AECP_AEM_STATUS_SUCCESS, desc_size_bytes+16, pkt);

    memcpy(aem, pkt->data.payload, 6);
    if (found_descriptor < 2) memcpy(&(aem->command.read_descriptor_resp.descriptor), descriptor, desc_size_bytes+40);

    return packet_size;
  }
  else // Descriptor not found, send NO_SUCH_DESCRIPTOR reply
  {
    int packet_size = sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+20+sizeof(avb_1722_1_aem_read_descriptor_command_t);

    avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_response_header(src_addr, AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR, 40, pkt);

    memcpy(aem, pkt->data.payload, 20+sizeof(avb_1722_1_aem_read_descriptor_command_t));

    return packet_size;
  }
}

static void process_avb_1722_1_aecp_aem_msg(avb_1722_1_aecp_packet_t *pkt, unsigned char src_addr[6], int message_type, int num_pkt_bytes, chanend c_tx)
{
  avb_1722_1_aecp_aem_msg_t *aem_msg = &(pkt->data.aem);
  unsigned char u_flag = AEM_MSG_U_FLAG(aem_msg);
  unsigned short command_type = AEM_MSG_COMMAND_TYPE(aem_msg);

  // Check/do something with the u_flag?

  if (message_type == AECP_CMD_AEM_COMMAND)
  {
    switch (command_type)
    {
      case AECP_AEM_CMD_ACQUIRE_ENTITY: // Long term exclusive control of the entity
      {
        unsigned char status;
        unsigned short desc_type, desc_id;
        avb_1722_1_aem_acquire_entity_command_t *cmd = (avb_1722_1_aem_acquire_entity_command_t *)(pkt->data.aem.command.payload);

        desc_type = NTOH_U16(cmd->descriptor_type);
        desc_id = NTOH_U16(cmd->descriptor_id);

        if (desc_type != AEM_ENTITY_TYPE || desc_id != 0)
        {
          status = AECP_AEM_STATUS_NOT_SUPPORTED;
        }
        else 
        {
          if (entity_acquired_status == AEM_ENTITY_NOT_ACQUIRED)
          {
            // Handled below
          }
          else if ((entity_acquired_status == AEM_ENTITY_ACQUIRED_BUT_PENDING) &&
              (compare_guid(pkt->controller_guid, &pending_controller_guid)))
          {
            /* The CONTROLLER_AVAILABLE sent to the current controller timed out */
            /* Acquire new controller because the old has gone away */
            entity_acquired_status = AEM_ENTITY_NOT_ACQUIRED; // Is set to ACQUIRED below
            pending_controller_guid.l = 0;
          }
          else if (entity_acquired_status == AEM_ENTITY_ACQUIRED ||
                  entity_acquired_status == AEM_ENTITY_ACQUIRED_AND_PERSISTENT)
          {
            if (!compare_guid(pkt->controller_guid, &acquired_controller_guid))
            {
              if ((entity_acquired_status == AEM_ENTITY_ACQUIRED) &&
                  (!compare_guid(pkt->controller_guid, &pending_controller_guid)))
              {
                /* If the Entity has been acquired by another Controller then the Entity sends a CONTROLLER_AVAILABLE
                command to the currently acquired Controller to verify that the Controller is still present. */

                avb_1722_1_create_controller_available_packet(acquired_controller_mac, acquired_controller_guid);
                mac_tx(c_tx, avb_1722_1_buf, 64, ETH_BROADCAST);

                // Set a flag to indicate that the current acquisition is pending
                entity_acquired_status = AEM_ENTITY_ACQUIRED_BUT_PENDING;

                for(int i=0; i < 8; i++) pending_controller_guid.c[i] = pkt->controller_guid[i];

                break;
              }
              else
              {
                /* Current acquired Controller is persistent or the CONTROLLER_AVAILABLE timed out. 
                The Entity returns an ENTITY_ACQUIRED response immediately to any other Controller. */
                status = AECP_AEM_STATUS_ENTITY_ACQUIRED;
                /* Reset the pending controller GUID field */
                pending_controller_guid.l = 0;
              }
            }
            else // The controller has already acquired this entity
            {
              status = AECP_AEM_STATUS_SUCCESS;
            }
          }
          else
          {
            // Ignore and don't send a response
            break;
          }

          if (entity_acquired_status == AEM_ENTITY_NOT_ACQUIRED)
          {
            if (AEM_ACQUIRE_ENTITY_PERSISTENT_FLAG(cmd))
            {
              entity_acquired_status = AEM_ENTITY_ACQUIRED_AND_PERSISTENT;
            }
            else
            {
              entity_acquired_status = AEM_ENTITY_ACQUIRED;
            }
            
            for(int i=0; i < 8; i++)
            {
              acquired_controller_guid.c[i] = pkt->controller_guid[i];
              acquired_controller_mac[i] = src_addr[i];
            }
            status = AECP_AEM_STATUS_SUCCESS;
          }
        }
        
        // TODO: Release
        printstr("1722.1 Controller ");

        for (int i=0; i < 8; i++)
        {
          cmd->owner_guid[i] = acquired_controller_guid.c[i];
          printhex(acquired_controller_guid.c[i]);
        }
        printstrln(" acquired entity");

        avb_1722_1_create_aecp_aem_response(src_addr, status, sizeof(avb_1722_1_aem_acquire_entity_command_t), pkt);

        mac_tx(c_tx, avb_1722_1_buf, 64, ETH_BROADCAST);

        break;
      }
      case AECP_AEM_CMD_LOCK_ENTITY: // Atomic operation on the entity
      {
        avb_1722_1_aem_lock_entity_command_t *cmd = (avb_1722_1_aem_lock_entity_command_t *)(pkt->data.aem.command.payload);

        /* Pseudo:
          if (global_entity_acquired)
            if (global_controller_guid != this_controller_guid)
              send ENTITY_ACQUIRED
            else 
              send LOCK_ENTITY response with locked_guid set to global_controller_guid
          else
            send LOCK_ENTITY response with locked_guid set to global_controller_guid
            set 60s timeout of lock
            NOTE >>> Can i use global_controller_guid for the locked guid?

        */

        /*
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
          avb_1722_1_aecp_aem_msg_t *aem =(avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_packet(src_addr, AECP_CMD_AEM_RESPONSE, AECP_AEM_STATUS_SUCCESS, 20, pkt);
          global_entity_locked = 1;

          mac_tx(c_tx, avb_1722_1_buf, 68, ETH_BROADCAST);
        }
        */
        
        break;
      }
      case AECP_AEM_CMD_READ_DESCRIPTOR:
      {
        unsigned short desc_read_type, desc_read_id;
        int num_tx_bytes;

        desc_read_type = NTOH_U16(aem_msg->command.read_descriptor_cmd.descriptor_type);
        desc_read_id = NTOH_U16(aem_msg->command.read_descriptor_cmd.descriptor_id);

        printstr("READ_DESCRIPTOR: "); printint(desc_read_type); printchar(','); printintln(desc_read_id);

        num_tx_bytes = create_aem_read_descriptor_response(desc_read_type, desc_read_id, src_addr, pkt);

        mac_tx(c_tx, avb_1722_1_buf, num_tx_bytes, ETH_BROADCAST);

        break;
      }
      // TODO: ENTITY_AVAILABLE
      default:
      {
        // AECP_AEM_STATUS_NOT_IMPLEMENTED
        return;
      }
    }
  }
  else // AECP_CMD_AEM_RESPONSE
  {
      switch (command_type)
      {
        case AECP_AEM_CMD_CONTROLLER_AVAILABLE:
        {
          if ((entity_acquired_status != AEM_ENTITY_ACQUIRED_BUT_PENDING) ||
              (!compare_guid(pkt->controller_guid, &pending_controller_guid)))
          {
            // Not interested... ignore
            break;
          }
          /* The acquired controller is still available.
           * We mark the entity status as acquired so that the ACQUIRE_ENTITY retry is responded to with 
           * the correct ENTITY_ACQUIRED status code */
          entity_acquired_status = AEM_ENTITY_ACQUIRED;
          break;
        }
        default:
          break;
      }
  }
}

void process_avb_1722_1_aecp_packet(unsigned char src_addr[6], avb_1722_1_aecp_packet_t *pkt, int num_pkt_bytes, chanend c_tx)
{
  int message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

  if (compare_guid(pkt->target_guid, &my_guid)==0) return;

  switch (message_type)
  {
    case AECP_CMD_AEM_COMMAND:
    case AECP_CMD_AEM_RESPONSE:
    {
#if AVB_1722_1_AEM_ENABLED
      process_avb_1722_1_aecp_aem_msg(pkt, src_addr, message_type, num_pkt_bytes, c_tx);
#endif
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
  return;
}

void avb_1722_1_aecp_aem_periodic(chanend c_tx)
{
  switch (aecp_aem_state)
  {
    case AECP_AEM_IDLE:
    case AECP_AEM_WAITING:
    {
      break;
    }
    case AECP_AEM_CONTROLLER_AVAILABLE_TIMEOUT:
    {
      break;
    }
    case AECP_AEM_LOCK_TIMEOUT:
    {
      break;
    }
  }

}