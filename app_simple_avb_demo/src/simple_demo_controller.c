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
static int entity_elected_master_clock = 0;
static int controller_state = 0;

#define XMOS_VENDOR_ID 0x00229700

void simple_demo_controller(int *change_stream, int *toggle_remote, chanend c_tx)
{
  if (*toggle_remote != controller_state)
  {
    avb_1722_1_controller_disconnect_all_listeners(c_tx, 0);

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
void avb_entity_on_new_entity_available(guid_t *my_guid, avb_1722_1_entity_record *entity, chanend c_tx)
{
  enum avb_source_state_t state;
  get_avb_source_state(0, &state);

  // If our first Talker stream isn't connected already, connect to the first XMOS listener we see
  if (state == AVB_SOURCE_STATE_DISABLED)
  {
    if ((entity->vendor_id == XMOS_VENDOR_ID) &&
       ((entity->listener_capabilites & AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK) == AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK) &&
       (entity->listener_stream_sinks >= 1))
    {
      avb_1722_1_controller_connect(my_guid, &entity->guid, 0, 0, c_tx);
    }
  }
}
#endif

/* The controller has indicated to connect this listener sink to a talker stream */
void avb_listener_on_talker_connect(int sink_num, REFERENCE_PARAM(guid_t, talker_guid), unsigned char dest_addr[6], unsigned int stream_id[2])
{
  int map[AVB_NUM_MEDIA_OUTPUTS];
  for (int i = 0; i < AVB_NUM_MEDIA_OUTPUTS; i++) map[i] = i;

  simple_printf("CONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  if ((talker_guid->l >> 40) != (XMOS_VENDOR_ID>>8))
  {
    set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED);
    printstrln("Non XMOS talker: setting input stream derived clock");
  }
  else
  {
    // If we were previously master clock, restore this on connection to an XMOS talker
    if (entity_elected_master_clock)
    {
      set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_LOCAL_CLOCK);
      printstrln("Entity is Master audio clock");
    }
  }

  set_avb_sink_sync(sink_num, 0);
  set_avb_sink_channels(sink_num, AVB_NUM_MEDIA_OUTPUTS);
  set_avb_sink_map(sink_num, map, AVB_NUM_MEDIA_OUTPUTS);
  set_avb_sink_id(sink_num, stream_id);
  set_avb_sink_addr(sink_num, dest_addr, 6);

  set_avb_sink_state(sink_num, AVB_SINK_STATE_POTENTIAL);

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

  for (int i=0; i < AVB_1722_1_MAX_ENTITIES; i++)
  {
    if (entities[i].guid.l != 0)
    {
      // Check to see if we are the only XMOS talker on the network
      if (((entities[i].talker_capabilities & AVB_1722_1_ADP_TALKER_CAPABILITIES_AUDIO_SOURCE)
           == AVB_1722_1_ADP_TALKER_CAPABILITIES_AUDIO_SOURCE))
      {
        break;
      }
    }

    if (i == AVB_1722_1_MAX_ENTITIES-1)
    {
      // Set us to the master audio clock
      set_device_media_clock_type(0, LOCAL_CLOCK);
      printstrln("Entity elected Master audio clock");
      entity_elected_master_clock = 1;
    }
  }

}