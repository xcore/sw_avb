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


#if AVB_1722_1_CONTROLLER_ENABLED
void avb_entity_on_new_entity_available(const_guid_ref_t my_guid, avb_1722_1_entity_record *entity, chanend c_tx)
{
  // If Talker is enabled, connect to the first XMOS listener we see
  if (AVB_DEMO_ENABLE_TALKER)
  {
    if ((entity->vendor_id == XMOS_VENDOR_ID) &&
       ((entity->listener_capabilities & AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK) == AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK) &&
       (entity->listener_stream_sinks >= 1))
    {
      // Ensure that the listener knows our GUID
      avb_1722_1_adp_announce();

      avb_1722_1_controller_connect(my_guid, &entity->guid, 0, 0, c_tx);
    }
  }
}

void avb_talker_on_listener_connect_failed(const_guid_ref_t my_guid, int source_num,
        const_guid_ref_t listener_guid, avb_1722_1_acmp_status_t status, chanend c_tx)
{
    int i = avb_1722_1_entity_database_find(listener_guid);
    
    // Don't attempt to re-connect if the other end says LISTENER_EXCLUSIVE
    if ((i != AVB_1722_1_MAX_ENTITIES) && (status != ACMP_STATUS_LISTENER_EXCLUSIVE))
        avb_entity_on_new_entity_available(my_guid, &entities[i], c_tx);
}
#endif

/* The controller has indicated to connect this listener sink to a talker stream */
avb_1722_1_acmp_status_t avb_listener_on_talker_connect(int sink_num, const_guid_ref_t talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], const_guid_ref_t my_guid)
{
  // Ensure XMOS devices only connect when they are known entities to ensure correct synchronisation
  int do_connect = 0;

  int channels_per_stream = AVB_NUM_MEDIA_OUTPUTS/AVB_NUM_SINKS;
	int map[channels_per_stream];
  for (int i = 0; i < channels_per_stream; i++) map[i] = sink_num ? sink_num*channels_per_stream+i  : sink_num+i;


  set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED);

  if ((talker_guid->l >> 40) != (XMOS_VENDOR_ID>>8))
  {
    // Non XMOS talker
    do_connect = 1;
  }
  else
  {
    int i = avb_1722_1_entity_database_find(talker_guid);

    if (i != AVB_1722_1_MAX_ENTITIES)
    {
      do_connect = 1;
      if (AVB_DEMO_ENABLE_TALKER && AVB_DEMO_ENABLE_LISTENER && (talker_guid->l < my_guid->l))
      {
        // Check if the remote Talker is also a Listener
        if (entities[i].listener_stream_sinks >= 1)
        {
          // We can be master clock
          set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_LOCAL_CLOCK);
          printstrln("Entity elected Master audio clock");
        }
        // else we remain input stream derived
      }
    }
  }

  if (do_connect)
  {
    simple_printf("CONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

    set_avb_sink_sync(sink_num, 0);
    set_avb_sink_channels(sink_num, channels_per_stream);
    set_avb_sink_map(sink_num, map, channels_per_stream);
    set_avb_sink_id(sink_num, stream_id);
    set_avb_sink_addr(sink_num, dest_addr, 6);

    set_avb_sink_state(sink_num, AVB_SINK_STATE_POTENTIAL);
    return ACMP_STATUS_SUCCESS;
  }

  simple_printf("CONNECTING Listener : entity not found : "); print_guid_ln(talker_guid);
  return ACMP_STATUS_NOT_SUPPORTED;
}

void avb_talker_on_source_address_reserved(int source_num, unsigned char mac_addr[6])
{
  // Do some debug print
  simple_printf("MAAP reserved Talker stream #%d address: %x:%x:%x:%x:%x:%x\n", source_num,
                            mac_addr[0],
                            mac_addr[1],
                            mac_addr[2],
                            mac_addr[3],
                            mac_addr[4],
                            mac_addr[5]);

  set_avb_source_dest(source_num, mac_addr, 6);

  /* NOTE: acmp_talker_init() must be called BEFORE talker_set_mac_address() otherwise it will zero
   * what was just set */
  avb_1722_1_acmp_talker_init();
  avb_1722_1_talker_set_mac_address(source_num, mac_addr);
  avb_1722_1_adp_announce();
}

