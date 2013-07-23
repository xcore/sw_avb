#include "avb.h"
#include <xccompat.h>
#include "avb_srp.h"
#include "avb_mmrp.h"
#include "avb_mvrp.h"
#include "avb_mrp.h"
#include "avb_stream.h"
#include "gptp_config.h"
#include "avb_control_types.h"
#include "avb_media_clock.h"
#include <string.h>
#include "simple_printf.h"
#include <print.h>
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"
#include "ethernet_server_def.h"
#include "mac_filter.h"
#include "avb_1722_maap.h"
#include "nettypes.h"
#include "avb_1722_router.h"

#if AVB_ENABLE_1722_1
#include "avb_1722_1.h"
#include "avb_1722_1_adp.h"
#endif

//#define AVB_TRANSMIT_BEFORE_RESERVATION 1

// Warning: The XC spec makes no assertions that a null chanend is numerically zero. There is
//          no isnull function in C, so this makes up that deficiency, but it may need modifying
//          if XC starts using some other value to mean a null
// #define isnull(A) (A == 0)

#define UNMAPPED (-1)
#define AVB_CHANNEL_UNMAPPED (-1)

typedef struct media_info_t {
  int core_id;
  chanend *unsafe clk_ctl;
  unsigned fifo;
  int local_id;
  int mapped_to;
} media_info_t;

static int max_talker_stream_id = 0;
static int max_listener_stream_id = 0;
static avb_source_info_t sources[AVB_NUM_SOURCES];
static avb_sink_info_t sinks[AVB_NUM_SINKS];
static media_info_t inputs[AVB_NUM_MEDIA_INPUTS];
static media_info_t outputs[AVB_NUM_MEDIA_OUTPUTS];

static unsigned char mac_addr[6];

static mrp_attribute_state *unsafe domain_attr[MRP_NUM_PORTS];

static unsafe void register_talkers(chanend talker_ctl[])
{
  for (int i=0;i<AVB_NUM_TALKER_UNITS;i++) {
    int tile_id, num_streams;
    talker_ctl[i] :> tile_id;
    talker_ctl[i] :> num_streams;
    for (int j=0;j<num_streams;j++) {
      avb_source_info_t *unsafe source = &sources[max_talker_stream_id];
      source->stream.state = AVB_SOURCE_STATE_DISABLED;
      chanend *unsafe p_talker_ctl = &talker_ctl[i];
      source->talker_ctl = p_talker_ctl;
      source->stream.tile_id = tile_id;
      source->stream.local_id = j;
      source->stream.flags = 0;
      source->reservation.stream_id[0] = (mac_addr[0] << 24) | (mac_addr[1] << 16) | (mac_addr[2] <<  8) | (mac_addr[3] <<  0);
      source->reservation.stream_id[1] = (mac_addr[4] << 24) | (mac_addr[5] << 16) | ((source->stream.local_id & 0xffff)<<0);
      source->presentation = AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS;
      source->reservation.vlan_id = AVB_DEFAULT_VLAN;
      source->stream.srp_talker_attr0 = mrp_get_attr();
      source->stream.srp_talker_attr1 = mrp_get_attr();
      source->stream.srp_talker_failed_attr = mrp_get_attr();
      // source->stream.srp_listener_attr = mrp_get_attr();
      mrp_attribute_init_source_info(source->stream.srp_talker_attr0, MSRP_TALKER_ADVERTISE, 0, 1, source);
      mrp_attribute_init_source_info(source->stream.srp_talker_attr1, MSRP_TALKER_ADVERTISE, 1, 1, source);
      // mrp_attribute_init(source->stream.srp_talker_failed_attr, MSRP_TALKER_FAILED, 0, 1, source);
      // mrp_attribute_init(source->stream.srp_listener_attr, MSRP_LISTENER, 0, 1, source);
      max_talker_stream_id++;
    }
  }
}


static int max_link_id = 0;


static unsafe void register_listeners(chanend listener_ctl[])
{
  for (int i=0;i<AVB_NUM_LISTENER_UNITS;i++) {
    int tile_id, num_streams;
    listener_ctl[i] :> tile_id;
    listener_ctl[i] :> num_streams;
    for (int j=0;j<num_streams;j++) {
      avb_sink_info_t *unsafe sink = &sinks[max_listener_stream_id];
      sink->stream.state = AVB_SINK_STATE_DISABLED;
      chanend *unsafe p_listener_ctl = &listener_ctl[i];
      sink->listener_ctl = p_listener_ctl;
      sink->stream.tile_id = tile_id;
      sink->stream.local_id = j;
      sink->stream.flags = 0;
      sink->reservation.vlan_id = AVB_DEFAULT_VLAN;
      // sink->stream.srp_talker_attr = mrp_get_attr();
      // sink->stream.srp_talker_failed_attr = mrp_get_attr();
      sink->stream.srp_listener_attr0 = mrp_get_attr();
      sink->stream.srp_listener_attr1 = mrp_get_attr();
      // mrp_attribute_init(sink->stream.srp_talker_attr, MSRP_TALKER_ADVERTISE, 0, 1, sink);
      // mrp_attribute_init(sink->stream.srp_talker_failed_attr, MSRP_TALKER_FAILED, 0, 1, sink);
      mrp_attribute_init_sink_info(sink->stream.srp_listener_attr0, MSRP_LISTENER, 0, 1, sink);
      mrp_attribute_init_sink_info(sink->stream.srp_listener_attr1, MSRP_LISTENER, 1, 1, sink);
      max_listener_stream_id++;
    }
    listener_ctl[i] <: max_link_id;
    max_link_id++;
  }
}

static unsafe void register_media(chanend media_ctl[])
{
  int input_id = 0;
  int output_id = 0;

  for (int i=0;i<AVB_NUM_MEDIA_UNITS;i++) {
    int core_id;
    int num_in;
    int num_out;
    chanend *unsafe clk_ctl;
    media_ctl[i] :> core_id;
    media_ctl[i] :> clk_ctl;
    media_ctl[i] :> num_in;

    for (int j=0;j<num_in;j++) {
      media_ctl[i] <: input_id;
      inputs[input_id].core_id = core_id;
      inputs[input_id].clk_ctl = clk_ctl;
      inputs[input_id].local_id = j;
      inputs[input_id].mapped_to = UNMAPPED;
      media_ctl[i] :> inputs[input_id].fifo;
      input_id++;

    }
    media_ctl[i] :> num_out;
    for (int j=0;j<num_out;j++) {
      media_ctl[i] <: output_id;
      outputs[output_id].core_id = core_id;
      outputs[output_id].clk_ctl = clk_ctl;
      outputs[output_id].local_id = j;
      outputs[output_id].mapped_to = UNMAPPED;
      media_ctl[i] :> outputs[output_id].fifo;
      output_id++;
    }
  }
}

static void init_media_clock_server(chanend media_clock_ctl)
{
	if (!isnull(media_clock_ctl)) {
		for (int i=0;i<AVB_NUM_MEDIA_OUTPUTS;i++) {
      media_clock_ctl <: outputs[i].fifo;
		}
	}
}

unsafe void avb_init(chanend c_media_ctl[],
              chanend ?c_listener_ctl[],
              chanend ?c_talker_ctl[],
              chanend ?c_media_clock_ctl,
              chanend c_mac_rx,
              chanend c_mac_tx,
              chanend c_ptp)
{
  mac_get_macaddr(c_mac_tx, mac_addr);

  mrp_init((char *)mac_addr);
  register_talkers(c_talker_ctl);
  register_listeners(c_listener_ctl);
  register_media(c_media_ctl);
  init_media_clock_server(c_media_clock_ctl);

  avb_1722_maap_init(mac_addr);

#if AVB_ENABLE_1722_1
  avb_1722_1_init(mac_addr);
#endif

  for(int i=0; i < MRP_NUM_PORTS; i++)
  {
    domain_attr[i] = mrp_get_attr();
    mrp_attribute_init_null(domain_attr[i], MSRP_DOMAIN_VECTOR, i, 1);
  }

#ifdef AVB_INCLUDE_MMRP
  avb_mmrp_init();
#endif

#ifndef AVB_EXCLUDE_MVRP
  avb_mvrp_init();
#endif

  c_mac_rx <: ETHERNET_TX_INIT_AVB_ROUTER;

  mac_set_custom_filter(c_mac_rx, MAC_FILTER_AVB_CONTROL);
  mac_request_status_packets(c_mac_rx);
}

#if 0
void avb_init_srp_only(chanend c_mac_rx0,
                       chanend c_mac_tx0)
{
  mac_get_macaddr(c_mac_tx0, mac_addr);
  mrp_init((char *)mac_addr);

  c_mac_rx = c_mac_rx0;
  c_mac_tx = c_mac_tx0;

  for(int i=0; i < MRP_NUM_PORTS; i++)
  {
    domain_attr[i] = mrp_get_attr();
    mrp_attribute_init(domain_attr[i], MSRP_DOMAIN_VECTOR, i, 1, NULL);
  }

  mac_set_custom_filter(c_mac_rx, MAC_FILTER_AVB_CONTROL);
  mac_request_status_packets(c_mac_rx);
}
#endif

#define timeafter(A, B) ((int)((B) - (A)) < 0)

void avb_periodic(chanend c_mac_tx, unsigned int time_now)
{
  static unsigned int first_time = 1;
  static int maap_started = 0;

	mrp_periodic();
#if AVB_ENABLE_1722_1
	avb_1722_1_periodic(c_mac_tx, c_ptp);
#endif
	avb_1722_maap_periodic(c_mac_tx);

  if ((first_time == 1) && (time_now != 1)){
    first_time = time_now;
  }

  if ((first_time != 1) && !maap_started && timeafter(time_now, first_time+(RECV_ANNOUNCE_TIMEOUT+ANNOUNCE_PERIOD)))
  {
    // Request a multicast addresses for stream transmission
    avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);
    maap_started = 1;
  }
}

void avb_start(void)
{
#if AVB_ENABLE_1722_1
  avb_1722_1_adp_init();
  avb_1722_1_adp_depart_then_announce();
  avb_1722_1_adp_discover_all();
#endif

  for (int i=0; i < MRP_NUM_PORTS; i++)
  {
    mrp_mad_begin(domain_attr[i]);
    mrp_mad_join(domain_attr[i], 1);
  }
}

static void avb_set_talker_bandwidth(chanend c_mac_tx)
{
#if defined(ETHERNET_TX_HP_QUEUE) && defined(ETHERNET_TRAFFIC_SHAPER)
  int data_size = 0;
  for (int i=0;i<AVB_NUM_SOURCES;i++) {
    avb_source_info_t *source = &sources[i];
    if (source->stream.state == AVB_SOURCE_STATE_POTENTIAL ||
        source->stream.state == AVB_SOURCE_STATE_ENABLED)
      {
        int samples_per_packet = (source->stream.rate + (AVB1722_PACKET_RATE-1))/AVB1722_PACKET_RATE;
        data_size += 18 + 32 + (source->stream.num_channels * samples_per_packet * 4);
      }
  }
  mac_set_qav_bandwidth(c_mac_tx, (data_size*8*AVB1722_PACKET_RATE*102)/100);
#endif
}

static void set_sink_state0(int sink_num, enum avb_sink_state_t state, chanend c_mac_tx, chanend ?c_media_clock_ctl) {
  unsafe {
    avb_sink_info_t *sink = &sinks[sink_num];
    chanend *unsafe c = sink->listener_ctl;
    if (sink->stream.state == AVB_SINK_STATE_DISABLED &&
        state == AVB_SINK_STATE_POTENTIAL) {

      chanend *unsafe clk_ctl = outputs[sink->map[0]].clk_ctl;
      simple_printf("Listener sink #%d chan map:\n", sink_num);
      *c <: AVB1722_CONFIGURE_LISTENER_STREAM;
      *c <: sink->stream.local_id;
      *c <: sink->stream.sync;
      *c <: sink->stream.rate;
      *c <: sink->stream.num_channels;

      for (int i=0;i<sink->stream.num_channels;i++) {
        if (sink->map[i] == AVB_CHANNEL_UNMAPPED) {
          *c <: 0;
          simple_printf("  %d unmapped\n", i);
        }
        else {
          *c <: outputs[sink->map[i]].fifo;
          simple_printf("  %d -> %x\n", i, sink->map[i]);
        }
      }
      *c :> int _;

      if (!isnull(c_media_clock_ctl)) {
        media_clock_register(c_media_clock_ctl, clk_ctl, sink->stream.sync);
      }

      *c <: AVB1722_GET_ROUTER_LINK;
      int router_link;
      *c :> router_link;

      avb_1722_add_stream_mapping(c_mac_tx,
                                  sink->reservation.stream_id,
                                  router_link,
                                  sink->stream.local_id);

  #ifndef AVB_EXCLUDE_MVRP
      if (sink->reservation.vlan_id) {
        avb_join_vlan(sink->reservation.vlan_id);
      }
  #endif

  #ifdef AVB_INCLUDE_MMRP
      if (sink->addr[0] & 1) {
        avb_join_multicast_group(sink->addr);
      }
  #endif

        avb_match_and_join_leave(sink->stream.srp_listener_attr0, 1);
        avb_match_and_join_leave(sink->stream.srp_listener_attr1, 1);

    }
    else if (sink->stream.state != AVB_SINK_STATE_DISABLED &&
            state == AVB_SINK_STATE_DISABLED) {

    *c <: AVB1722_DISABLE_LISTENER_STREAM;
    *c <: sink->stream.local_id;
    *c :> int _;

    avb_match_and_join_leave(sink->stream.srp_listener_attr0, 0);
    avb_match_and_join_leave(sink->stream.srp_listener_attr1, 0);

    avb_1722_remove_stream_mapping(c_mac_tx, sink->reservation.stream_id);

  #ifdef AVB_INCLUDE_MMRP
      if (sink->addr[0] & 1) {
        avb_leave_multicast_group(sink->addr);
      }
  #endif

  #ifndef AVB_EXCLUDE_MVRP
      if (sink->reservation.vlan_id) {
        avb_leave_vlan(sink->reservation.vlan_id);
      }
  #endif
    }
    sink->stream.state = state;
  }
}

static void set_source_state0(int source_num, enum avb_source_state_t state, chanend c_mac_tx, chanend ?c_media_clock_ctl) {
  unsafe {
    char stream_string[] = "Talker stream ";
    avb_source_info_t *source = &sources[source_num];
    chanend *unsafe c = source->talker_ctl;
    if (source->stream.state == AVB_SOURCE_STATE_DISABLED &&
        state == AVB_SOURCE_STATE_POTENTIAL) {
      // enable the source
      int valid = 1;
      chanend *unsafe clk_ctl = inputs[source->map[0]].clk_ctl;

      if (source->stream.num_channels <= 0) {
        valid = 0;
      }

      // check that the map is ok
      for (int i=0;i<source->stream.num_channels;i++) {
        if (inputs[source->map[i]].mapped_to != UNMAPPED) {
          valid = 0;
        }
        if (inputs[source->map[i]].clk_ctl != clk_ctl) {
          valid = 0;
        }
      }


      if (valid) {
        unsigned fifo_mask = 0;

        for (int i=0;i<source->stream.num_channels;i++) {
          inputs[source->map[i]].mapped_to = source_num;
          fifo_mask |= (1 << source->map[i]);
        }

        *c <: AVB1722_CONFIGURE_TALKER_STREAM;
        *c <: source->stream.local_id;
        *c <: source->stream.format;

        for (int i=0; i < 6;i++) {
          *c <: source->reservation.dest_mac_addr[i];
        }

        *c <: source_num;
        *c <: source->stream.num_channels;
        *c <: fifo_mask;

        for (int i=0;i<source->stream.num_channels;i++) {
          *c <: inputs[source->map[i]].fifo;
        }
        *c <: source->stream.rate;

        if (source->presentation)
          *c <: source->presentation;
        else
          *c <: AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS;

        *c :> int _;

    #ifndef AVB_EXCLUDE_MVRP
        if (source->reservation.vlan_id) {
          avb_join_vlan(source->reservation.vlan_id);
        }
    #endif
        avb_match_and_join_leave(source->stream.srp_talker_attr0, 1);
        avb_match_and_join_leave(source->stream.srp_talker_attr1, 1);

        if (!isnull(c_media_clock_ctl)) {
          media_clock_register(c_media_clock_ctl, clk_ctl, source->stream.sync);
        }

    #if defined(AVB_TRANSMIT_BEFORE_RESERVATION)
        {
          *c <: AVB1722_TALKER_GO;
          *c <: source->stream.local_id;
          *c :> int _;

          printstr(stream_string); simple_printf("#%d on\n", source_num);
        }
    #else
        printstr(stream_string); simple_printf("#%d ready\n", source_num);
    #endif

      }
    }
    else if (source->stream.state == AVB_SOURCE_STATE_ENABLED &&
        state == AVB_SOURCE_STATE_POTENTIAL) {
      // stop transmission

        *c <: AVB1722_TALKER_STOP;
        *c <: source->stream.local_id;
        *c :> int _;

        printstr(stream_string); simple_printf("#%d off\n", source_num);
    }
    else if (source->stream.state == AVB_SOURCE_STATE_POTENTIAL &&
             state == AVB_SOURCE_STATE_ENABLED) {
      // start transmitting

      printstr(stream_string); simple_printf("#%d on\n", source_num);
      *c <: AVB1722_TALKER_GO;
      *c <: source->stream.local_id;
      *c :> int _;
    }
    else if (source->stream.state != AVB_SOURCE_STATE_DISABLED &&
             state == AVB_SOURCE_STATE_DISABLED) {
      // disabled the source
        for (int i=0;i<source->stream.num_channels;i++) {
          inputs[source->map[i]].mapped_to = UNMAPPED;
        }

        *c <: AVB1722_TALKER_STOP;
        *c <: source->stream.local_id;
        *c :> int _;

    #ifndef AVB_EXCLUDE_MVRP
      if (source->reservation.vlan_id) {
        avb_leave_vlan(source->reservation.vlan_id);
      }
    #endif

        // And remove the group
        avb_match_and_join_leave(source->stream.srp_talker_attr0, 0);
        avb_match_and_join_leave(source->stream.srp_talker_attr1, 0);
    }
    avb_set_talker_bandwidth(c_mac_tx);
    source->stream.state = state;
  }
}


// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

[[combinable]]
void avb_manager(server interface avb_interface avb,
                 chanend c_media_ctl[],
                 chanend ?c_listener_ctl[],
                 chanend ?c_talker_ctl[],
                 chanend ?c_media_clock_ctl,
                 chanend c_mac_rx,
                 chanend c_mac_tx,
                 chanend c_ptp) {
  unsigned periodic_timeout;
  timer tmr;
  unsigned int nbytes;
  unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
  unsigned int port_num;

  unsafe {
    avb_init(c_media_ctl, c_listener_ctl, c_talker_ctl, c_media_clock_ctl, c_mac_rx, c_mac_tx, c_ptp);
  }
  tmr :> periodic_timeout;

  while (1) {
    select {
      // Receive and process any incoming AVB packets (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_mac_rx, buf, nbytes, port_num):
      {
        avb_process_control_packet(buf, nbytes, c_mac_tx, c_media_clock_ctl, port_num);
        break;
      }
      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> unsigned int time_now:
      {
        avb_periodic(c_mac_tx, time_now);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }
      case avb.get_source_format(int source_num, enum avb_stream_format_t &format, int &rate) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          format = source->stream.format;
          rate = source->stream.rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_format(int source_num, enum avb_stream_format_t format, int rate) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->stream.format = format;
          source->stream.rate = rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_channels(int source_num, int &channels) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          channels = source->stream.num_channels;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_channels(int source_num, int channels) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->stream.num_channels = channels;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_sync(int source_num, int &sync) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          sync = source->stream.sync;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_sync(int source_num, int sync) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->stream.sync = sync;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_presentation(int source_num, int &presentation) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          presentation = source->presentation;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_presentation(int source_num, int presentation) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->presentation = presentation;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_vlan(int source_num, int &vlan) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          vlan = source->reservation.vlan_id;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_vlan(int source_num, int vlan) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->reservation.vlan_id = vlan;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }
      case avb.get_source_state(int source_num, enum avb_source_state_t &state) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          state = source->stream.state;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_state(int source_num, enum avb_source_state_t state) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          unsafe {
            set_source_state0(source_num, state, c_mac_tx, c_media_clock_ctl);
          }
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_map(int source_num, int map[], int &len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          len = source[source_num].stream.num_channels;
          memcpy(map, source->map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }      
      case avb.set_source_map(int source_num, int map[len], int len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED &&
          len <= AVB_MAX_CHANNELS_PER_TALKER_STREAM) {
          avb_source_info_t *source = &sources[source_num];
          memcpy(source->map, map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_dest(int source_num, unsigned char addr[], int &len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          len = 6;
          memcpy(addr, source->reservation.dest_mac_addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_source_dest(int source_num, unsigned char addr[], int len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED &&
          len == 6) {
          avb_source_info_t *source = &sources[source_num];
          memcpy(source->reservation.dest_mac_addr, addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_source_id(int source_num, unsigned int id[2]) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          memcpy(id, source->reservation.stream_id, 8);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_sink_id(int sink_num, unsigned int stream_id[2]) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(stream_id, sink->reservation.stream_id, 8);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_sink_id(int sink_num, unsigned int stream_id[2]) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(sink->reservation.stream_id, stream_id, 8);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_sink_format(int sink_num, enum avb_stream_format_t &format, int &rate) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          format = sink->stream.format;
          rate = sink->stream.rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_sink_format(int sink_num, enum avb_stream_format_t format, int rate) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->stream.format = format;
          sink->stream.rate = rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_sink_channels(int sink_num, int &channels) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          channels = sink->stream.num_channels;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }
      case avb.set_sink_channels(int sink_num, int channels) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->stream.num_channels = channels;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }
      case avb.get_sink_sync(int sink_num, int &sync) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sync = sink->stream.sync;
          return_val = 1;
        }
        else return_val = 0;
        break;        
      }
      case avb.set_sink_sync(int sink_num, int sync) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->stream.sync = sync;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }      
      case avb.get_sink_vlan(int sink_num, int &vlan) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          vlan = sink->reservation.vlan_id;
          return_val = 1;
        }
        else return_val = 0;
        break;       
      }
      case avb.set_sink_vlan(int sink_num, int vlan) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->reservation.vlan_id = vlan;
          return_val = 1;
        }
        else return_val = 0;
        break;       
      }
      case avb.get_sink_addr(int sink_num, unsigned char addr[], int &len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          len = 6;
          memcpy(addr, sink->reservation.dest_mac_addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;        
      }
      case avb.set_sink_addr(int sink_num, unsigned char addr[len], int len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED && len == 6) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(sink->reservation.dest_mac_addr, addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb.get_sink_state(int sink_num, enum avb_sink_state_t &state) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          state = sink->stream.state; 
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.set_sink_state(int sink_num, enum avb_sink_state_t state) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          unsafe {
            set_sink_state0(sink_num, state, c_mac_tx, c_media_clock_ctl);
          }
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb.get_sink_map(int sink_num, int map[], int &len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          len = sinks->stream.num_channels;
          memcpy(map, sink->map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;     
      }
      case avb.set_sink_map(int sink_num, int map[len], int len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED && 
            len <= AVB_MAX_CHANNELS_PER_LISTENER_STREAM) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(sink->map, map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb.get_device_media_clock_rate(int clock_num, int &rate) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          rate = media_clock_get_rate(c_media_clock_ctl, clock_num);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb.set_device_media_clock_rate(int clock_num, int rate) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_rate(c_media_clock_ctl, clock_num, rate);
          return_val = 1;
        }
        else return_val = 0;
        break;          
      }
      case avb.get_device_media_clock_state(int clock_num, enum device_media_clock_state_t &state) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          state = media_clock_get_state(c_media_clock_ctl, clock_num);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb.set_device_media_clock_state(int clock_num, enum device_media_clock_state_t state) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_state(c_media_clock_ctl, clock_num, state);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb.get_device_media_clock_source(int clock_num, int &source) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          int source0 = source;
          media_clock_get_source(c_media_clock_ctl, clock_num, source0);
          source = source0;
          return_val = 1;
        }
        else return_val = 0;
        break;         
      }
      case avb.set_device_media_clock_source(int clock_num, int source) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_source(c_media_clock_ctl, clock_num, source);
          return_val = 1;
        }
        else return_val = 0;
        break;          
      }
      case avb.get_device_media_clock_type(int clock_num, enum device_media_clock_type_t &clock_type) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          clock_type = media_clock_get_type(c_media_clock_ctl, clock_num);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb.set_device_media_clock_type(int clock_num, enum device_media_clock_type_t clock_type) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_type(c_media_clock_ctl, clock_num, clock_type);
          return_val = 1;
        }
        else return_val = 0;
        break;          
      }
    }
  }
}

unsafe int set_avb_source_port(int source_num,
                        int srcport) {
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num];
    chanend *unsafe c = source->talker_ctl;
    *c <: AVB1722_SET_PORT;
    *c <: source->stream.local_id;
    *c <: srcport;
    *c :> int _;

    return 1;
  }
  else
    return 0;
}

#ifdef MEDIA_OUTPUT_FIFO_VOLUME_CONTROL
void set_avb_source_volumes(int sink_num, int volumes[], int count)
{
	if (sink_num < AVB_NUM_SINKS) {
		  avb_sink_info_t *sink = &sinks[sink_num];
		  chanend c = sink->listener_ctl;
		  xc_abi_outuint(c, AVB1722_ADJUST_LISTENER_STREAM);
		  xc_abi_outuint(c, sink->stream.local_id);
		  xc_abi_outuint(c, AVB1722_ADJUST_LISTENER_VOLUME);
		  xc_abi_outuint(c, count);
	      for (int i=0;i<count;i++) {
	          xc_abi_outuint(c, volumes[i]);
	      }
	}
}
#endif


void avb_process_control_packet(unsigned int buf0[], int nbytes, chanend c_tx, chanend ?c_media_clock_ctl, unsigned int port_num)
{
  if (nbytes == STATUS_PACKET_LEN) {
    if (((unsigned char *)buf0)[0]) { // Link up
      avb_start();
    }
    else { // Link down
      for (int i=0; i < AVB_NUM_SOURCES; i++) {
        set_source_state0(i, AVB_SOURCE_STATE_DISABLED, c_tx, c_media_clock_ctl);
      }

      for (int i=0; i < AVB_NUM_SINKS; i++) {
        set_sink_state0(i, AVB_SOURCE_STATE_DISABLED, c_tx, c_media_clock_ctl);
      }      
    }
  }
  else {
    struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf0[0];

    int etype, eth_hdr_size;
    int has_qtag = ethernet_hdr->ethertype[1]==0x18;
    eth_hdr_size = has_qtag ? 18 : 14;

    if (has_qtag) {
      struct tagged_ethernet_hdr_t *tagged_ethernet_hdr = (tagged_ethernet_hdr_t *) &buf0[0];
      etype = (int)(tagged_ethernet_hdr->ethertype[0] << 8) + (int)(tagged_ethernet_hdr->ethertype[1]);
    }
    else {
      etype = (int)(ethernet_hdr->ethertype[0] << 8) + (int)(ethernet_hdr->ethertype[1]);
    }
    int len = nbytes - eth_hdr_size;

    unsigned char *buf = (unsigned char *) buf0;

    unsafe {

      switch (etype) {
        /* fallthrough intended */
        case AVB_SRP_ETHERTYPE:
        // TODO: #define around MMRP, disabled by default
        case AVB_MMRP_ETHERTYPE:
        case AVB_MVRP_ETHERTYPE:
          avb_mrp_process_packet((unsigned char *unsafe)&buf[eth_hdr_size], etype, len, port_num);
          break;
        case AVB_1722_ETHERTYPE:
          // We know that the cd field is true because the MAC filter only forwards
          // 1722 control to this thread
        #if AVB_ENABLE_1722_1
          avb_1722_1_process_packet((unsigned char *unsafe)&buf[eth_hdr_size], ethernet_hdr->src_addr, len, c_tx);
        #endif
          avb_1722_maap_process_packet((unsigned char *unsafe)&buf[eth_hdr_size], ethernet_hdr->src_addr, len, c_tx);
          break;
      }
    }
  }

}


unsigned avb_control_get_mac_tx(void)
{
  return 1;
}

unsigned avb_control_get_c_ptp(void)
{
  return 1;
}


int get_avb_ptp_gm(unsigned char a0[])
{
  // ptp_get_current_grandmaster(*c_ptp, a0);
  return 1;
}

int get_avb_ptp_port_pdelay(int srcport, unsigned *pdelay)
{
  if (srcport == 0)
  {
    // ptp_get_propagation_delay(*c_ptp, pdelay);
    return 1;
  }
  else
  {
    return 0;
  }
}

unsigned avb_get_source_stream_index_from_stream_id(unsigned int stream_id[2])
{
  for (unsigned i=0; i<AVB_NUM_SOURCES; ++i) {
    if (stream_id[0] == sources[i].reservation.stream_id[0] &&
        stream_id[1] == sources[i].reservation.stream_id[1]) {
      return i;
    }
  }
  return -1u;  
}

unsigned avb_get_sink_stream_index_from_stream_id(unsigned int stream_id[2])
{
  for (unsigned i=0; i<AVB_NUM_SINKS; ++i) {
    if (stream_id[0] == sinks[i].reservation.stream_id[0] &&
        stream_id[1] == sinks[i].reservation.stream_id[1]) {
      return i;
    }
  }
  return -1u;  
}

unsigned avb_get_source_stream_index_from_pointer(avb_source_info_t *unsafe p)
{
	for (unsigned i=0; i<AVB_NUM_SOURCES; ++i) {
		if (p == &sources[i]) return i;
	}
	return -1u;
}

unsigned avb_get_sink_stream_index_from_pointer(avb_sink_info_t *unsafe p)
{
	for (unsigned i=0; i<AVB_NUM_SINKS; ++i) {
		if (p == &sinks[i]) return i;
	}
	return -1u;
}
