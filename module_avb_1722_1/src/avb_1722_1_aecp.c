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
#include "avb_1722_1_aecp_controls.h"
#include "reboot.h"
#include <xs1.h>
#include <platform.h>
#include "avb_util.h"

#if AVB_1722_1_USE_AVC
#include "avc_commands.h"
#endif
#if AVB_1722_1_AEM_ENABLED
#include "aem_descriptor_types.h"
#include "aem_descriptors.h"
#include "aem_descriptor_structs.h"
#endif

extern unsigned int avb_1722_1_buf[AVB_1722_1_PACKET_SIZE_WORDS];
extern guid_t my_guid;
extern unsigned char my_mac_addr[6];
static int g_serial_num;

static avb_timer aecp_aem_lock_timer;

static enum {
  AEM_ENTITY_NOT_ACQUIRED,
  AEM_ENTITY_ACQUIRED,
  AEM_ENTITY_ACQUIRED_AND_PERSISTENT,
  AEM_ENTITY_ACQUIRED_BUT_PENDING = 0x80000001
} entity_acquired_status = AEM_ENTITY_NOT_ACQUIRED;

static enum {
  AECP_AEM_CONTROLLER_AVAILABLE_IN_A=0,
  AECP_AEM_CONTROLLER_AVAILABLE_IN_B,
  AECP_AEM_CONTROLLER_AVAILABLE_IN_C,
  AECP_AEM_CONTROLLER_AVAILABLE_IN_D,
  AECP_AEM_CONTROLLER_AVAILABLE_IN_E,
  AECP_AEM_CONTROLLER_AVAILABLE_IN_F,
  AECP_AEM_CONTROLLER_AVAILABLE_IDLE
} aecp_aem_controller_available_state = AECP_AEM_CONTROLLER_AVAILABLE_IDLE;

static avb_timer aecp_aem_controller_available_timer;
static guid_t pending_controller_guid;
static guid_t acquired_controller_guid;
static unsigned char acquired_controller_mac[6];
static unsigned char pending_controller_mac[6];
static unsigned short pending_controller_sequence;
static unsigned char pending_persistent;
static unsigned short aecp_controller_available_sequence = -1;


static enum {
    AECP_AEM_IDLE,
    AECP_AEM_WAITING,
    AECP_AEM_CONTROLLER_AVAILABLE_TIMEOUT,
    AECP_AEM_LOCK_TIMEOUT
} aecp_aem_state = AECP_AEM_IDLE;

// Called on startup to initialise certain static descriptor fields
void avb_1722_1_aem_descriptors_init(unsigned int serial_num)
{
  g_serial_num = serial_num;
}

void avb_1722_1_aecp_aem_init(unsigned int serial_num)
{
  avb_1722_1_aem_descriptors_init(serial_num);
  init_avb_timer(&aecp_aem_lock_timer, 100);
  init_avb_timer(&aecp_aem_controller_available_timer, 5);

  aecp_aem_state = AECP_AEM_WAITING;
}

static unsigned char *avb_1722_1_create_aecp_response_header(unsigned char dest_addr[6], char status, int message_type, unsigned int data_len, avb_1722_1_aecp_packet_t* cmd_pkt)
{
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

  avb_1722_1_create_1722_1_header(dest_addr, DEFAULT_1722_1_AECP_SUBTYPE, message_type+1, status, data_len, hdr);

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
  avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_response_header(src_addr, status, AECP_CMD_AEM_COMMAND, command_data_len+12, cmd_pkt);

  /* Copy payload_specific_data into the response */
  memcpy(aem, cmd_pkt->data.payload, command_data_len + 2);
}

static void generate_object_name(char *object_name, int number) {
  char num_string[2];
  num_string[0] = (char)(number + 0x30);
  num_string[1] = '\0';
  strcat(object_name, num_string);
}

__attribute__((overlay))
int get_config_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_configuration_0);
  *descriptor = (unsigned char *)&desc_configuration_0;

  return 1;
}

__attribute__((overlay))
int get_audio_unit_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_audio_unit_0);
  *descriptor = (unsigned char *)&desc_audio_unit_0;

  return 1;
}

__attribute__((overlay))
int get_external_port_desc(int read_type, int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  if (read_type == AEM_EXTERNAL_PORT_INPUT_TYPE)
  {
    *desc_size_bytes = sizeof(desc_external_input_port_0);
    *descriptor = (unsigned char *)&desc_external_input_port_0;
  }
  else
  {
    *desc_size_bytes = sizeof(desc_external_output_port_0);
    *descriptor = (unsigned char *)&desc_external_output_port_0;    
  }

  return 1;
}

__attribute__((overlay))
int get_control_identify_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_control_identify);
  *descriptor = (unsigned char *)&desc_control_identify;

  return 1;
}

__attribute__((overlay))
int get_jack_desc(int read_type, int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  if (read_type == AEM_JACK_INPUT_TYPE)
  {
    *desc_size_bytes = sizeof(desc_jack_input_0);
    *descriptor = (unsigned char *)&desc_jack_input_0;
  }
  else
  {
    *desc_size_bytes = sizeof(desc_jack_output_0);
    *descriptor = (unsigned char *)&desc_jack_output_0;    
  }

  return 1;
}

__attribute__((overlay))
int get_interface_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_avb_interface_0);
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
  avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)(pkt->data.payload);
  memcpy(&(aem->command.read_descriptor_resp.descriptor), &desc_avb_interface_0, sizeof(desc_avb_interface_0)+40);

  for (int i=0; i < 6; i++)
  {
    // mac_address in AVB Interface Descriptor
    aem->command.read_descriptor_resp.descriptor[70+i] = my_mac_addr[i];
  }

  // TODO: Should be stored centrally, possibly query PTP for ID per interface
  aem->command.read_descriptor_resp.descriptor[78+0] = my_mac_addr[0];
  aem->command.read_descriptor_resp.descriptor[78+1] = my_mac_addr[1];
  aem->command.read_descriptor_resp.descriptor[78+2] = my_mac_addr[2];
  aem->command.read_descriptor_resp.descriptor[78+3] = 0xff;
  aem->command.read_descriptor_resp.descriptor[78+4] = 0xfe;
  aem->command.read_descriptor_resp.descriptor[78+5] = my_mac_addr[3];
  aem->command.read_descriptor_resp.descriptor[78+6] = my_mac_addr[4];
  aem->command.read_descriptor_resp.descriptor[78+7] = my_mac_addr[5];
  aem->command.read_descriptor_resp.descriptor[78+8] = 0;
  aem->command.read_descriptor_resp.descriptor[78+9] = 1;  

  return 2;
}

__attribute__ ((overlay))
int get_entity_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_entity);
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
  avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)(pkt->data.payload);
  memcpy(&(aem->command.read_descriptor_resp.descriptor), &desc_entity, sizeof(desc_entity)+40);
  // unsigned char *desc = &(aem->command.read_descriptor_resp.descriptor);

  for (int i=0; i < 8; i++)
  {
    aem->command.read_descriptor_resp.descriptor[4+i] = my_guid.c[7-i];
  }

  avb_itoa((int)g_serial_num,(char *)&aem->command.read_descriptor_resp.descriptor[244], 10, 0);

  return 2;
}

__attribute__((overlay))
int get_clock_source_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  switch (read_id) {
    case 0:
      *desc_size_bytes = sizeof(desc_clock_source_0);
      *descriptor = (unsigned char *)&desc_clock_source_0;
      break;
    case 1:
      *desc_size_bytes = sizeof(desc_clock_source_1);
      *descriptor = (unsigned char *)&desc_clock_source_1;
      break;   
    default:
      return 0;   
  }

  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
  avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)(pkt->data.payload);
  memcpy(&(aem->command.read_descriptor_resp.descriptor), *descriptor, sizeof(desc_clock_source_0)+40);

  for (int i=0; i < 6; i++)
  {
    // clock_source_identifier in clock source descriptor
    aem->command.read_descriptor_resp.descriptor[74+i] = my_mac_addr[i];
  }

  return 2;
}

__attribute__((overlay))
int get_clock_domain_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_clock_domain_0);
  *descriptor = (unsigned char *)&desc_clock_domain_0;

  return 1;
}

__attribute__((overlay))
int get_locale_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_locale_0);
  *descriptor = (unsigned char *)&desc_locale_0;

  return 1;
}

__attribute__((overlay))
int get_strings_desc(int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  *desc_size_bytes = sizeof(desc_strings_0);
  *descriptor = (unsigned char *)&desc_strings_0;

  return 1;
}

int find_static_descriptor(unsigned int read_type, unsigned int read_id, int *desc_size_bytes, unsigned char **descriptor)
{
  switch (read_type) {
    case AEM_ENTITY_TYPE:
      return get_entity_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_CONFIGURATION_TYPE:
      return get_config_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_AUDIO_UNIT_TYPE:
      return get_audio_unit_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_EXTERNAL_PORT_INPUT_TYPE:
    case AEM_EXTERNAL_PORT_OUTPUT_TYPE:
      return get_external_port_desc(read_type, read_id, desc_size_bytes, descriptor); break;
    case AEM_CONTROL_TYPE:
      switch (read_id) 
      {
        case DESCRIPTOR_INDEX_CONTROL_IDENTIFY:
          return get_control_identify_desc(read_id, desc_size_bytes, descriptor); break;
        default:
          return 0;
      }
    case AEM_JACK_INPUT_TYPE:
    case AEM_JACK_OUTPUT_TYPE:
      return get_jack_desc(read_type, read_id, desc_size_bytes, descriptor); break;
    case AEM_AVB_INTERFACE_TYPE:
      return get_interface_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_CLOCK_SOURCE_TYPE:
      return get_clock_source_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_CLOCK_DOMAIN_TYPE:
      return get_clock_domain_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_LOCALE_TYPE:
      return get_locale_desc(read_id, desc_size_bytes, descriptor); break;
    case AEM_STRINGS_TYPE:
      return get_strings_desc(read_id, desc_size_bytes, descriptor); break;
    default:
      return 0;
  }
}

__attribute__((overlay))
int generate_audio_map_desc(int read_id)
{
  const int num_mappings = (read_id < AVB_NUM_SINKS) ? AVB_NUM_MEDIA_OUTPUTS/AVB_NUM_SINKS : AVB_NUM_MEDIA_INPUTS/AVB_NUM_SOURCES;

  /* Since the map descriptors aren't constant size, unlike the clusters, and
   * dependent on the number of channels, we don't use a template */

  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
  avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)(pkt->data.payload);
  unsigned char *pktptr = (unsigned char *)&(aem->command.read_descriptor_resp.descriptor);
  aem_desc_audio_map_t *audio_map = (aem_desc_audio_map_t *)pktptr;

  int desc_size_bytes = 8+(num_mappings*8);

  memset(audio_map, 0, desc_size_bytes);
  hton_16(audio_map->descriptor_type, AEM_AUDIO_MAP_TYPE);
  hton_16(audio_map->descriptor_index, read_id);
  hton_16(audio_map->mappings_offset, 8);
  hton_16(audio_map->number_of_mappings, num_mappings);

  for (int i=0; i < num_mappings; i++)
  {
    hton_16(audio_map->mappings[i].mapping_stream_index, read_id);
    hton_16(audio_map->mappings[i].mapping_stream_channel, i);
    hton_16(audio_map->mappings[i].mapping_cluster_offset, (read_id < AVB_NUM_SOURCES) ? i%AVB_NUM_SOURCES : i%AVB_NUM_SINKS);
    hton_16(audio_map->mappings[i].mapping_cluster_channel, 0); // Single channel audio clusters
  }

  return desc_size_bytes;
}

__attribute__((overlay))
void generate_template_desc(int read_type, int read_id, unsigned char *descriptor)
{
  aem_desc_audio_cluster_t *cluster = (aem_desc_audio_cluster_t *)descriptor;
  char id_num = (char)read_id;

  // The descriptor id is also the channel number
  cluster->descriptor_index[1] = (uint8_t)read_id;

  if ((read_type == AEM_AUDIO_CLUSTER_TYPE) || read_type == AEM_STREAM_OUTPUT_TYPE)
  {
    int id = (int)read_id;;
    if (read_id >= AVB_NUM_MEDIA_OUTPUTS) {
      id = (int)read_id - AVB_NUM_MEDIA_OUTPUTS;
    }
    memset(cluster->object_name, 0, 64);
    strcpy((char *)cluster->object_name, "Output ");
    generate_object_name((char *)cluster->object_name, id);
  }
  else if (read_type == AEM_STREAM_INPUT_TYPE)
  {
    memset(cluster->object_name, 0, 64);
    strcpy((char *)cluster->object_name, "Input ");
    generate_object_name((char *)cluster->object_name, (int)id_num);
  }

  if (read_type == AEM_STREAM_PORT_OUTPUT_TYPE) {
    aem_desc_stream_port_input_output_t *stream_port = (aem_desc_stream_port_input_output_t *)descriptor;
    hton_16(stream_port->base_cluster, read_id * AVB_NUM_SOURCES);
    hton_16(stream_port->base_map, read_id);
  }
  else if (read_type == AEM_STREAM_PORT_INPUT_TYPE) {
    aem_desc_stream_port_input_output_t *stream_port = (aem_desc_stream_port_input_output_t *)descriptor;
    hton_16(stream_port->base_cluster, read_id * AVB_NUM_SINKS);
    hton_16(stream_port->base_map, read_id);
  }
}

static int create_aem_read_descriptor_response(unsigned int read_type, unsigned int read_id, unsigned char src_addr[6], avb_1722_1_aecp_packet_t *pkt)
{
  int desc_size_bytes = 0, i = 0;
  unsigned char *descriptor = NULL;
  int found_descriptor = 0;

#if AEM_GENERATE_DESCRIPTORS_ON_FLY
  switch (read_type) {
    case AEM_AUDIO_CLUSTER_TYPE:
      if (read_id < (AVB_NUM_MEDIA_OUTPUTS+AVB_NUM_MEDIA_INPUTS)) {
        descriptor = &desc_audio_cluster_template[0];
        desc_size_bytes = sizeof(aem_desc_audio_cluster_t);
      }
      break;
    case AEM_STREAM_INPUT_TYPE:
      if (read_id < AVB_NUM_SINKS) {
        descriptor = &desc_stream_input_0[0];
        desc_size_bytes = sizeof(aem_desc_stream_input_output_t);
      }
      break;
    case AEM_STREAM_OUTPUT_TYPE:
      if (read_id < AVB_NUM_SOURCES) {
        descriptor = &desc_stream_output_0[0];
        desc_size_bytes = sizeof(aem_desc_stream_input_output_t);
      }
      break;
    case AEM_STREAM_PORT_INPUT_TYPE:
      if (read_id < AVB_NUM_SINKS) {
        descriptor = &desc_stream_port_input_0[0];
        desc_size_bytes = sizeof(aem_desc_stream_port_input_output_t);
      }
      break;
    case AEM_STREAM_PORT_OUTPUT_TYPE:
      if (read_id < AVB_NUM_SOURCES) {
        descriptor = &desc_stream_port_output_0[0];
        desc_size_bytes = sizeof(aem_desc_stream_port_input_output_t);
      }
      break;
  }

  if (descriptor != NULL)
  {
    generate_template_desc(read_type, read_id, descriptor);

    found_descriptor = 1;
  }
  else if (read_type == AEM_AUDIO_MAP_TYPE)
  {
    if (read_id < (AVB_NUM_SINKS+AVB_NUM_SOURCES))
    {
      desc_size_bytes = generate_audio_map_desc(read_id);
      found_descriptor = 2;
    }

  }
  else
#endif
  {
    found_descriptor = find_static_descriptor(read_type, read_id, &desc_size_bytes, &descriptor);
  }


  if (found_descriptor)
  {
    int packet_size = sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+24+desc_size_bytes;

    avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_response_header(src_addr, AECP_AEM_STATUS_SUCCESS, AECP_CMD_AEM_COMMAND, desc_size_bytes+16, pkt);

    memcpy(aem, pkt->data.payload, 6);
    if (found_descriptor < 2) memcpy(&(aem->command.read_descriptor_resp.descriptor), descriptor, desc_size_bytes+40);

    return packet_size;
  }
  else // Descriptor not found, send NO_SUCH_DESCRIPTOR reply
  {
    int packet_size = sizeof(ethernet_hdr_t)+sizeof(avb_1722_1_packet_header_t)+20+sizeof(avb_1722_1_aem_read_descriptor_command_t);

    avb_1722_1_aecp_aem_msg_t *aem = (avb_1722_1_aecp_aem_msg_t*)avb_1722_1_create_aecp_response_header(src_addr, AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR, AECP_CMD_AEM_COMMAND, 40, pkt);

    memcpy(aem, pkt->data.payload, 20+sizeof(avb_1722_1_aem_read_descriptor_command_t));

    return packet_size;
  }
}

static unsigned short avb_1722_1_create_controller_available_packet(void)
{
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
  avb_1722_1_aecp_aem_msg_t *aem_msg = &(pkt->data.aem);

  avb_1722_1_create_1722_1_header(acquired_controller_mac, DEFAULT_1722_1_AECP_SUBTYPE, AECP_CMD_AEM_COMMAND, AECP_AEM_STATUS_SUCCESS, AVB_1722_1_AECP_COMMAND_DATA_OFFSET, hdr);

  set_64(pkt->target_guid, acquired_controller_guid.c);
  set_64(pkt->controller_guid, my_guid.c);
  hton_16(pkt->sequence_id, aecp_controller_available_sequence);

  AEM_MSG_SET_COMMAND_TYPE(aem_msg, AECP_AEM_CMD_CONTROLLER_AVAILABLE);
  AEM_MSG_SET_U_FLAG(aem_msg, 0);

  return AVB_1722_1_AECP_PAYLOAD_OFFSET;
}

static unsigned short avb_1722_1_create_acquire_response_packet(unsigned char status)
{
  struct ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
  avb_1722_1_aecp_packet_t *pkt = (avb_1722_1_aecp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);
  avb_1722_1_aecp_aem_msg_t *aem_msg = &(pkt->data.aem);

  aem_msg->command.acquire_entity_cmd.flags[0] = pending_persistent;
  set_64(aem_msg->command.acquire_entity_cmd.owner_guid, acquired_controller_guid.c);
  hton_16(aem_msg->command.acquire_entity_cmd.descriptor_type, 0);
  hton_16(aem_msg->command.acquire_entity_cmd.descriptor_id, 0);

  avb_1722_1_create_1722_1_header(pending_controller_mac, DEFAULT_1722_1_AECP_SUBTYPE, AECP_CMD_AEM_RESPONSE, status, AVB_1722_1_AECP_COMMAND_DATA_OFFSET + sizeof(avb_1722_1_aem_acquire_entity_command_t), hdr);

  set_64(pkt->target_guid, my_guid.c);
  set_64(pkt->controller_guid, pending_controller_guid.c);
  hton_16(pkt->sequence_id, pending_controller_sequence);

  AEM_MSG_SET_COMMAND_TYPE(aem_msg, AECP_AEM_CMD_ACQUIRE_ENTITY);
  AEM_MSG_SET_U_FLAG(aem_msg, 0);

  return sizeof(avb_1722_1_aem_acquire_entity_command_t) + AVB_1722_1_AECP_PAYLOAD_OFFSET;
}

__attribute__((overlay))
static unsigned short process_aem_cmd_acquire(avb_1722_1_aecp_packet_t *pkt, unsigned char *status, unsigned char src_addr[6], chanend c_tx)
{
  unsigned short descriptor_index = ntoh_16(pkt->data.aem.command.acquire_entity_cmd.descriptor_id);
  unsigned short descriptor_type = ntoh_16(pkt->data.aem.command.acquire_entity_cmd.descriptor_type);

  if (AEM_ENTITY_TYPE == descriptor_type && 0 == descriptor_index)
  {
    if (AEM_ACQUIRE_ENTITY_RELEASE_FLAG(&(pkt->data.aem.command.acquire_entity_cmd)))
    {
      //Release
      if (entity_acquired_status == AEM_ENTITY_NOT_ACQUIRED)
      {
        *status = AECP_AEM_STATUS_BAD_ARGUMENTS;
      }
      else if (compare_guid(pkt->controller_guid, &acquired_controller_guid))
      {
        *status = AECP_AEM_STATUS_SUCCESS;
        entity_acquired_status = AEM_ENTITY_NOT_ACQUIRED;
        printstr("1722.1 Controller ");
        for(int i=0; i < 8; i++)
        {
          printhex(acquired_controller_guid.c[7-i]);
          acquired_controller_guid.c[7-i] = 0;
        }
        printstrln(" released entity");
        memset(&acquired_controller_mac, 0, 6);
      }
      else
      {
        *status = AECP_AEM_STATUS_ENTITY_ACQUIRED;

        for(int i=0; i < 8; i++)
        {
          pkt->data.aem.command.acquire_entity_cmd.owner_guid[i] = acquired_controller_guid.c[7-i];
        }
      }
    }
    else
    {
      //Acquire

      switch (entity_acquired_status)
      {
        case AEM_ENTITY_NOT_ACQUIRED:
          *status = AECP_AEM_STATUS_SUCCESS;
          if (AEM_ACQUIRE_ENTITY_PERSISTENT_FLAG(&(pkt->data.aem.command.acquire_entity_cmd)))
          {
            entity_acquired_status = AEM_ENTITY_ACQUIRED_AND_PERSISTENT;
          }
          else
          {
            entity_acquired_status = AEM_ENTITY_ACQUIRED;
          }
          printstr("1722.1 Controller ");
          for(int i=0; i < 8; i++)
          {
            acquired_controller_guid.c[7-i] = pkt->controller_guid[i];
            pkt->data.aem.command.acquire_entity_cmd.owner_guid[i] = acquired_controller_guid.c[7-i];
            printhex(acquired_controller_guid.c[7-i]);
          }
          printstrln(" acquired entity");
          memcpy(&acquired_controller_mac, &src_addr, 6);
          break;

        case AEM_ENTITY_ACQUIRED_BUT_PENDING:

          break;

        case AEM_ENTITY_ACQUIRED:
          if (compare_guid(pkt->controller_guid, &acquired_controller_guid))
          {
            *status = AECP_AEM_STATUS_SUCCESS;
          }
          else
          {
            *status = AECP_AEM_STATUS_IN_PROGRESS;
            for(int i=0; i < 8; i++)
            {
              pending_controller_guid.c[7-i] = pkt->controller_guid[i];
            }
            memcpy(&pending_controller_mac, &src_addr, 6);
            pending_controller_sequence = ntoh_16(pkt->sequence_id);
            pending_persistent = AEM_ACQUIRE_ENTITY_PERSISTENT_FLAG(&(pkt->data.aem.command.acquire_entity_cmd));

            aecp_controller_available_sequence++;

            avb_1722_1_create_controller_available_packet();
            mac_tx(c_tx, avb_1722_1_buf, 64, -1);

            start_avb_timer(&aecp_aem_controller_available_timer, 12);
            aecp_aem_controller_available_state = AECP_AEM_CONTROLLER_AVAILABLE_IN_A;
            entity_acquired_status = AEM_ENTITY_ACQUIRED_BUT_PENDING;
          }

          for(int i=0; i < 8; i++)
          {
            pkt->data.aem.command.acquire_entity_cmd.owner_guid[i] = acquired_controller_guid.c[7-i];
          }
          break;
        case AEM_ENTITY_ACQUIRED_AND_PERSISTENT:
          if (compare_guid(pkt->controller_guid, &acquired_controller_guid))
          {
            *status = AECP_AEM_STATUS_SUCCESS;
          }
          else
          {
            *status = AECP_AEM_STATUS_ENTITY_ACQUIRED;
          }

          for(int i=0; i < 8; i++)
          {
            pkt->data.aem.command.acquire_entity_cmd.owner_guid[i] = acquired_controller_guid.c[7-i];
          }
          break;
      }
    }
  }
  else
  {
    *status = AECP_AEM_STATUS_NOT_SUPPORTED;
  }


  return sizeof(avb_1722_1_aem_acquire_entity_command_t) + AVB_1722_1_AECP_PAYLOAD_OFFSET;
}

// __attribute__((overlay))
static void process_avb_1722_1_aecp_aem_msg(avb_1722_1_aecp_packet_t *pkt,
                                            unsigned char src_addr[6],
                                            int message_type,
                                            int num_pkt_bytes,
                                            chanend c_tx,
                                            CLIENT_INTERFACE(avb_interface, i_avb_api),
                                            CLIENT_INTERFACE(avb_1722_1_control_callbacks, i_1722_1_entity))
{
  avb_1722_1_aecp_aem_msg_t *aem_msg = &(pkt->data.aem);
  unsigned short command_type = AEM_MSG_GET_COMMAND_TYPE(aem_msg);
  unsigned char status = AECP_AEM_STATUS_SUCCESS;
  int cd_len = 0;

  if (message_type == AECP_CMD_AEM_COMMAND)
  {
    if (compare_guid(pkt->target_guid, &my_guid)==0) return;

    switch (command_type)
    {
      case AECP_AEM_CMD_ACQUIRE_ENTITY: // Long term exclusive control of the entity
      {
        cd_len = process_aem_cmd_acquire(pkt, &status, src_addr, c_tx);
        break;
      }
      case AECP_AEM_CMD_LOCK_ENTITY: // Atomic operation on the entity
      {
        break;
      }
      case AECP_AEM_CMD_ENTITY_AVAILABLE:
      {
        cd_len = AVB_1722_1_AECP_PAYLOAD_OFFSET;
        break;
      }
      case AECP_AEM_CMD_REBOOT:
      {
        // Reply before reboot, do the reboot after sending the packet
        cd_len = AVB_1722_1_AECP_PAYLOAD_OFFSET;
        break;
      }
      case AECP_AEM_CMD_READ_DESCRIPTOR:
      {
        unsigned int desc_read_type, desc_read_id;
        int num_tx_bytes;

        desc_read_type = ntoh_16(aem_msg->command.read_descriptor_cmd.descriptor_type);
        desc_read_id = ntoh_16(aem_msg->command.read_descriptor_cmd.descriptor_id);

        num_tx_bytes = create_aem_read_descriptor_response(desc_read_type, desc_read_id, src_addr, pkt);

        if (num_tx_bytes < 64) num_tx_bytes = 64;

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
      #endif
      case AECP_AEM_CMD_GET_SAMPLING_RATE:
      case AECP_AEM_CMD_SET_SAMPLING_RATE:
      {
        process_aem_cmd_getset_sampling_rate(pkt, &status, command_type, i_avb_api);
        cd_len = sizeof(avb_1722_1_aem_getset_sampling_rate_t);
        break;
      }
      case AECP_AEM_CMD_GET_CLOCK_SOURCE:
      case AECP_AEM_CMD_SET_CLOCK_SOURCE:
      {
        process_aem_cmd_getset_clock_source(pkt, &status, command_type, i_avb_api);
        cd_len = sizeof(avb_1722_1_aem_getset_clock_source_t);
        break;
      }
      case AECP_AEM_CMD_START_STREAMING:
      case AECP_AEM_CMD_STOP_STREAMING:
      {
        process_aem_cmd_startstop_streaming(pkt, &status, command_type, i_avb_api);
        cd_len = sizeof(avb_1722_1_aem_startstop_streaming_t);
        break;
      }
      case AECP_AEM_CMD_GET_CONTROL:
      case AECP_AEM_CMD_SET_CONTROL:
      {
        cd_len = process_aem_cmd_getset_control(pkt, &status, command_type, i_1722_1_entity) + sizeof(avb_1722_1_aem_getset_control_t) + AVB_1722_1_AECP_COMMAND_DATA_OFFSET;
        break;
      }
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
    }
  }
  else // AECP_CMD_AEM_RESPONSE
  {
    switch (command_type)
    {
      case AECP_AEM_CMD_CONTROLLER_AVAILABLE:
        if (AEM_ENTITY_ACQUIRED_BUT_PENDING == entity_acquired_status && compare_guid(pkt->controller_guid, &my_guid))
        {
          if (compare_guid(pkt->target_guid, &pending_controller_guid))
          {
            entity_acquired_status = AEM_ENTITY_ACQUIRED;
            aecp_aem_controller_available_state = AECP_AEM_CONTROLLER_AVAILABLE_IDLE;

            cd_len = avb_1722_1_create_acquire_response_packet(AECP_AEM_STATUS_SUCCESS);
          }
          else if (compare_guid(pkt->target_guid, &acquired_controller_guid))
          {
            if (AEM_ACQUIRE_ENTITY_PERSISTENT_FLAG(&(pkt->data.aem.command.acquire_entity_cmd)))
            {
              entity_acquired_status = AEM_ENTITY_ACQUIRED_AND_PERSISTENT;
            }
            else
            {
              entity_acquired_status = AEM_ENTITY_ACQUIRED;
            }

            cd_len = avb_1722_1_create_acquire_response_packet(AECP_AEM_STATUS_ENTITY_ACQUIRED);
            stop_avb_timer(&aecp_aem_controller_available_timer);
          }
        }
        break;
      default:
        break;
    }
  }

  if (cd_len > 0)
  {
    int num_tx_bytes = cd_len;

    if (num_tx_bytes < 64) num_tx_bytes = 64;

    mac_tx(c_tx, avb_1722_1_buf, num_tx_bytes, -1);

    if (command_type == AECP_AEM_CMD_REBOOT) {
      waitfor(10000); // Wait for the response packet to egress
      device_reboot();
    }
  }
}

__attribute__((overlay))
static void process_avb_1722_1_aecp_address_access_cmd(avb_1722_1_aecp_packet_t *pkt,
                                            unsigned char src_addr[6],
                                            int message_type,
                                            int num_pkt_bytes,
                                            CLIENT_INTERFACE(spi_inteface, i_spi),
                                            chanend c_tx)
{
  avb_1722_1_aecp_address_access_t *aa_cmd = &(pkt->data.address);
  int tlv_count = ntoh_16(aa_cmd->tlv_count);
  unsigned short status = AECP_AA_STATUS_SUCCESS;
  int mode = ADDRESS_MSG_GET_MODE(aa_cmd);
  int length = ADDRESS_MSG_GET_LENGTH(aa_cmd);
  int cd_len = 0;

  if (compare_guid(pkt->target_guid, &my_guid)==0) return;

  if (tlv_count != 1 || mode != AECP_AA_MODE_WRITE || length != 256) {
    status = AECP_AA_STATUS_UNSUPPORTED;
  }
  else {
    long long address = ntoh_32(&aa_cmd->address[4]);
    if (avb_write_upgrade_image_page(i_spi, address, aa_cmd->data) != 0) {
      status = AECP_AA_STATUS_ADDRESS_INVALID;
    }
    cd_len = num_pkt_bytes;
  }

  // Send a response if required
  if (cd_len > 0)
  {
    avb_1722_1_aecp_address_access_t *aa_pkt = (avb_1722_1_aecp_address_access_t*)avb_1722_1_create_aecp_response_header(src_addr, status, AECP_CMD_ADDRESS_ACCESS_COMMAND, 278, pkt);

    /* Copy payload_specific_data into the response */
    memcpy(aa_pkt, pkt->data.payload, sizeof(avb_1722_1_aecp_address_access_t));
  }

  if (cd_len > 0)
  {
    int num_tx_bytes = cd_len;

    if (num_tx_bytes < 64) num_tx_bytes = 64;

    mac_tx(c_tx, avb_1722_1_buf, 304, -1);
  }
}


void process_avb_1722_1_aecp_packet(unsigned char src_addr[6],
                                    avb_1722_1_aecp_packet_t *pkt,
                                    int num_pkt_bytes,
                                    chanend c_tx,
                                    CLIENT_INTERFACE(avb_interface, i_avb),
                                    CLIENT_INTERFACE(avb_1722_1_control_callbacks, i_1722_1_entity),
                                    CLIENT_INTERFACE(spi_inteface, i_spi))
{
  int message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));

  switch (message_type)
  {
    case AECP_CMD_AEM_COMMAND:
    case AECP_CMD_AEM_RESPONSE:
    {
#if AVB_1722_1_AEM_ENABLED
      process_avb_1722_1_aecp_aem_msg(pkt, src_addr, message_type, num_pkt_bytes, c_tx, i_avb, i_1722_1_entity);
#endif
      break;
    }
    case AECP_CMD_ADDRESS_ACCESS_COMMAND:
    {
      process_avb_1722_1_aecp_address_access_cmd(pkt, src_addr, message_type, num_pkt_bytes, i_spi, c_tx);
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
  char available_timeouts[5] = {12, 1, 11, 12, 2};
  if (avb_timer_expired(&aecp_aem_controller_available_timer))
  {
    int cd_len = 0;

    //Timeline

    //TX Controller Available
    //  IN_A state for 120ms
    //TX IN_PROGRESS response
    //  IN_B state for 120ms
    //TX IN_PROGRESS
    //  IN_C state for 10ms
    //TX Controller Available
    //  IN_D state for 110ms
    //TX IN_PROGRESS response
    //  IN_E state for 120ms
    //TX IN_PROGRESS response
    //  IN_F state for 20ms
    //Timed out so it's an acquire

    switch (aecp_aem_controller_available_state)
    {
      case AECP_AEM_CONTROLLER_AVAILABLE_IDLE:
        //Nothing to do
        break;
      case AECP_AEM_CONTROLLER_AVAILABLE_IN_A:
      case AECP_AEM_CONTROLLER_AVAILABLE_IN_B:
      case AECP_AEM_CONTROLLER_AVAILABLE_IN_C:
      case AECP_AEM_CONTROLLER_AVAILABLE_IN_D:
      case AECP_AEM_CONTROLLER_AVAILABLE_IN_E:
        cd_len = avb_1722_1_create_acquire_response_packet(AECP_AEM_STATUS_IN_PROGRESS);
        start_avb_timer(&aecp_aem_controller_available_timer, available_timeouts[aecp_aem_controller_available_state]);
        aecp_aem_controller_available_state++;
        break;
      case AECP_AEM_CONTROLLER_AVAILABLE_IN_F:
        if (pending_persistent)
        {
          entity_acquired_status = AEM_ENTITY_ACQUIRED_AND_PERSISTENT;
        }
        else
        {
          entity_acquired_status = AEM_ENTITY_ACQUIRED;
        }
        printstr("1722.1 Controller ");
        for(int i=0; i < 8; i++)
        {
          acquired_controller_guid.c[7-i] = pending_controller_guid.c[7-i];
          //pkt->data.aem.command.acquire_entity_cmd.owner_guid[i] = acquired_controller_guid.c[7-i] = pending_controller_guid[7-i];
          printhex(acquired_controller_guid.c[7-i]);
        }
        printstrln(" acquired entity after timeout");
        memcpy(&acquired_controller_mac, &pending_controller_mac, 6);

        //TODO: Construct and send response to pending controller
        cd_len = avb_1722_1_create_acquire_response_packet(AECP_AEM_STATUS_SUCCESS);

        aecp_aem_controller_available_state = AECP_AEM_CONTROLLER_AVAILABLE_IDLE;
        break;
    }

    if (cd_len)
    {
      int num_tx_bytes = cd_len;

      if(num_tx_bytes < 64)
      {
        num_tx_bytes = 64;
      }

      mac_tx(c_tx, avb_1722_1_buf, num_tx_bytes, -1);
    }
  }

}
