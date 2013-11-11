#include <xclib.h>
#include <string.h>
#include "avb.h"
#include "avb_conf.h"
#include "avb_srp.h"
#include "avb_mrp_pdu.h"
#include "avb_srp_pdu.h"
#include "avb_stream.h"
#include "avb_1722_talker.h"
#include <print.h>
#include "avb_control_types.h"
#include "stdlib.h"
#include "simple_printf.h"
#include "avb_1722_router.h"
#include "avb_api.h"
#include "ethernet_tx_client.h"

/* This needs to be greater than the actual max number of handled streams, because SRP
   cannot remove the attributes as quickly as a connection can be torn down and setup
   again on the Mac. The Mac uses a new set of stream IDs, presumably because the old
   attributes are still hanging around in their implementation also.
   Hence there will be a period where the number of active stream entries is greater
   than the max capable number until they are cleaned up.
*/
#ifndef AVB_STREAM_TABLE_ENTRIES
#define AVB_STREAM_TABLE_ENTRIES (8+4)
#endif

typedef struct avb_stream_entry
{
  avb_srp_info_t reservation;
  int listener_present;
  int talker_present;
  int bw_reserved[MRP_NUM_PORTS]; // While the bw_reserved flag is set/not set we do not add/subtract Qav credit
} avb_stream_entry;

static avb_stream_entry stream_table[AVB_STREAM_TABLE_ENTRIES];
static unsigned int port_bandwidth[MRP_NUM_PORTS];

static mrp_attribute_state *domain_attr[MRP_NUM_PORTS];

static chanend c_mac_tx;

void srp_store_mac_tx_chanend(chanend c_mac_tx0) {
  c_mac_tx = c_mac_tx0;
}

void srp_domain_init(void) {
  for(int i=0; i < MRP_NUM_PORTS; i++)
  {
    domain_attr[i] = mrp_get_attr();
    mrp_attribute_init(domain_attr[i], MSRP_DOMAIN_VECTOR, i, 1, NULL);
  }
}

void srp_domain_join(void)
{
  for (int i=0; i < MRP_NUM_PORTS; i++)
  {
      mrp_mad_begin(domain_attr[i]);
      mrp_mad_join(domain_attr[i], 1);
  }
}

static int srp_calculate_stream_bandwidth(int max_frame_size, int extra_byte) {
  const int interframe_gap = 12;
  const int preamble_sfd = 8;
  const int eth_header_and_tag = 18;
  const int crc = 4;
  const int total_frame_size = interframe_gap + preamble_sfd + eth_header_and_tag + max_frame_size + crc + extra_byte;

  return total_frame_size * 8 * AVB1722_PACKET_RATE;
}

static void srp_increase_port_bandwidth(int max_frame_size, int extra_byte, int port) {
  int stream_bandwidth_bps = srp_calculate_stream_bandwidth(max_frame_size, extra_byte);
  port_bandwidth[port] += stream_bandwidth_bps;
  simple_printf("Increasing port %d shaper bandwidth to %d\n", port, port_bandwidth[port]);
  mac_set_qav_bandwidth(c_mac_tx, port, port_bandwidth[port]);
}

static void srp_decrease_port_bandwidth(int max_frame_size, int extra_byte, int port) {
  int stream_bandwidth_bps = srp_calculate_stream_bandwidth(max_frame_size, extra_byte);
  port_bandwidth[port] -= stream_bandwidth_bps;
   simple_printf("Decreasing port %d shaper bandwidth to %d\n", port, port_bandwidth[port]);
  mac_set_qav_bandwidth(c_mac_tx, port, port_bandwidth[port]);
}

int avb_srp_match_listener_to_talker_stream_id(unsigned stream_id[2], avb_srp_info_t **stream, int is_listener)
{
  for(int i=0;i<AVB_STREAM_TABLE_ENTRIES;i++)
  {
    if (((is_listener && stream_table[i].talker_present == 1) ||
        (!is_listener && stream_table[i].listener_present == 1)) &&
        stream_id[0] == stream_table[i].reservation.stream_id[0] &&
        stream_id[1] == stream_table[i].reservation.stream_id[1]) {
      if (stream != NULL)
      {
        *stream = &stream_table[i].reservation;
      }
      return 1;
    }
  }

  return 0;
}

// Either return an index to update, or a new index if not matched, or -1 if no entries free
static int srp_match_reservation_entry_by_id(unsigned stream_id[2]) {
  int empty_index = -1;
  for(int i=0;i<AVB_STREAM_TABLE_ENTRIES;i++)
  {
    if (stream_id[0] == stream_table[i].reservation.stream_id[0] &&
        stream_id[1] == stream_table[i].reservation.stream_id[1]) {
      return i;
    }
    if (stream_table[i].reservation.stream_id[0] == 0 &&
        stream_table[i].reservation.stream_id[1] == 0) {
      empty_index = i;
    }
  }
  return empty_index;
}

avb_stream_entry *srp_add_reservation_entry_stream_id_only(unsigned int stream_id[2]) {
  int entry = srp_match_reservation_entry_by_id(stream_id);

  if (entry >= 0) {
    if (!stream_table[entry].talker_present) memset(&stream_table[entry].reservation, 0, sizeof(avb_srp_info_t));
    stream_table[entry].reservation.stream_id[0] = stream_id[0];
    stream_table[entry].reservation.stream_id[1] = stream_id[1];
    stream_table[entry].listener_present = 1;
    simple_printf("Added stream:\n ID: %x%x\n", stream_id[0], stream_id[1]);
  } else {
    printstrln("Assert: Out of stream entries");
    __builtin_trap();
    return NULL;
  }

  return &stream_table[entry];
}

avb_stream_entry *srp_add_reservation_entry(avb_srp_info_t *reservation) {
  int entry = srp_match_reservation_entry_by_id(reservation->stream_id);

  if (entry >= 0) {
    memcpy(&stream_table[entry].reservation, reservation, sizeof(avb_srp_info_t));
    simple_printf("Added stream:\n ID: %x%x\n DA:", reservation->stream_id[0], reservation->stream_id[1]);
    for (int i=0; i < 6; i++) {
      printhex(stream_table[entry].reservation.dest_mac_addr[i]); printchar(':');
    }
    simple_printf("\n max size: %d\n interval: %d\n",
                stream_table[entry].reservation.tspec_max_frame_size,
                stream_table[entry].reservation.tspec_max_interval
                );
    stream_table[entry].talker_present = 1;
  } else {
    printstrln("Assert: Out of stream entries");
    // __builtin_trap();
    return NULL;
  }

  return &stream_table[entry];
}

void srp_remove_reservation_entry(avb_srp_info_t *reservation) {
  int entry = srp_match_reservation_entry_by_id(reservation->stream_id);

  if (entry >= 0) {
    simple_printf("Removed stream:\n ID: %x%x\n", reservation->stream_id[0], reservation->stream_id[1]);
    if (stream_table[entry].reservation.stream_id[0] < 10 && stream_table[entry].reservation.stream_id[1] < 5) {
      __builtin_trap();
    }
    memset(&stream_table[entry], 0x00, sizeof(avb_stream_entry));
  } else {
    simple_printf("Assert: Tried to remove a reservation that isn't stored: %x%d", reservation->stream_id[0], reservation->stream_id[1]);
    __builtin_trap();
  }
}

void srp_cleanup_reservation_entry(mrp_attribute_state *st) {
  if (st->attribute_type == MSRP_TALKER_ADVERTISE ||
      st->attribute_type == MSRP_TALKER_FAILED ||
      st->attribute_type == MSRP_LISTENER) {
    mrp_attribute_state *matched1 = mrp_match_attribute_pair_by_stream_id(st, 1, 0);
    mrp_attribute_state *matched2 = mrp_match_attr_by_stream_and_type(st, 1);

    // If there is no match, it is either because there is genuinely no attribute match or if the reservation entry has been zeroed
    // Hence the stream ID is zero
    if (!matched1 && !matched2) {
      avb_srp_info_t *attribute_info = st->attribute_info;
      if (attribute_info == NULL) __builtin_trap();

      if (attribute_info->stream_id[0] != 0 && attribute_info->stream_id[1] != 0) {
        avb_1722_remove_stream_from_table(c_mac_tx, attribute_info->stream_id);
        srp_remove_reservation_entry(attribute_info);
      }
    }
  }
}

static void create_propagated_attribute_and_join(mrp_attribute_state *attr, int new) {
  mrp_attribute_state *st = mrp_get_attr();
  avb_srp_info_t *stream_data = attr->attribute_info;
  mrp_attribute_init(st, attr->attribute_type, !attr->port_num, 0, stream_data);
#if 0
  simple_printf("JOIN mrp_attribute_init: %d, %d, STREAM_ID[0]: %x\n", attr->attribute_type, !attr->port_num, stream_data->stream_id[0]);
#endif
  mrp_mad_begin(st);
  mrp_mad_join(st, new);
  st->propagated = 1; // Propagated to other port
}


// TODO: Remove listener bool param and just use the attribute type from the state structure
static void avb_srp_map_join(mrp_attribute_state *attr, int new, int listener)
{
  avb_srp_info_t *attribute_info = attr->attribute_info;
#if 0
  if (listener) printstrln("Listener MAP_Join.indication");
  else printstrln("Talker MAP_Join.indication");
#endif
  mrp_attribute_state *matched_talker_listener = mrp_match_attribute_pair_by_stream_id(attr, 1, 0);
  mrp_attribute_state *matched_stream_id_opposite_port = mrp_match_attr_by_stream_and_type(attr, 1);

#if 0
  simple_printf("matched_talker_listener: %d(here:%d, prop:%d, new:%d), matched_stream_id_opposite_port: %d\n", matched_talker_listener,
   matched_talker_listener ? matched_talker_listener->propagated : 0, matched_talker_listener ? matched_talker_listener->here : 0, new, matched_stream_id_opposite_port);
#endif
  // Attribute propagation:
  if (!matched_stream_id_opposite_port && !listener)
  {
    create_propagated_attribute_and_join(attr, new);
  }
  else if (!matched_stream_id_opposite_port &&
            matched_talker_listener &&
            !matched_talker_listener->propagated &&
            !matched_talker_listener->here)
  {
    create_propagated_attribute_and_join(attr, new);
  }

  /**** LISTENER ****/
  if (listener && matched_talker_listener && !matched_talker_listener->propagated) {
    if (!matched_talker_listener->here) { // Handle case where the Talker is not this endpoint

      if (!matched_talker_listener->here) {
        int entry = srp_match_reservation_entry_by_id(attribute_info->stream_id);
        if (!stream_table[entry].bw_reserved[attr->port_num]) {
          stream_table[entry].bw_reserved[attr->port_num] = 1;
          srp_increase_port_bandwidth(attribute_info->tspec_max_frame_size, 1, attr->port_num);
          avb_1722_enable_stream_forwarding(c_mac_tx, attribute_info->stream_id);
        }
      }
      if (matched_stream_id_opposite_port)
      {
        mrp_mad_join(matched_stream_id_opposite_port, 1);
        matched_stream_id_opposite_port->propagated = 1; // Propagate to other port
      }
    }
  }
  /**************/

  /**** TALKER ****/
  if (!listener && matched_stream_id_opposite_port) {
    mrp_mad_join(matched_stream_id_opposite_port, 1);
    matched_stream_id_opposite_port->propagated = 1; // Propagate to other port
  }
  /**************/

  mrp_debug_dump_attrs();

}

void avb_srp_map_leave(mrp_attribute_state *attr)
{
#if 0
  if (attr->attribute_type == MSRP_LISTENER) printstrln("Listener MAP_Leave.indication");
  else if (attr->attribute_type == MSRP_TALKER_ADVERTISE) printstrln("Talker MAP_Leave.indication");
#endif
  mrp_attribute_state *matched_talker_listener = mrp_match_attribute_pair_by_stream_id(attr, 1, 0);
  mrp_attribute_state *matched_stream_id_opposite_port = mrp_match_attr_by_stream_and_type(attr, 1);

  mrp_debug_dump_attrs();

  if (attr->attribute_type == MSRP_LISTENER)
  {
    avb_srp_info_t *attribute_info = attr->attribute_info;
    int entry = srp_match_reservation_entry_by_id(attribute_info->stream_id);

    if (matched_stream_id_opposite_port) {
      if (stream_table[entry].bw_reserved[attr->port_num]) {
        srp_decrease_port_bandwidth(attribute_info->tspec_max_frame_size, 1, attr->port_num);
        avb_1722_disable_stream_forwarding(c_mac_tx, attribute_info->stream_id);
        stream_table[entry].bw_reserved[attr->port_num] = 0;
        // Propagate Listener leave:
        mrp_mad_leave(matched_stream_id_opposite_port);
      }
    }
  }
  else if (attr->attribute_type == MSRP_TALKER_ADVERTISE)
  {
    if (matched_stream_id_opposite_port) {
      // Propagate Talker leave:
      mrp_mad_leave(matched_stream_id_opposite_port);
    }

    if (matched_talker_listener) {
      /* Special case of behaviour described in 25.3.4.4.1.
       * "the Bridge shall act as a proxy for the Listener(s) and automatically generate a
       *  MAD_Leave.request back toward the Talker for those Listener attributes"
       */
      mrp_mad_leave(matched_talker_listener);
    }
  }
}

int avb_srp_match_talker_failed(mrp_attribute_state *attr,
                                char *msg,
                                int i)
{
  return avb_srp_match_talker_advertise(attr, msg, i);
}

int avb_srp_match_talker_advertise(mrp_attribute_state *attr,
                                   char *fv,
                                   int i)
{
  avb_source_info_t *source_info = (avb_source_info_t *) attr->attribute_info;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_talker_first_value *first_value = (srp_talker_first_value *) fv;

  if (source_info == NULL) return 0;

  my_stream_id = source_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + source_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += i;


  return (my_stream_id == stream_id);
}

int avb_srp_match_listener(mrp_attribute_state *attr,
                           char *fv,
                           int i,
                           int four_packed_event)
{
  avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_listener_first_value *first_value = (srp_listener_first_value *) fv;

  if (sink_info == NULL) return 0;

  /*
  if (four_packed_event != AVB_SRP_FOUR_PACKED_EVENT_READY)
    return 0;
  */

  my_stream_id = sink_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + sink_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += i;

  return (my_stream_id == stream_id);
}

int avb_srp_match_domain(mrp_attribute_state *attr,char *fv,int i)
{
  // This returns zero becauase we don't expect to have to merge/aggragate domain messages
  return 0;
}

void avb_srp_listener_join_ind(CLIENT_INTERFACE(avb_interface, avb), mrp_attribute_state *attr, int new, int four_packed_event)
{
  enum avb_source_state_t state;
  avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
  unsigned stream = avb_get_source_stream_index_from_stream_id(sink_info->reservation.stream_id);

  if (MRP_NUM_PORTS == 2) {
    avb_srp_map_join(attr, new, 1);
  }

  if (stream != -1u) {

    avb_get_source_state(avb, stream, &state);

    int entry = srp_match_reservation_entry_by_id(sink_info->reservation.stream_id);

    if (mrp_match_attr_by_stream_and_type(attr, 1)) { // Listener ready on the other port also, therefore send on both ports
      if (stream_table[entry].bw_reserved[!attr->port_num] != 1) {
        srp_increase_port_bandwidth(sink_info->reservation.tspec_max_frame_size, 0, !attr->port_num);
        set_avb_source_port(stream, -1);
        stream_table[entry].bw_reserved[!attr->port_num] = 1;
      }
    }
    else { // Just this port
      if (stream_table[entry].bw_reserved[attr->port_num] != 1) {
        srp_increase_port_bandwidth(sink_info->reservation.tspec_max_frame_size, 0, attr->port_num);
        set_avb_source_port(stream, attr->port_num);
        stream_table[entry].bw_reserved[attr->port_num] = 1;
      }
    }

    if (state == AVB_SOURCE_STATE_POTENTIAL) {
      if (four_packed_event == AVB_SRP_FOUR_PACKED_EVENT_READY ||
        four_packed_event == AVB_SRP_FOUR_PACKED_EVENT_READY_FAILED) {
  #if SRP_AUTO_TALKER_STREAM_CONTROL
        avb_set_source_state(avb, stream, AVB_SOURCE_STATE_ENABLED);
  #else
  #endif
      }
    }
  }
}

void avb_srp_listener_leave_ind(CLIENT_INTERFACE(avb_interface, avb), mrp_attribute_state *attr, int four_packed_event)
{
  enum avb_source_state_t state;
  avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
  unsigned stream = avb_get_source_stream_index_from_stream_id(sink_info->reservation.stream_id);
  mrp_attribute_state *matched_listener_opposite_port = mrp_match_attr_by_stream_and_type(attr, 1);

  int entry = srp_match_reservation_entry_by_id(sink_info->reservation.stream_id);

  if (MRP_NUM_PORTS == 2) {
    avb_srp_map_leave(attr);
  }

  if (stream != -1u)
  {
      if (stream_table[entry].bw_reserved[attr->port_num] == 1) {
        srp_decrease_port_bandwidth(sink_info->reservation.tspec_max_frame_size, 0, attr->port_num);
        if (matched_listener_opposite_port) { // Transmitting on both ports
          set_avb_source_port(stream, !attr->port_num);
        }
        stream_table[entry].bw_reserved[attr->port_num] = 0;
      }
    avb_get_source_state(avb, stream, &state);

    if (state == AVB_SOURCE_STATE_ENABLED && !matched_listener_opposite_port) {
  #if SRP_AUTO_TALKER_STREAM_CONTROL
      avb_set_source_state(avb, stream, AVB_SOURCE_STATE_POTENTIAL);
      if (stream_table[entry].bw_reserved[attr->port_num] == 1) {
        srp_decrease_port_bandwidth(sink_info->reservation.tspec_max_frame_size, 0, attr->port_num);
        stream_table[entry].bw_reserved[attr->port_num] = 0;
      }
  #else
  #endif
   }
  }
}

void avb_srp_leave_talker_attrs(unsigned int stream_id[2]) {
  for (int i=0; i < MRP_NUM_PORTS; i++) {
    mrp_attribute_state *matched_talker_advertise = mrp_match_talker_non_prop_attribute(stream_id, i);

    if (matched_talker_advertise) {
      mrp_mad_leave(matched_talker_advertise);
    }
  }

}

void avb_srp_create_and_join_talker_advertise_attrs(avb_srp_info_t *reservation) {
  avb_stream_entry *stream_ptr = srp_add_reservation_entry(reservation);

  // FIXME: Can we get duplication of Talker attributes here?

  for (int i=0; i < MRP_NUM_PORTS; i++) {
    mrp_attribute_state *attr = mrp_get_attr();
    mrp_attribute_init(attr, MSRP_TALKER_ADVERTISE, i, 1, stream_ptr);
    mrp_mad_begin(attr);
    mrp_mad_join(attr, 1);
  }
  mrp_debug_dump_attrs();
}


void avb_srp_leave_listener_attrs(unsigned int stream_id[2]) {
  // LL1: Find Talker advertise attribute that has not been propagated
  mrp_attribute_state *matched_talker_advertise = mrp_match_talker_non_prop_attribute(stream_id, -1);

  if (matched_talker_advertise) {
    mrp_attribute_state *matched_listener_opposite_port = mrp_match_attribute_pair_by_stream_id(matched_talker_advertise, 1, 0);
    mrp_attribute_state *matched_listener_this_port = mrp_match_attribute_pair_by_stream_id(matched_talker_advertise, 0, 0);

    // LL2: If there is a non-propagated Listener attr on the opposite port, then there is another Listener
    // Do not leave
    if (matched_listener_opposite_port) {
      // Do nothing
    }
    else {
      // LL3: Else do leave on the Listener
      if (matched_listener_this_port) {
        mrp_mad_leave(matched_listener_this_port);
      }
    }
  }
}

/* LJN: Listener Join
*/

void avb_srp_join_listener_attrs(unsigned int stream_id[2]) {
  // LJ1. Find Talker advertise attribute that has not been propagated
  mrp_attribute_state *matched_talker_advertise = mrp_match_talker_non_prop_attribute(stream_id, -1);

  if (matched_talker_advertise) {
    mrp_attribute_state *matched_listener_same_port = mrp_match_attribute_pair_by_stream_id(matched_talker_advertise, 0, 0);

    // LJ2. If Listener Ready attribute not present on this port, create it
    if (!matched_listener_same_port) {
        matched_listener_same_port = mrp_get_attr();
        mrp_attribute_init(matched_listener_same_port,
                           MSRP_LISTENER,
                           matched_talker_advertise->port_num,
                           1,
                           matched_talker_advertise->attribute_info);
        mrp_mad_begin(matched_listener_same_port);
    }

    // LJ3. Join Listener Ready on the Talker advertise port
    mrp_mad_join(matched_listener_same_port, 1);
  }
  else { // LJ4: If the Talker advertise hasn't matched, then it probably hasn't arrived yet
    // LJ5: Create Listener attrs on both ports but leave them disabled
    avb_stream_entry *stream_ptr = srp_add_reservation_entry_stream_id_only(stream_id);
    for (int i=0; i < MRP_NUM_PORTS; i++) {
        mrp_attribute_state *listener_attr = mrp_get_attr();
        mrp_attribute_init(listener_attr,
                           MSRP_LISTENER,
                           i,
                           1,
                           stream_ptr);
    }
  }

  mrp_debug_dump_attrs();
}


mrp_attribute_state* avb_srp_process_new_attribute_from_packet(int mrp_attribute_type, char *fv, int num, int port_num)
{
  srp_talker_first_value *packet = (srp_talker_first_value *) fv;
  unsigned int pdu_streamId[2];
  avb_stream_entry *stream_ptr = NULL;

  unsigned long long streamId;

  for (int i=0;i<8;i++)
    streamId = (streamId << 8) + packet->StreamId[i];

  streamId += num;

  pdu_streamId[0] = streamId >> 32;
  pdu_streamId[1] = (unsigned) streamId;

  avb_srp_info_t reservation;

  reservation.stream_id[0] = pdu_streamId[0];
  reservation.stream_id[1] = pdu_streamId[1];


  switch (mrp_attribute_type)
  {
    case MSRP_TALKER_ADVERTISE:
    case MSRP_TALKER_FAILED:
    {
      unsigned long long x;
      reservation.vlan_id = ntoh_16(packet->VlanID);

      for(int i=0;i<6;i++)
        x = (x<<8) + packet->DestMacAddr[i];

      x += num;

      int tmp = byterev(x);
      memcpy(&reservation.dest_mac_addr[2], &tmp, 4);
      tmp = byterev(x>>32)>>16;
      memcpy(&reservation.dest_mac_addr, &tmp, 2);

      reservation.tspec_max_frame_size = ntoh_16(packet->TSpecMaxFrameSize);
      reservation.tspec_max_interval = ntoh_16(packet->TSpecMaxIntervalFrames);
      reservation.tspec = packet->TSpec;
      reservation.accumulated_latency = ntoh_32(packet->AccumulatedLatency);
      stream_ptr = srp_add_reservation_entry(&reservation);
      break;
    }
    case MSRP_LISTENER:
    {
      stream_ptr = srp_add_reservation_entry_stream_id_only(pdu_streamId);
      break;
    }
    default:
      __builtin_trap();
      break;
  }

  if (stream_ptr) {
    mrp_attribute_state *st = mrp_get_attr();
    mrp_attribute_init(st, mrp_attribute_type, port_num, 0, stream_ptr);

    return st;
  }
  else {
    return NULL;
  }
}

void avb_srp_talker_join_ind(mrp_attribute_state *attr, int new)
{
  if (MRP_NUM_PORTS == 2) {
    avb_sink_info_t *sink_info = (avb_sink_info_t *) attr->attribute_info;
    unsigned stream = avb_get_sink_stream_index_from_stream_id(sink_info->reservation.stream_id);
    mrp_attribute_state *matched_listener_this_port = mrp_match_attribute_pair_by_stream_id(attr, 0, 1);

    /* This covers the case where the Listener joins before the Talker attribute. When we receive the Talker join new,
     * we also trigger join for the Listener attribute on the same port if it already exists.
     * We also need to mark the disabled Listener attr we created on the other port as unused
     */
    if (stream != -1u && matched_listener_this_port)
    {
      mrp_attribute_state *matched_listener_opposite_port = mrp_match_attribute_pair_by_stream_id(attr, 1, 1);
      if (matched_listener_opposite_port) {
        mrp_change_applicant_state(matched_listener_opposite_port, MRP_EVENT_DUMMY, MRP_UNUSED);
      }

      mrp_mad_begin(matched_listener_this_port);
      mrp_mad_join(matched_listener_this_port, 1);
    }

    avb_srp_map_join(attr, new, 0);
  }
}

void avb_srp_talker_leave_ind(mrp_attribute_state *attr)
{
  if (MRP_NUM_PORTS == 2) {
    unsigned stream = avb_get_sink_stream_index_from_pointer(attr->attribute_info);
    if (stream != -1u) {
      // This could be used to report talker advertising instead of the snooping scheme above
    }
    else
    {
      avb_srp_map_leave(attr);
    }
  }
}

static int check_listener_firstvalue_merge(char *buf,
                                avb_sink_info_t *sink_info)
{
  mrp_vector_header *hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));
  int num_values = hdr->NumberOfValuesLow;
  unsigned long long stream_id=0, my_stream_id=0;
  srp_listener_first_value *first_value =
    (srp_listener_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

  // check if we can merge
  my_stream_id = sink_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + sink_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += num_values;


  if (my_stream_id != stream_id)
    return 0;

  return 1;

}


static int encode_listener_message(char *buf,
                                  mrp_attribute_state *st,
                                  int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr =
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));
  int merge = 0;
  avb_sink_info_t *sink_info = st->attribute_info;

  int num_values;

  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_LISTENER)
    return 0;

  num_values = hdr->NumberOfValuesLow;

  if (num_values == 0)
    merge = 1;
  else
    merge = check_listener_firstvalue_merge(buf, sink_info);


  if (merge) {
    srp_listener_first_value *first_value =
      (srp_listener_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));
    unsigned *streamId = sink_info->reservation.stream_id;
    unsigned int streamid;

    if (num_values == 0) {
      streamid = byterev(streamId[0]);
      memcpy(&first_value->StreamId[0], &streamid, 4);
      streamid = byterev(streamId[1]);
      memcpy(&first_value->StreamId[4], &streamid, 4);

      int port_to_transmit = st->port_num;
      if (MRP_DEBUG_ATTR_EGRESS)
      {
        simple_printf("Port %d out: MSRP_LISTENER, stream %x:%x\n", port_to_transmit, streamId[0], streamId[1]);
      }

    }

    mrp_encode_three_packed_event(buf, vector, st->attribute_type);
    mrp_encode_four_packed_event(buf, AVB_SRP_FOUR_PACKED_EVENT_READY, st->attribute_type);

    hdr->NumberOfValuesLow = num_values+1;

  }

  return merge;
}

void avb_srp_domain_join_ind(mrp_attribute_state *attr, int new)
{
  //printstr("SRP Domain join ind\n");
}

void avb_srp_domain_leave_ind(mrp_attribute_state *attr)
{
  //printstr("SRP domain leave ind\n");
}

static int check_domain_firstvalue_merge(char *buf) {
  // We never both to merge domain attribute together
  return 0;
}

static int encode_domain_message(char *buf,
                                mrp_attribute_state *st,
                                int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr =
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));
  int merge = 0;
  int num_values;


  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_DOMAIN)
    return 0;


  num_values = hdr->NumberOfValuesLow;

  if (num_values == 0)
    merge = 1;
  else
    merge = check_domain_firstvalue_merge(buf);

  if (merge) {
    srp_domain_first_value *first_value =
      (srp_domain_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

    first_value->SRclassID = AVB_SRP_SRCLASS_DEFAULT;
    first_value->SRclassPriority = AVB_SRP_TSPEC_PRIORITY_DEFAULT;
    first_value->SRclassVID[0] = (AVB_DEFAULT_VLAN>>8)&0xff;
    first_value->SRclassVID[1] = (AVB_DEFAULT_VLAN&0xff);

    mrp_encode_three_packed_event(buf, vector, st->attribute_type);

    hdr->NumberOfValuesLow = num_values+1;
  }

  return merge;
}


static int check_talker_firstvalue_merge(char *buf,
                              avb_source_info_t *source_info)
{
  mrp_vector_header *hdr = (mrp_vector_header *) (buf + sizeof(mrp_msg_header));
  int num_values = hdr->NumberOfValuesLow;
  unsigned long long stream_id=0, my_stream_id=0;
  unsigned long long dest_addr=0, my_dest_addr=0;
  int framesize=0, my_framesize=0;
  int vlan, my_vlan;
  srp_talker_first_value *first_value =
    (srp_talker_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

  // check if we can merge

  for (int i=0;i<6;i++) {
    my_dest_addr = (my_dest_addr << 8) + source_info->reservation.dest_mac_addr[i];
    dest_addr = (dest_addr << 8) + first_value->DestMacAddr[i];
  }

  dest_addr += num_values;

  if (dest_addr != my_dest_addr)
    return 0;

  // check if we can merge
  my_stream_id = source_info->reservation.stream_id[0];
  my_stream_id = (my_stream_id << 32) + source_info->reservation.stream_id[1];

  for (int i=0;i<8;i++) {
    stream_id = (stream_id << 8) + first_value->StreamId[i];
  }

  stream_id += num_values;

  if (my_stream_id != stream_id)
    return 0;


  vlan = ntoh_16(first_value->VlanID);
  my_vlan = source_info->reservation.vlan_id;

  if (vlan != my_vlan)
    return 0;

  my_framesize = source_info->reservation.tspec_max_frame_size;
  framesize = ntoh_16(first_value->TSpecMaxFrameSize);

  if (framesize != my_framesize)
    return 0;


  return 1;

}

static int encode_talker_message(char *buf,
                                mrp_attribute_state *st,
                                int vector)
{
  mrp_msg_header *mrp_hdr = (mrp_msg_header *) buf;
  mrp_vector_header *hdr =
    (mrp_vector_header *) (buf + sizeof(mrp_msg_header));
  int merge = 0;
  avb_source_info_t *source_info = st->attribute_info;
  avb_srp_info_t *attribute_info;
  int num_values;

  if (mrp_hdr->AttributeType != AVB_SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE)
    return 0;

  if (st->here)
    attribute_info = &source_info->reservation;
  else
    attribute_info = st->attribute_info;

  num_values = hdr->NumberOfValuesLow;

  if (num_values == 0)
    merge = 1;
  else
    merge = check_talker_firstvalue_merge(buf, source_info);



  if (merge) {
    srp_talker_first_value *first_value =
      (srp_talker_first_value *) (buf + sizeof(mrp_msg_header) + sizeof(mrp_vector_header));

    // The SRP layer

    if (num_values == 0) {
      unsigned int streamid;
      for (int i=0;i<6;i++) {
        first_value->DestMacAddr[i] = attribute_info->dest_mac_addr[i];
      }

      streamid = byterev(attribute_info->stream_id[0]);
      memcpy(&first_value->StreamId[0], &streamid, 4);
      streamid = byterev(attribute_info->stream_id[1]);
      memcpy(&first_value->StreamId[4], &streamid, 4);

      int port_to_transmit = st->port_num;
      if (MRP_DEBUG_ATTR_EGRESS)
      {
        simple_printf("Port %d out: MSRP_TALKER_ADVERTISE, stream %x:%x\n", port_to_transmit, attribute_info->stream_id[0], attribute_info->stream_id[1]);
      }

      hton_16(first_value->VlanID, attribute_info->vlan_id);
      first_value->TSpec = attribute_info->tspec;
      hton_16(first_value->TSpecMaxFrameSize, attribute_info->tspec_max_frame_size);
      hton_16(first_value->TSpecMaxIntervalFrames,
                 attribute_info->tspec_max_interval);
      hton_32(first_value->AccumulatedLatency,
                 attribute_info->accumulated_latency);


    }

    mrp_encode_three_packed_event(buf, vector, st->attribute_type);

    hdr->NumberOfValuesLow = num_values+1;

  }

  return merge;
}







int avb_srp_encode_message(char *buf,
                          mrp_attribute_state *st,
                          int vector)
{
  switch (st->attribute_type) {
  case MSRP_TALKER_ADVERTISE:
    return encode_talker_message(buf, st, vector);
    break;
  case MSRP_LISTENER:
    return encode_listener_message(buf, st, vector);
    break;
  case MSRP_DOMAIN_VECTOR:
    return encode_domain_message(buf, st, vector);
    break;

  default:
    break;
  }
  return 0;
}


int avb_srp_compare_talker_attributes(mrp_attribute_state *a,
                                      mrp_attribute_state *b)
{
  avb_stream_info_t *source_info_a = (avb_stream_info_t *) a->attribute_info;
  avb_stream_info_t *source_info_b = (avb_stream_info_t *) b->attribute_info;
  return (source_info_a->local_id < source_info_b->local_id);
}

int avb_srp_compare_listener_attributes(mrp_attribute_state *a,
                                       mrp_attribute_state *b)
{
  avb_sink_info_t *sink_info_a = (avb_sink_info_t *) a->attribute_info;
  avb_sink_info_t *sink_info_b = (avb_sink_info_t *) b->attribute_info;
  unsigned int *sA = sink_info_a->reservation.stream_id;
  unsigned int *sB = sink_info_b->reservation.stream_id;
  for (int i=0;i<2;i++) {
    if (sA[i] < sB[i])
      return 1;
    if (sB[i] < sA[i])
      return 0;
  }
  return 0;
}

