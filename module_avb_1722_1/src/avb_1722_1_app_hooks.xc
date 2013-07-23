#include <xccompat.h>
#include <print.h>
#include "simple_printf.h"
#include "avb.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_control_types.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_protocol.h"
#include "avb_1722_1_acmp.h"
#include "avb_1722_1_adp.h"
#include "avb_1722_1_app_hooks.h"

#if 0
/*** ADP ***/
void __attribute__((weak)) avb_entity_on_new_entity_available(client interface avb_interface avb, const_guid_ref_t my_guid, avb_1722_1_entity_record *entity, chanend c_tx)
{
  // Do nothing in the core stack 
}

/*** ACMP ***/

/* The controller has indicated that a listener is connecting to this talker stream */
void __attribute__((weak)) avb_talker_on_listener_connect(client interface avb_interface avb, int source_num, const_guid_ref_t listener_guid)
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

/* The controller has indicated that a listener is disconnecting from this talker stream */
void __attribute__((weak)) avb_talker_on_listener_disconnect(client interface avb_interface avb, int source_num, const_guid_ref_t listener_guid, int connection_count)
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

/* The controller has indicated to connect this listener sink to a talker stream */
avb_1722_1_acmp_status_t __attribute__((weak)) avb_listener_on_talker_connect(client interface avb_interface avb, int sink_num, const_guid_ref_t talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], const_guid_ref_t my_guid)
{
  int map[AVB_NUM_MEDIA_OUTPUTS];
  for (int i = 0; i < AVB_NUM_MEDIA_OUTPUTS; i++) map[i] = i;

  simple_printf("CONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  avb.set_sink_sync(sink_num, 0);
  avb.set_sink_channels(sink_num, AVB_NUM_MEDIA_OUTPUTS);
  avb.set_sink_map(sink_num, map, AVB_NUM_MEDIA_OUTPUTS);
  avb.set_sink_id(sink_num, stream_id);
  avb.set_sink_addr(sink_num, dest_addr, 6);

  avb.set_sink_state(sink_num, AVB_SINK_STATE_POTENTIAL);

  return ACMP_STATUS_SUCCESS;
}

/* The controller has indicated to disconnect this listener sink from a talker stream */
void __attribute__((weak)) avb_listener_on_talker_disconnect(client interface avb_interface avb, int sink_num, const_guid_ref_t talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], const_guid_ref_t my_guid)
{
  simple_printf("DISCONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  avb.set_sink_state(sink_num, AVB_SINK_STATE_DISABLED);
}

/* The controller has indicated that a listener has returned an error on connection attempt */
void __attribute__((weak)) avb_talker_on_listener_connect_failed(client interface avb_interface avb, const_guid_ref_t my_guid, int source_num,
        const_guid_ref_t listener_guid, avb_1722_1_acmp_status_t status, chanend c_tx)
{
    // Do nothing
}

#endif