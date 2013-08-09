#include "avb.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include "misc_timer.h"
#include "avb_srp_pdu.h"
#include <string.h>
#include <print.h>
#include "simple_printf.h"
#include "xccompat.h"
#include "avb_1722_1.h"

#if AVB_1722_1_USE_AVC
#include "avc_commands.h"
#endif
#if AVB_1722_1_AEM_ENABLED
#include "aem_descriptor_types.h"
#include "aem_descriptors.h"
#endif

extern unsigned int avb_1722_1_buf[AVB_1722_1_PACKET_SIZE_WORDS];
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

  set_64(pkt->target_guid, target_guid.c);
  for (int i=0; i < 6; i++) pkt->controller_guid[i] = 0;
  hton_16(pkt->sequence_id, 0);
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

static int sampling_rate_from_sfc(int sfc)
{
  switch (sfc)
  {
    case 0: return 32000;
    case 1: return 44100;
    case 2: return 48000;
    case 3: return 88200;
    case 4: return 96000;
    case 5: return 176400;
    case 6: return 192000;
    default: return 0;
  }
}

static int sfc_from_sampling_rate(int rate)
{
  switch (rate)
  {
    case 32000: return 0;
    case 44100: return 1;
    case 48000: return 2;
    case 88200: return 3;
    case 96000: return 4;
    case 176400: return 5;
    case 192000: return 6;
    default: return 0;
  }
}

static void process_aem_cmd_getset_stream_format(avb_1722_1_aecp_packet_t *pkt, unsigned char *status, unsigned short command_type)
{
  avb_1722_1_aem_getset_stream_format_t *cmd = (avb_1722_1_aem_getset_stream_format_t *)(pkt->data.aem.command.payload);
  unsigned short stream_index = ntoh_16(cmd->descriptor_id);
  unsigned short desc_type = ntoh_16(cmd->descriptor_type);
  enum avb_stream_format_t format;
  int rate;
  int channels;


  if (command_type == AECP_AEM_CMD_GET_STREAM_FORMAT)
  {
    cmd->stream_format[0] = 0x00;
    cmd->stream_format[1] = 0xa0;
    cmd->stream_format[2] = 0x02;
    cmd->stream_format[4] = 0x40; // b[0], nb[1], reserved[2:]
    cmd->stream_format[5] = 0; // label_iec_60958_cnt
    cmd->stream_format[7] = 0; // label_midi_cnt[0:3], label_smptecnt[4:]

    if ((desc_type == AEM_STREAM_INPUT_TYPE) && get_avb_sink_format(stream_index, &format, &rate))
    {
      get_avb_sink_channels(stream_index, &channels);
    }
    else if ((desc_type == AEM_STREAM_OUTPUT_TYPE) && get_avb_source_format(stream_index, &format, &rate))
    {
      get_avb_source_channels(stream_index, &channels);
    }
    else *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;

    cmd->stream_format[2] = sfc_from_sampling_rate(rate); // 10.3.2 in 61883-6
    cmd->stream_format[3] = channels; // dbs
    cmd->stream_format[6] = channels; // label_mbla_cnt
  }
  else // AECP_AEM_CMD_SET_STREAM_FORMAT
  {
    format = AVB_SOURCE_FORMAT_MBLA_24BIT;
    rate = sampling_rate_from_sfc(cmd->stream_format[2]);
    channels = cmd->stream_format[6];

    if (desc_type == AEM_STREAM_INPUT_TYPE)
    {
      // Check if we are currently streaming
      enum avb_sink_state_t state;
      if (get_avb_sink_state(stream_index, &state))
      {
        if (state == AVB_SINK_STATE_DISABLED)
        {
          set_avb_sink_format(stream_index, format, rate);
          set_avb_sink_channels(stream_index, channels);
        }
        else *status = AECP_AEM_STATUS_STREAM_IS_RUNNING;
      }
      else *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;

    }
    else if ((desc_type == AEM_STREAM_OUTPUT_TYPE))
    {
      enum avb_source_state_t state;
      if (get_avb_source_state(stream_index, &state))
      {
        if (state == AVB_SOURCE_STATE_DISABLED)
        {
          set_avb_source_format(stream_index, format, rate);
          set_avb_source_channels(stream_index, channels);
        }
        else *status = AECP_AEM_STATUS_STREAM_IS_RUNNING;
      }
      else *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
    }
    else
    {
      // invalid
    }

  }

}

static void process_aem_cmd_getset_sampling_rate(avb_1722_1_aecp_packet_t *pkt, unsigned char *status, unsigned short command_type)
{
  avb_1722_1_aem_getset_sampling_rate_t *cmd = (avb_1722_1_aem_getset_sampling_rate_t *)(pkt->data.aem.command.payload);
  unsigned short media_clock_id = ntoh_16(cmd->descriptor_id);
  int rate;

  if (command_type == AECP_AEM_CMD_GET_SAMPLING_RATE)
  {
    if (get_device_media_clock_rate(media_clock_id, &rate))
    {
      hton_32(cmd->sampling_rate, rate);
      return;
    }
  }
  else // AECP_AEM_CMD_SET_SAMPLING_RATE
  {
    rate = ntoh_32(cmd->sampling_rate);

    if (set_device_media_clock_rate(media_clock_id, rate))
    {
      // Success
      return;
    }
  }

  *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
}

static void process_aem_cmd_getset_clock_source(avb_1722_1_aecp_packet_t *pkt, unsigned char *status, unsigned short command_type)
{
  avb_1722_1_aem_getset_clock_source_t *cmd = (avb_1722_1_aem_getset_clock_source_t *)(pkt->data.aem.command.payload);
  unsigned short media_clock_id = ntoh_16(cmd->descriptor_id);
  // The clock source descriptor's index corresponds to the clock type in our implementation
  enum device_media_clock_type_t source_index;

  if (command_type == AECP_AEM_CMD_GET_CLOCK_SOURCE)
  {
    if (get_device_media_clock_type(media_clock_id, &source_index))
    {
      hton_16(cmd->clock_source_index, source_index);
      return;
    }
  }
  else // AECP_AEM_CMD_SET_CLOCK_SOURCE
  {
    source_index = ntoh_16(cmd->clock_source_index);

    if (set_device_media_clock_type(media_clock_id, source_index))
    {
      // Success
      return;
    }
  }

  *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
}

static void process_aem_cmd_startstop_streaming(avb_1722_1_aecp_packet_t *pkt, unsigned char *status, unsigned short command_type)
{
  avb_1722_1_aem_startstop_streaming_t *cmd = (avb_1722_1_aem_startstop_streaming_t *)(pkt->data.aem.command.payload);
  unsigned short stream_index = ntoh_16(cmd->descriptor_id);
  unsigned short desc_type = ntoh_16(cmd->descriptor_type);

  if (desc_type == AEM_STREAM_INPUT_TYPE)
  {
    enum avb_sink_state_t state;
    if (get_avb_sink_state(stream_index, &state))
    {
      if (command_type == AECP_AEM_CMD_START_STREAMING)
      {
        set_avb_sink_state(stream_index, AVB_SINK_STATE_ENABLED);
      }
      else
      {
        if (state == AVB_SINK_STATE_ENABLED)
        {
          set_avb_sink_state(stream_index, AVB_SINK_STATE_POTENTIAL);
        }
      }
    }
    else *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;

  }
  else if ((desc_type == AEM_STREAM_OUTPUT_TYPE))
  {
    enum avb_source_state_t state;
    if (get_avb_source_state(stream_index, &state))
    {
      if (command_type == AECP_AEM_CMD_START_STREAMING)
      {
        set_avb_source_state(stream_index, AVB_SOURCE_STATE_ENABLED);
      }
      else
      {
        if (state == AVB_SINK_STATE_ENABLED)
        {
          set_avb_source_state(stream_index, AVB_SOURCE_STATE_POTENTIAL);
        }
      }
    }
    else *status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;
  }
}

static void process_avb_1722_1_aecp_aem_msg(avb_1722_1_aecp_packet_t *pkt, unsigned char src_addr[6], int message_type, int num_pkt_bytes, chanend c_tx)
{
  avb_1722_1_aecp_aem_msg_t *aem_msg = &(pkt->data.aem);
  unsigned char u_flag = AEM_MSG_GET_U_FLAG(aem_msg);
  unsigned short command_type = AEM_MSG_GET_COMMAND_TYPE(aem_msg);
  unsigned char status = AECP_AEM_STATUS_SUCCESS;

  // TODO: Check if the entity is locked/acquired and reply with appropriate status if it is (only on relevant commands)

  if (message_type == AECP_CMD_AEM_COMMAND)
  {
    int cd_len = 0;

    switch (command_type)
    {
      case AECP_AEM_CMD_ACQUIRE_ENTITY: // Long term exclusive control of the entity
      {
        unsigned short desc_type, desc_id;
        avb_1722_1_aem_acquire_entity_command_t *cmd = (avb_1722_1_aem_acquire_entity_command_t *)(pkt->data.aem.command.payload);

        desc_type = ntoh_16(cmd->descriptor_type);
        desc_id = ntoh_16(cmd->descriptor_id);

        if (desc_type != AEM_ENTITY_TYPE || desc_id != 0)
        {
          status = AECP_AEM_STATUS_NOT_SUPPORTED;
        }
        else 
        {
          #if 0
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
                mac_tx(c_tx, avb_1722_1_buf, 64, 0);

                // Set a flag to indicate that the current acquisition is pending
                entity_acquired_status = AEM_ENTITY_ACQUIRED_BUT_PENDING;

                memcpy(pending_controller_guid.c, pkt->controller_guid, 8);

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
        #endif
        
          // TODO: Release
          printstr("1722.1 Controller ");

          for(int i=0; i < 8; i++)
          {
            acquired_controller_guid.c[i] = pkt->controller_guid[i];
            acquired_controller_mac[i] = src_addr[i];
          }

          for (int i=0; i < 8; i++)
          {
            cmd->owner_guid[i] = acquired_controller_guid.c[i];
            printhex(acquired_controller_guid.c[i]);
          }
          printstrln(" acquired entity");

          cd_len = sizeof(avb_1722_1_aem_acquire_entity_command_t);
        }

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
        if (ntoh_32(cmd->flags) == 1) // Unlock
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

          mac_tx(c_tx, avb_1722_1_buf, 68, 0);
        }
        */
        
        break;
      }
      case AECP_AEM_CMD_READ_DESCRIPTOR:
      {
        unsigned short desc_read_type, desc_read_id;
        int num_tx_bytes;

        desc_read_type = ntoh_16(aem_msg->command.read_descriptor_cmd.descriptor_type);
        desc_read_id = ntoh_16(aem_msg->command.read_descriptor_cmd.descriptor_id);

        simple_printf("READ_DESCRIPTOR - type: %d, id: %d\n", desc_read_type, desc_read_id);

        num_tx_bytes = create_aem_read_descriptor_response(desc_read_type, desc_read_id, src_addr, pkt);

        mac_tx(c_tx, avb_1722_1_buf, num_tx_bytes, -1);

        break;
      }
      #if 0
      case AECP_AEM_CMD_GET_AVB_INFO:
      {
        // Command and response share descriptor_type and descriptor_index
        avb_1722_1_aem_get_avb_info_response_t *cmd = (avb_1722_1_aem_get_avb_info_response_t *)(pkt->data.aem.command.payload);
        unsigned short desc_id = ntoh_16(cmd->descriptor_id);

        if (desc_id == 0)
        {
          unsigned int pdelay;
          get_avb_ptp_gm(&cmd->as_grandmaster_id[0]);
          get_avb_ptp_port_pdelay(0, &pdelay);
          hton_32(cmd->propagation_delay, pdelay);
          hton_16(cmd->msrp_mappings_count, 1);
          cmd->msrp_mappings[0] = AVB_SRP_SRCLASS_DEFAULT;
          cmd->msrp_mappings[1] = AVB_SRP_TSPEC_PRIORITY_DEFAULT;
          cmd->msrp_mappings[2] = (AVB_DEFAULT_VLAN>>8)&0xff;
          cmd->msrp_mappings[3] = (AVB_DEFAULT_VLAN&0xff);

          cd_len = sizeof(avb_1722_1_aem_get_avb_info_response_t);
        }
        break;
      }
      case AECP_AEM_CMD_GET_STREAM_FORMAT:
      case AECP_AEM_CMD_SET_STREAM_FORMAT: // Fallthrough intentional
      {
        process_aem_cmd_getset_stream_format(pkt, &status, command_type);
        cd_len = sizeof(avb_1722_1_aem_getset_stream_format_t);
        break;
      }
      case AECP_AEM_CMD_GET_SAMPLING_RATE:
      case AECP_AEM_CMD_SET_SAMPLING_RATE:
      {
        process_aem_cmd_getset_sampling_rate(pkt, &status, command_type);
        cd_len = sizeof(avb_1722_1_aem_getset_sampling_rate_t);
        break;
      }
      case AECP_AEM_CMD_GET_CLOCK_SOURCE:
      case AECP_AEM_CMD_SET_CLOCK_SOURCE:
      {
        process_aem_cmd_getset_clock_source(pkt, &status, command_type);
        cd_len = sizeof(avb_1722_1_aem_getset_clock_source_t);
        break;
      }
      case AECP_AEM_CMD_START_STREAMING:
      case AECP_AEM_CMD_STOP_STREAMING:
      {
        process_aem_cmd_startstop_streaming(pkt, &status, command_type);
        cd_len = sizeof(avb_1722_1_aem_startstop_streaming_t);
        break;
      }
      #endif
      // TODO: ENTITY_AVAILABLE
      default:
      {
        // AECP_AEM_STATUS_NOT_IMPLEMENTED
        return;
      }
    }

    // Send a response if required
    if (cd_len > 0)
    {
      avb_1722_1_create_aecp_aem_response(src_addr, status, cd_len, pkt);
      mac_tx(c_tx, avb_1722_1_buf, 64, -1);
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
