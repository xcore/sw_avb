#include <xccompat.h>
#include <print.h>
#include "simple_printf.h"
#include "avb.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_1722_maap.h"
#include "avb_1722_maap_protocol.h"
#include "avb_control_types.h"
#if AVB_ENABLE_1722_1
#include "avb_1722_1_common.h"
#include "avb_1722_1_acmp.h"
#include "avb_1722_1_adp.h"
#endif

#if AVB_ENABLE_1722_1
extern avb_1722_1_entity_record entities[AVB_1722_1_MAX_ENTITIES];
static int controller_state = 0;

#define XMOS_VENDOR_ID 0x00229700

void simple_demo_controller(int *change_stream, int *toggle_remote, chanend c_tx)
{
  if (*toggle_remote != controller_state)
  {
    avb_1722_1_controller_disconnect_all_listeners(0, c_tx);

    if (*toggle_remote)
    {
      avb_1722_1_acmp_controller_deinit();
    }
    else
    {
      avb_1722_1_acmp_controller_init();
      avb_1722_1_entity_database_flush();
      avb_1722_1_adp_discover_all();
    }
  }
  controller_state = *toggle_remote;
}


void avb_entity_on_new_entity_available(client interface avb_interface avb, const_guid_ref_t my_guid, avb_1722_1_entity_record *entity, chanend c_tx)
{
  // If Talker is enabled, connect to the first XMOS listener we see
  if (AVB_DEMO_ENABLE_TALKER && AVB_1722_1_CONTROLLER_ENABLED)
  {
    if ((entity->vendor_id == XMOS_VENDOR_ID) &&
       ((entity->listener_capabilities & AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK) == AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK) &&
       (entity->listener_stream_sinks >= 1))
    {
      // Ensure that the listener knows our GUID
      avb_1722_1_adp_announce();

      avb_1722_1_controller_connect(my_guid, entity->guid, 0, 0, c_tx);
    }
  }
}

void avb_talker_on_listener_connect_failed(client interface avb_interface avb, const_guid_ref_t my_guid, int source_num,
        const_guid_ref_t listener_guid, avb_1722_1_acmp_status_t status, chanend c_tx)
{
}

/* The controller has indicated that a listener is connecting to this talker stream */
void avb_talker_on_listener_connect(client interface avb_interface avb, int source_num, const_guid_ref_t listener_guid)
{
  unsigned stream_id[2];
  enum avb_source_state_t state;
  avb.get_source_state(source_num, state);
  avb.get_source_id(source_num, stream_id);

  simple_printf("CONNECTING Talker stream #%d (%x%x) -> Listener ", source_num, stream_id[0], stream_id[1]); print_guid_ln(listener_guid);

  // If this is the first listener to connect to this talker stream, we do a stream registration
  // to reserve the necessary bandwidth on the network
  if (state == AVB_SOURCE_STATE_DISABLED)
  {
    avb_1722_1_talker_set_stream_id(source_num, stream_id);

    avb.set_source_state(source_num, AVB_SOURCE_STATE_POTENTIAL);
  }
}

/* The controller has indicated to connect this listener sink to a talker stream */
avb_1722_1_acmp_status_t avb_listener_on_talker_connect(client interface avb_interface avb, int sink_num, const_guid_ref_t talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], const_guid_ref_t my_guid)
{
  const int channels_per_stream = AVB_NUM_MEDIA_OUTPUTS/AVB_NUM_SINKS;
  int map[AVB_NUM_MEDIA_OUTPUTS/AVB_NUM_SINKS];
  for (int i = 0; i < channels_per_stream; i++) map[i] = sink_num ? sink_num*channels_per_stream+i  : sink_num+i;

  avb.set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED);

  simple_printf("CONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  avb.set_sink_sync(sink_num, 0);
  avb.set_sink_channels(sink_num, channels_per_stream);
  avb.set_sink_map(sink_num, map, channels_per_stream);
  avb.set_sink_id(sink_num, stream_id);
  avb.set_sink_addr(sink_num, dest_addr, 6);

  avb.set_sink_state(sink_num, AVB_SINK_STATE_POTENTIAL);
  return ACMP_STATUS_SUCCESS;
}

/* The controller has indicated to disconnect this listener sink from a talker stream */
void avb_listener_on_talker_disconnect(client interface avb_interface avb, int sink_num, const_guid_ref_t talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], const_guid_ref_t my_guid)
{
  simple_printf("DISCONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  avb.set_sink_state(sink_num, AVB_SINK_STATE_DISABLED);
}

/* The controller has indicated that a listener is disconnecting from this talker stream */
void avb_talker_on_listener_disconnect(client interface avb_interface avb, int source_num, const_guid_ref_t listener_guid, int connection_count)
{
  unsigned stream_id[2];
  enum avb_source_state_t state;
  avb.get_source_state(source_num, state);
  avb.get_source_id(source_num, stream_id);

  simple_printf("DISCONNECTING Talker stream #%d (%x%x) -> Listener ", source_num, stream_id[0], stream_id[1]); print_guid_ln(listener_guid);

  if ((state > AVB_SOURCE_STATE_DISABLED) && (connection_count == 0))
  {
    avb.set_source_state(source_num, AVB_SOURCE_STATE_DISABLED);
  }
}

void avb_talker_on_source_address_reserved(client interface avb_interface avb, int source_num, unsigned char mac_addr[6])
{
  // Do some debug print
  simple_printf("MAAP reserved Talker stream #%d address: %x:%x:%x:%x:%x:%x\n", source_num,
                            mac_addr[0],
                            mac_addr[1],
                            mac_addr[2],
                            mac_addr[3],
                            mac_addr[4],
                            mac_addr[5]);

  avb.set_source_dest(source_num, mac_addr, 6);

  /* NOTE: acmp_talker_init() must be called BEFORE talker_set_mac_address() otherwise it will zero
   * what was just set */
  avb_1722_1_acmp_talker_init();
  avb_1722_1_talker_set_mac_address(source_num, mac_addr);
  avb_1722_1_adp_announce();
}
#endif
