#include <xccompat.h>

#define __AVB_C__
#include "avb.h"
#include "avb_srp.h"
#include "avb_mmrp.h"
#include "avb_mvrp.h"
#include "avb_mrp.h"
#include "avb_stream.h"
#include "gptp_config.h"
#include "avb_control_types.h"
#include "c_io.h"
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

#if AVB_ENABLE_1722_1
#include "avb_1722_1.h"
#include "avb_1722_1_adp.h"
#endif

//#define AVB_TRANSMIT_BEFORE_RESERVATION 1

// Warning: The XC spec makes no assertions that a null chanend is numerically zero. There is
//          no isnull function in C, so this makes up that deficiency, but it may need modifying
//          if XC starts using some other value to mean a null
#define isnull(A) (A == 0)

#define UNMAPPED (-1)
#define AVB_CHANNEL_UNMAPPED (-1)

typedef struct media_info_t {
  int core_id;
  int clk_ctl;
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

static chanend media_clock_svr;

static unsigned char mac_addr[6];

static chanend c_mac_tx;
static chanend c_mac_rx;
static chanend c_ptp;

static mrp_attribute_state *domain_attr[MRP_NUM_PORTS];

static void register_talkers(chanend talker_ctl[])
{
  for (int i=0;i<AVB_NUM_TALKER_UNITS;i++) {
    int tile_id = xc_abi_inuint(talker_ctl[i]);
    int num_streams = xc_abi_inuint(talker_ctl[i]);
    for (int j=0;j<num_streams;j++) {
      avb_source_info_t *source = &sources[max_talker_stream_id];
      source->stream.state = AVB_SOURCE_STATE_DISABLED;
      source->talker_ctl = talker_ctl[i];
      source->stream.tile_id = tile_id;
      source->stream.local_id = j;
      source->stream.flags = 0;
      source->reservation.stream_id[0] = (mac_addr[0] << 24) | (mac_addr[1] << 16) | (mac_addr[2] <<  8) | (mac_addr[3] <<  0);
      source->reservation.stream_id[1] = (mac_addr[4] << 24) | (mac_addr[5] << 16) | ((source->stream.local_id & 0xffff)<<0);
      source->presentation = AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS;
      source->reservation.vlan_id = AVB_DEFAULT_VLAN;
      source->stream.srp_talker_attr = mrp_get_attr();
      source->stream.srp_talker_failed_attr = mrp_get_attr();
      // source->stream.srp_listener_attr = mrp_get_attr();
      mrp_attribute_init(source->stream.srp_talker_attr, MSRP_TALKER_ADVERTISE, 0, 1, source);
      // mrp_attribute_init(source->stream.srp_talker_failed_attr, MSRP_TALKER_FAILED, 0, 1, source);
      // mrp_attribute_init(source->stream.srp_listener_attr, MSRP_LISTENER, 0, 1, source);
      max_talker_stream_id++;
    }
  }
}


static int max_link_id = 0;


static void register_listeners(chanend listener_ctl[])
{
  for (int i=0;i<AVB_NUM_LISTENER_UNITS;i++) {
    int tile_id = xc_abi_inuint(listener_ctl[i]);
    int num_streams = xc_abi_inuint(listener_ctl[i]);
    for (int j=0;j<num_streams;j++) {
      avb_sink_info_t *sink = &sinks[max_listener_stream_id];
      sink->stream.state = AVB_SINK_STATE_DISABLED;
      sink->listener_ctl = listener_ctl[i];
      sink->stream.tile_id = tile_id;
      sink->stream.local_id = j;
      sink->stream.flags = 0;
      sink->reservation.vlan_id = AVB_DEFAULT_VLAN;
      // sink->stream.srp_talker_attr = mrp_get_attr();
      // sink->stream.srp_talker_failed_attr = mrp_get_attr();
      sink->stream.srp_listener_attr = mrp_get_attr();
      // mrp_attribute_init(sink->stream.srp_talker_attr, MSRP_TALKER_ADVERTISE, 0, 1, sink);
      // mrp_attribute_init(sink->stream.srp_talker_failed_attr, MSRP_TALKER_FAILED, 0, 1, sink);
      mrp_attribute_init(sink->stream.srp_listener_attr, MSRP_LISTENER, 0, 1, sink);
      max_listener_stream_id++;
    }
    xc_abi_outuint(listener_ctl[i], max_link_id);
    max_link_id++;
  }
}

static void register_media(chanend media_ctl[])
{
  int input_id = 0;
  int output_id = 0;

  for (int i=0;i<AVB_NUM_MEDIA_UNITS;i++) {
    int core_id;
    int num_in;
    int num_out;
    int clk_ctl;
    core_id = xc_abi_inuint(media_ctl[i]);
    clk_ctl = xc_abi_inuint(media_ctl[i]);
    num_in = xc_abi_inuint(media_ctl[i]);
    for (int j=0;j<num_in;j++) {
      xc_abi_outuint(media_ctl[i], input_id);
      inputs[input_id].core_id = core_id;
      inputs[input_id].clk_ctl = clk_ctl;
      inputs[input_id].local_id = j;
      inputs[input_id].mapped_to = UNMAPPED;
      inputs[input_id].fifo = xc_abi_inuint(media_ctl[i]);
      input_id++;

    }
    num_out = xc_abi_inuint(media_ctl[i]);
    for (int j=0;j<num_out;j++) {
      xc_abi_outuint(media_ctl[i], output_id);
      outputs[output_id].core_id = core_id;
      outputs[output_id].clk_ctl = clk_ctl;
      outputs[output_id].local_id = j;
      outputs[output_id].mapped_to = UNMAPPED;
      outputs[output_id].fifo = xc_abi_inuint(media_ctl[i]);
      output_id++;
    }
  }
}

static void init_media_clock_server(chanend media_clock_ctl)
{
	media_clock_svr = media_clock_ctl;
	if (!isnull(media_clock_ctl)) {
		for (int i=0;i<AVB_NUM_MEDIA_OUTPUTS;i++) {
			xc_abi_outuint(media_clock_svr, outputs[i].fifo);
		}
	}

}

void avb_init(chanend media_ctl[],
              chanend listener_ctl[],
              chanend talker_ctl[],
              chanend media_clock_ctl,
              chanend c_mac_rx0,
              chanend c_mac_tx0,
              chanend c_ptp0)
{
  mac_get_macaddr(c_mac_tx0, mac_addr);

  mrp_init((char *)mac_addr);
  register_talkers(talker_ctl);
  register_listeners(listener_ctl);
  register_media(media_ctl);
  init_media_clock_server(media_clock_ctl);

  avb_1722_maap_init(mac_addr);

#if AVB_ENABLE_1722_1
  avb_1722_1_init(mac_addr);
#endif

  c_mac_rx = c_mac_rx0;
  c_mac_tx = c_mac_tx0;
  c_ptp = c_ptp0;

  for(int i=0; i < MRP_NUM_PORTS; i++)
  {
    domain_attr[i] = mrp_get_attr();
    mrp_attribute_init(domain_attr[i], MSRP_DOMAIN_VECTOR, i, 1, NULL);
  }

#ifdef AVB_INCLUDE_MMRP
  avb_mmrp_init();
#endif

#ifndef AVB_EXCLUDE_MVRP
  avb_mvrp_init();
#endif

  xc_abi_outuint(c_mac_tx, ETHERNET_TX_INIT_AVB_ROUTER);

  mac_set_custom_filter(c_mac_rx, MAC_FILTER_AVB_CONTROL);
  mac_request_status_packets(c_mac_rx);
}

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

#define timeafter(A, B) ((int)((B) - (A)) < 0)

void avb_periodic(unsigned int time_now)
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
    avb_1722_maap_request_addresses(AVB_NUM_SOURCES, NULL);
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

static void avb_set_talker_bandwidth()
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

int get_avb_sources(int *a0)
{
  *a0 = AVB_NUM_SOURCES;
  return 1;
}




int getset_avb_source_format(int set,
                             int source_num,
                             enum avb_stream_format_t *format,
                             int *rate)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set || sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      source->stream.format = *format;
      source->stream.rate = *rate;
    }
    *format = source->stream.format;
    *rate = source->stream.rate;
    return 1;
  }
  else
    return 0;
}


int getset_avb_source_channels(int set,
                               int source_num,
                               int *channels)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set || sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      source->stream.num_channels = *channels;
    }
    *channels = source->stream.num_channels;
    return 1;
  }
  else
    return 0;
}

int getset_avb_source_sync(int set,
                           int source_num,
                           int *sync)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set || sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      source->stream.sync = *sync;
    }
    *sync = source->stream.sync;
    return 1;
  }
  else
    return 0;
}

int getset_avb_source_presentation(int set,
                               int source_num,
                               int *presentation)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set || sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      source->presentation = *presentation;
    }
    *presentation = source->presentation;
    return 1;
  }
  else
    return 0;
}

int getset_avb_source_vlan(int set,
                               int source_num,
                               int *vlan)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set || sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      source->reservation.vlan_id = *vlan;
    }
    *vlan = source->reservation.vlan_id;
    return 1;
  }
  else
    return 0;
}

int getset_avb_source_state(int set,
                            int source_num,
                            enum avb_source_state_t *state)
{
  char stream_string[] = "Talker stream ";
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      if (source->stream.state == AVB_SOURCE_STATE_DISABLED &&
          *state == AVB_SOURCE_STATE_POTENTIAL) {
        // enable the source
        int valid = 1;
        int clk_ctl;
 
        if (source->stream.num_channels <= 0)
          valid = 0;

        clk_ctl = inputs[source->map[0]].clk_ctl;

        // check that the map is ok
        for (int i=0;i<source->stream.num_channels;i++) {
          if (inputs[source->map[i]].mapped_to != UNMAPPED)
            valid = 0;
          if (inputs[source->map[i]].clk_ctl != clk_ctl)
            valid = 0;
        }


        if (valid) {
          chanend c = source->talker_ctl;
          unsigned fifo_mask = 0;

          for (int i=0;i<source->stream.num_channels;i++) {
            inputs[source->map[i]].mapped_to = source_num;
            fifo_mask |= (1 << source->map[i]);
          }

          xc_abi_outuint(c, AVB1722_CONFIGURE_TALKER_STREAM);
          xc_abi_outuint(c, source->stream.local_id);
          xc_abi_outuint(c, source->stream.format);
          for (int i=0; i < 6;i++) {
            xc_abi_outuint(c, source->reservation.dest_mac_addr[i]);
          }
          xc_abi_outuint(c, source_num);
          xc_abi_outuint(c, source->stream.num_channels);
          xc_abi_outuint(c, fifo_mask);
          for (int i=0;i<source->stream.num_channels;i++) {
            xc_abi_outuint(c, inputs[source->map[i]].fifo);
          }
          xc_abi_outuint(c, source->stream.rate);
          if (source->presentation)
            xc_abi_outuint(c, source->presentation);
          else
            xc_abi_outuint(c, AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS);

          (void) xc_abi_inuint(c);

#ifndef AVB_EXCLUDE_MVRP
          if (source->reservation.vlan_id)
            avb_join_vlan(source->reservation.vlan_id);
#endif

          mrp_attribute_state *matched_stream_id_other_port = mrp_match_attr_by_stream_and_type(source->stream.srp_talker_attr, 1);
          mrp_attribute_state *matched_stream_id_this_port = mrp_match_attr_by_stream_and_type(source->stream.srp_talker_attr, 0);

          if (matched_stream_id_other_port && matched_stream_id_other_port->propagate) {
            mrp_mad_join(matched_stream_id_other_port, 1);
          }
          else if (matched_stream_id_this_port && matched_stream_id_this_port->propagate) {
            mrp_mad_join(matched_stream_id_this_port, 1);
          }
          else {
            mrp_mad_begin(source->stream.srp_talker_attr);
            mrp_mad_join(source->stream.srp_talker_attr, 1);
          }

          // mrp_mad_begin(source->stream.srp_talker_attr);
          // mrp_mad_begin(source->stream.srp_talker_failed_attr);
          // mrp_mad_begin(source->stream.srp_listener_attr);

          // mrp_mad_join(source->stream.srp_talker_attr, 1);

          if (!isnull(media_clock_svr)) {
        	  media_clock_register(media_clock_svr, clk_ctl, source->stream.sync);
          }

#if defined(AVB_TRANSMIT_BEFORE_RESERVATION)
          {
        	chanend c = source->talker_ctl;
            xc_abi_outuint(c, AVB1722_TALKER_GO);
            xc_abi_outuint(c, source->stream.local_id);
            (void) xc_abi_inuint(c); //ACK

            printstr(stream_string); simple_printf("#%d on\n", source_num);
          }
#else
          printstr(stream_string); simple_printf("#%d ready\n", source_num);
#endif

        }
      }
      else if (source->stream.state == AVB_SOURCE_STATE_ENABLED &&
          *state == AVB_SOURCE_STATE_POTENTIAL) {
    	  // stop transmission

          chanend c = source->talker_ctl;
          xc_abi_outuint(c, AVB1722_TALKER_STOP);
          xc_abi_outuint(c, source->stream.local_id);
          (void) xc_abi_inuint(c); //ACK

          printstr(stream_string); simple_printf("#%d off\n", source_num);
      }
      else if (source->stream.state == AVB_SOURCE_STATE_POTENTIAL &&
               *state == AVB_SOURCE_STATE_ENABLED) {
        // start transmitting

        printstr(stream_string); simple_printf("#%d on\n", source_num);
        chanend c = source->talker_ctl;
        xc_abi_outuint(c, AVB1722_TALKER_GO);
        xc_abi_outuint(c, source->stream.local_id);
        (void) xc_abi_inuint(c); //ACK
      }
      else if (source->stream.state != AVB_SOURCE_STATE_DISABLED &&
               *state == AVB_SOURCE_STATE_DISABLED) {
        // disabled the source
          for (int i=0;i<source->stream.num_channels;i++) {
            inputs[source->map[i]].mapped_to = UNMAPPED;
          }
          chanend c = source->talker_ctl;
          xc_abi_outuint(c, AVB1722_TALKER_STOP);
          xc_abi_outuint(c, source->stream.local_id);
          (void) xc_abi_inuint(c); //ACK

#ifndef AVB_EXCLUDE_MVRP
        if (source->reservation.vlan_id)
          avb_leave_vlan(source->reservation.vlan_id);
#endif

          // And remove the group
          mrp_attribute_state *matched_stream_id_other_port = mrp_match_attr_by_stream_and_type(source->stream.srp_talker_attr, 1);
          mrp_attribute_state *matched_stream_id_this_port = mrp_match_attr_by_stream_and_type(source->stream.srp_talker_attr, 0);

          if (matched_stream_id_other_port && matched_stream_id_other_port->propagate) {
            mrp_mad_leave(matched_stream_id_other_port);
          }
          else if (matched_stream_id_this_port && matched_stream_id_this_port->propagate) {
            mrp_mad_leave(matched_stream_id_this_port);
          }
          else {
            mrp_mad_leave(source->stream.srp_talker_attr);
          }
      }
      avb_set_talker_bandwidth();
      source->stream.state = *state;
    }
    *state = source->stream.state;
    return 1;
  }
  else
    return 0;
}

int getset_avb_source_map(int set, int source_num, int map[], int *map_len)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set ||
       (sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED &&
        *map_len <= AVB_MAX_CHANNELS_PER_TALKER_STREAM))) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      memcpy(source->map, map, *map_len<<2);
    }
    else {
      *map_len = source[source_num].stream.num_channels;
    }
    memcpy(map, source->map, *map_len<<2);
    return 1;
  }
  else
    return 0;
}


int getset_avb_source_dest(int set, int source_num, unsigned char dest[], int *dest_len)
{
  if (source_num < AVB_NUM_SOURCES &&
      (!set ||
       (sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED &&
        *dest_len == 6))) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      memcpy(source->reservation.dest_mac_addr, dest, 6);
    }
    *dest_len = 6;
    memcpy(dest, source->reservation.dest_mac_addr, 6);
    return 1;
  }
  else
    return 0;
}

int get_avb_source_id(int source_num, unsigned int a1[2])
{
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num];

    memcpy(a1, source->reservation.stream_id, 8);

    return 1;
  }
  else
    return 0;
}



int get_avb_sinks(int *a0)
{
  *a0 = AVB_NUM_SINKS;
  return 1;
}

int getset_avb_sink_id(int set, int sink_num, unsigned int a1[2])
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      memcpy(sink->reservation.stream_id, a1, 8);
    }
    memcpy(a1, sink->reservation.stream_id, 8);
    return 1;
  }
  else
    return 0;
}

int getset_avb_sink_format(int set,
                           int sink_num,
                           enum avb_stream_format_t *format,
                           int *rate)
{
  if (sink_num < AVB_NUM_SOURCES &&
      (!set || sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      sink->stream.format = *format;
      sink->stream.rate = *rate;
    }
    *format = sink->stream.format;
    *rate = sink->stream.rate;
    return 1;
  }
  else
    return 0;
}

int getset_avb_sink_channels(int set,
                               int sink_num,
                               int *channels)
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      sink->stream.num_channels = *channels;
    }
    *channels = sink->stream.num_channels;
    return 1;
  }
  else
    return 0;
}


int getset_avb_sink_sync(int set,
                         int sink_num,
                         int *sync)
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      sink->stream.sync = *sync;
    }
    *sync = sink->stream.sync;
    return 1;
  }
  else
    return 0;
}

int getset_avb_sink_vlan(int set,
                         int sink_num,
                         int *vlan)
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      sink->reservation.vlan_id = *vlan;
    }
    *vlan = sink->reservation.vlan_id;
    return 1;
  }
  else
    return 0;
}


int getset_avb_sink_addr(int set, int sink_num, unsigned char addr[], int *addr_len)
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set ||
       (sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED &&
        *addr_len == 6))) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      memcpy(sink->reservation.dest_mac_addr, addr, 6);
    }
    *addr_len = 6;
    memcpy(addr, sink->reservation.dest_mac_addr, 6);
    return 1;
  }
  else
    return 0;
}


int getset_avb_sink_state(int set,
                          int sink_num,
                          enum avb_sink_state_t *state)
{
  if (sink_num < AVB_NUM_SINKS) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      if (sink->stream.state == AVB_SINK_STATE_DISABLED &&
          *state == AVB_SINK_STATE_POTENTIAL) {
        chanend c = sink->listener_ctl;
        int clk_ctl = outputs[sink->map[0]].clk_ctl;
        simple_printf("Listener sink #%d chan map:\n", sink_num);
        xc_abi_outuint(c, AVB1722_CONFIGURE_LISTENER_STREAM);
        xc_abi_outuint(c, sink->stream.local_id);
        xc_abi_outuint(c, sink->stream.sync);
        xc_abi_outuint(c, sink->stream.rate);
        xc_abi_outuint(c, sink->stream.num_channels);
        for (int i=0;i<sink->stream.num_channels;i++)
        {
          if (sink->map[i] == AVB_CHANNEL_UNMAPPED)
          {
            xc_abi_outuint(c, 0);
            simple_printf("  %d unmapped\n", i);
          }
          else
          {
            xc_abi_outuint(c, outputs[sink->map[i]].fifo);
            simple_printf("  %d -> %x\n", i, sink->map[i]);
          }
        }
        (void) xc_abi_inuint(c);

        if (!isnull(media_clock_svr)) {
          media_clock_register(media_clock_svr, clk_ctl, sink->stream.sync);
        }

        { int router_link;

          xc_abi_outuint(c, AVB1722_GET_ROUTER_LINK);
          router_link = xc_abi_inuint(c);

          avb_1722_add_stream_mapping(c_mac_tx,
                                      sink->reservation.stream_id,
                                      router_link,
                                      sink->stream.local_id);
        }

#ifndef AVB_EXCLUDE_MVRP
        if (sink->reservation.vlan_id)
          avb_join_vlan(sink->reservation.vlan_id);
#endif

#ifdef AVB_INCLUDE_MMRP
        if (sink->addr[0] & 1)
          avb_join_multicast_group(sink->addr);
#endif

          mrp_attribute_state *matched_stream_id_other_port = mrp_match_attr_by_stream_and_type(sink->stream.srp_listener_attr, 1);
          mrp_attribute_state *matched_stream_id_this_port = mrp_match_attr_by_stream_and_type(sink->stream.srp_listener_attr, 0);

          if (matched_stream_id_other_port && matched_stream_id_other_port->propagate) {
            mrp_mad_join(matched_stream_id_other_port, 1);
          }
          else if (matched_stream_id_this_port && matched_stream_id_this_port->propagate) {
            mrp_mad_join(matched_stream_id_this_port, 1);
          }
          else {
            mrp_mad_begin(sink->stream.srp_listener_attr);
            mrp_mad_join(sink->stream.srp_listener_attr, 1);
          }

        // mrp_mad_begin(sink->stream.srp_talker_attr);
        // mrp_mad_begin(sink->stream.srp_talker_failed_attr);
        // mrp_mad_begin(sink->stream.srp_listener_attr);
        // mrp_mad_join(sink->stream.srp_listener_attr, 1);
      }
      else if (sink->stream.state != AVB_SINK_STATE_DISABLED &&
              *state == AVB_SINK_STATE_DISABLED) {

		  chanend c = sink->listener_ctl;
		  xc_abi_outuint(c, AVB1722_DISABLE_LISTENER_STREAM);
		  xc_abi_outuint(c, sink->stream.local_id);
		  (void) xc_abi_inuint(c);

      mrp_attribute_state *matched_stream_id_other_port = mrp_match_attr_by_stream_and_type(sink->stream.srp_listener_attr, 1);
      mrp_attribute_state *matched_stream_id_this_port = mrp_match_attr_by_stream_and_type(sink->stream.srp_listener_attr, 0);

      if (matched_stream_id_other_port && matched_stream_id_other_port->propagate && !matched_stream_id_other_port->here) {
        mrp_mad_leave(matched_stream_id_other_port);
      }
      else if (matched_stream_id_this_port && matched_stream_id_this_port->propagate && !matched_stream_id_this_port->here) {
        mrp_mad_leave(matched_stream_id_this_port);
      }
      else {
        mrp_mad_leave(sink->stream.srp_listener_attr);
      }

      avb_1722_remove_stream_mapping(c_mac_tx, sink->reservation.stream_id);

#ifdef AVB_INCLUDE_MMRP
        if (sink->addr[0] & 1)
          avb_leave_multicast_group(sink->addr);
#endif

#ifndef AVB_EXCLUDE_MVRP
        if (sink->reservation.vlan_id)
          avb_leave_vlan(sink->reservation.vlan_id);
#endif
      }
      sink->stream.state = *state;
    }
    *state = sink->stream.state;
    return 1;
  }
  else
    return 0;
}

int getset_avb_sink_map(int set, int sink_num, int map[], int *map_len)
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || (sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED
                && *map_len <= AVB_MAX_CHANNELS_PER_LISTENER_STREAM))) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      memcpy(sink->map, map, *map_len<<2);
    }
    else
      *map_len = sinks->stream.num_channels;
    memcpy(map, sink->map, *map_len<<2);
    return 1;
  }
  else
    return 0;
}


int get_device_media_clocks(int *a0)
{
  *a0 = AVB_NUM_MEDIA_CLOCKS;
  return 1;
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

int getset_device_media_clock_rate(int set, int media_clock_num, int *a2)
{
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(media_clock_svr)) {
    if (set) {
      media_clock_set_rate(media_clock_svr, media_clock_num, *a2);
    }
    *a2 = media_clock_get_rate(media_clock_svr, media_clock_num);
    return 1;
  }
  else
    return 0;
}


int getset_device_media_clock_state(int set, int media_clock_num, enum device_media_clock_state_t *a2) {
  // TODO: this doesn't maintain the state properly
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(media_clock_svr)) {
    if (set) {
      media_clock_set_state(media_clock_svr, media_clock_num, *a2);
    }
    *a2 = media_clock_get_state(media_clock_svr, media_clock_num);
    return 1;
  }
  else
    return 0;
}


int getset_device_media_clock_source(int set, int media_clock_num, int *a0)
{
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(media_clock_svr)) {
    if (set) {
      media_clock_set_source(media_clock_svr, media_clock_num, *a0);
    }
    media_clock_get_source(media_clock_svr, media_clock_num, a0);
    return 1;
  }
  else
    return 0;
}

int getset_device_media_clock_type(int set, int media_clock_num, enum device_media_clock_type_t *a2)
{
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(media_clock_svr)) {
    if (set) {
      media_clock_set_type(media_clock_svr, media_clock_num, *a2);
    }
    *a2 = media_clock_get_type(media_clock_svr, media_clock_num);
    return 1;
  }
  else
    return 0;
}


int get_media_ins(int *a0)
{
  *a0 = AVB_NUM_MEDIA_INPUTS;
  return 1;
}

int get_media_outs(int *a0)
{
  *a0 = AVB_NUM_MEDIA_OUTPUTS;
  return 1;
}

void avb_process_control_packet(unsigned int buf0[], int nbytes, chanend c_tx, unsigned int port_num)
{
  if (nbytes == STATUS_PACKET_LEN)
  {
    if (((unsigned char *)buf0)[0]) // Link up
    {
      avb_start();
    }
    else // Link down
    {
      for (int i=0; i < AVB_NUM_SOURCES; i++)
      {
        set_avb_source_state(i, AVB_SOURCE_STATE_DISABLED);
      }

      for (int i=0; i < AVB_NUM_SINKS; i++)
      {
        set_avb_sink_state(i, AVB_SOURCE_STATE_DISABLED);
      }      
    }
  }
  else
  {
    struct ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *) &buf0[0];
    unsigned char *buf = (unsigned char *) buf0;

    int has_qtag = ethernet_hdr->ethertype[1]==0x18;
    int eth_hdr_size = has_qtag ? 18 : 14;
    int etype;
    int len = nbytes - eth_hdr_size;

    if (has_qtag)
    {
      struct tagged_ethernet_hdr_t *tagged_ethernet_hdr = (tagged_ethernet_hdr_t *) &buf0[0];
      etype = (int)(tagged_ethernet_hdr->ethertype[0] << 8) + (int)(tagged_ethernet_hdr->ethertype[1]);
    }
    else
    {
      etype = (int)(ethernet_hdr->ethertype[0] << 8) + (int)(ethernet_hdr->ethertype[1]);
    }

    switch (etype)
    {
      /* fallthrough intended */
      case AVB_SRP_ETHERTYPE:
      // TODO: #define around MMRP, disabled by default
      case AVB_MMRP_ETHERTYPE:
      case AVB_MVRP_ETHERTYPE:
        avb_mrp_process_packet(&buf[eth_hdr_size], etype, len, port_num);
        break;
      case AVB_1722_ETHERTYPE:
        // We know that the cd field is true because the MAC filter only forwards
        // 1722 control to this thread
      #if AVB_ENABLE_1722_1
        avb_1722_1_process_packet(&buf[eth_hdr_size], &(ethernet_hdr->src_addr[0]), len, c_tx);
      #endif
        avb_1722_maap_process_packet(&buf[eth_hdr_size], &(ethernet_hdr->src_addr[0]), len, c_tx);
        break;
    }
  }

}


unsigned avb_control_get_mac_tx(void)
{
  return c_mac_tx;
}

unsigned avb_control_get_c_ptp(void)
{
  return c_ptp;
}

char *avb_control_get_my_mac_addr(void)
{
  return (char *) &mac_addr[0];
}


int get_avb_ptp_gm(unsigned char a0[])
{
  ptp_get_current_grandmaster(c_ptp, a0);
  return 1;
}

int get_avb_ptp_ports(int *a0)
{
  return 0;
}

int get_avb_ptp_rateratio(int *a0)
{
  return 0;
}

int get_avb_ptp_port_pdelay(int port, unsigned *pdelay)
{
  if (port == 0)
  {
    ptp_get_propagation_delay(c_ptp, pdelay);
    return 1;
  }
  else
  {
    return 0;
  }
}

unsigned avb_get_source_stream_index_from_pointer(void* ptr)
{
	avb_source_info_t* p = (avb_source_info_t*)ptr;
	for (unsigned i=0; i<AVB_NUM_SOURCES; ++i) {
		if (p == &sources[i]) return i;
	}
	return -1u;
}

unsigned avb_get_sink_stream_index_from_pointer(void* ptr)
{
	avb_sink_info_t* p = (avb_sink_info_t*)ptr;
	for (unsigned i=0; i<AVB_NUM_SINKS; ++i) {
		if (p == &sinks[i]) return i;
	}
	return -1u;
}
