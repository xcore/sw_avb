#include <xccompat.h>
#include <print.h>
#include "simple_printf.h"
#include "avb.h"
#include "avb_conf.h"
#include "avb_1722_common.h"
#include "avb_control_types.h"
#if AVB_ENABLE_1722_1
#include "avb_1722_1_common.h"
#include "avb_1722_1_protocol.h"
#include "avb_1722_1_acmp.h"
#include "avb_1722_1_adp.h"
#endif
#include "avb_1722_1_app_hooks.h"

/*** ADP ***/
void __attribute__((weak)) avb_entity_on_new_entity_available(guid_t *my_guid, avb_1722_1_entity_record *entity, chanend c_tx)
{
  // Do nothing in the core stack 
}


/*** ACMP ***/

/* The controller has indicated that a listener is connecting to this talker stream */
void __attribute__((weak)) avb_talker_on_listener_connect(int source_num, guid_t *listener_guid)
{
  unsigned stream_id[2];
  enum avb_source_state_t state;
  get_avb_source_state(source_num, &state);
  get_avb_source_id(source_num, stream_id);

  simple_printf("CONNECTING Talker stream #%d (%x%x) -> Listener ", source_num, stream_id[0], stream_id[1]); print_guid_ln(listener_guid);

  // If this is the first listener to connect to this talker stream, we do a stream registration
  // to reserve the necessary bandwidth on the network
  if (state == AVB_SOURCE_STATE_DISABLED)
  {
    get_avb_source_id(source_num, stream_id);
    avb_1722_1_talker_set_stream_id(source_num, stream_id);

    set_avb_source_state(source_num, AVB_SOURCE_STATE_POTENTIAL);
  }

  return;

}

/* The controller has indicated that a listener is disconnecting from this talker stream */
void __attribute__((weak)) avb_talker_on_listener_disconnect(int source_num, guid_t *listener_guid, int connection_count)
{
  unsigned stream_id[2];
  enum avb_source_state_t state;
  get_avb_source_state(source_num, &state);
  get_avb_source_id(source_num, stream_id);

  simple_printf("DISCONNECTING Talker stream #%d (%x%x) -> Listener ", source_num, stream_id[0], stream_id[1]); print_guid_ln(listener_guid);

  if ((state > AVB_SOURCE_STATE_DISABLED) && (connection_count == 0))
  {
    set_avb_source_state(source_num, AVB_SOURCE_STATE_DISABLED);
  }
}

/* The controller has indicated to connect this listener sink to a talker stream */
void __attribute__((weak)) avb_listener_on_talker_connect(int sink_num, guid_t *talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], guid_t *my_guid)
{
  int map[AVB_NUM_MEDIA_OUTPUTS];
  for (int i = 0; i < AVB_NUM_MEDIA_OUTPUTS; i++) map[i] = i;

  simple_printf("CONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  set_avb_sink_sync(sink_num, 0);
  set_avb_sink_channels(sink_num, AVB_NUM_MEDIA_OUTPUTS);
  set_avb_sink_map(sink_num, map, AVB_NUM_MEDIA_OUTPUTS);
  set_avb_sink_id(sink_num, stream_id);
  set_avb_sink_addr(sink_num, dest_addr, 6);

  set_avb_sink_state(sink_num, AVB_SINK_STATE_POTENTIAL);

}

/* The controller has indiscated to disconnect this listener sink from a talker stream */
void __attribute__((weak)) avb_listener_on_talker_disconnect(int sink_num, guid_t *talker_guid, unsigned char dest_addr[6], unsigned int stream_id[2], guid_t *my_guid)
{
  simple_printf("DISCONNECTING Listener sink #%d -> Talker stream %x%x, DA: ", sink_num, stream_id[0], stream_id[1]); print_mac_ln(dest_addr);

  set_avb_sink_state(sink_num, AVB_SINK_STATE_DISABLED);
}