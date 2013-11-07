#include "avb.h"
#include <xccompat.h>
#include "avb_srp.h"
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
#include "avb_api.h"
#include "avb_srp_interface.h"

#if AVB_ENABLE_1722_1
#include "avb_1722_1.h"
#include "avb_1722_1_adp.h"
#endif

//#define AVB_TRANSMIT_BEFORE_RESERVATION 1

#define UNMAPPED (-1)
#define AVB_CHANNEL_UNMAPPED (-1)

typedef struct media_info_t {
  int tile_id;
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

static unsafe void register_talkers(chanend talker_ctl[], unsigned char mac_addr[6])
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
      source->reservation.tspec = (AVB_SRP_TSPEC_PRIORITY_DEFAULT << 5 |
          AVB_SRP_TSPEC_RANK_DEFAULT << 4 |
          AVB_SRP_TSPEC_RESERVED_VALUE);
      source->reservation.tspec_max_interval = AVB_SRP_MAX_INTERVAL_FRAMES_DEFAULT;
      source->reservation.accumulated_latency = AVB_SRP_ACCUMULATED_LATENCY_DEFAULT;
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
      max_listener_stream_id++;
    }
    listener_ctl[i] <: max_link_id;
    max_link_id++;
  }
}

static void register_media(chanend media_ctl[])
{
  unsafe {
    int input_id = 0;
    int output_id = 0;

    for (int i=0;i<AVB_NUM_MEDIA_UNITS;i++) {
      int tile_id;
      int num_in;
      int num_out;
      chanend *unsafe clk_ctl;
      media_ctl[i] :> tile_id;
      media_ctl[i] :> clk_ctl;
      media_ctl[i] :> num_in;

      for (int j=0;j<num_in;j++) {
        media_ctl[i] <: input_id;
        inputs[input_id].tile_id = tile_id;
        inputs[input_id].clk_ctl = clk_ctl;
        inputs[input_id].local_id = j;
        inputs[input_id].mapped_to = UNMAPPED;
        media_ctl[i] :> inputs[input_id].fifo;
        input_id++;

      }
      media_ctl[i] :> num_out;
      for (int j=0;j<num_out;j++) {
        media_ctl[i] <: output_id;
        outputs[output_id].tile_id = tile_id;
        outputs[output_id].clk_ctl = clk_ctl;
        outputs[output_id].local_id = j;
        outputs[output_id].mapped_to = UNMAPPED;
        media_ctl[i] :> outputs[output_id].fifo;
        output_id++;
      }
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

void avb_init(chanend c_media_ctl[],
              chanend (&?c_listener_ctl)[],
              chanend (&?c_talker_ctl)[],
              chanend ?c_media_clock_ctl,
              chanend c_ptp,
              chanend c_mac_tx)
{
  unsigned char mac_addr[6];
  mac_get_macaddr(c_mac_tx, mac_addr);
  unsafe {
    register_talkers(c_talker_ctl, mac_addr);
    register_listeners(c_listener_ctl);
  }
}

static void set_sink_state0(unsigned sink_num,
                            enum avb_sink_state_t state,
                            chanend c_mac_tx,
                            chanend ?c_media_clock_ctl,
                            client interface srp_interface i_srp) {
  unsafe {
    avb_sink_info_t *sink = &sinks[sink_num];
    chanend *unsafe c = sink->listener_ctl;
    if (sink->stream.state == AVB_SINK_STATE_DISABLED &&
        state == AVB_SINK_STATE_POTENTIAL) {

      chanend *unsafe clk_ctl = outputs[sink->map[0]].clk_ctl;
      simple_printf("Listener sink #%d chan map:\n", sink_num);
      master {
        *c <: AVB1722_CONFIGURE_LISTENER_STREAM;
        *c <: (int)sink->stream.local_id;
        *c <: (int)sink->stream.sync;
        *c <: sink->stream.rate;
        *c <: (int)sink->stream.num_channels;

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
      }

      if (!isnull(c_media_clock_ctl)) {
        media_clock_register(c_media_clock_ctl, clk_ctl, sink->stream.sync);
      }

      int router_link;

      master {
        *c <: AVB1722_GET_ROUTER_LINK;
        *c :> router_link;
      }

      avb_1722_add_stream_mapping(c_mac_tx,
                                  sink->reservation.stream_id,
                                  router_link,
                                  sink->stream.local_id);

      for (int i=0; i < MRP_NUM_PORTS; i++) {
        if (sink->reservation.vlan_id) {
          avb_join_vlan(sink->reservation.vlan_id, i);
        }
      }

        i_srp.register_attach_request(sink->reservation.stream_id);

    }
    else if (sink->stream.state != AVB_SINK_STATE_DISABLED &&
            state == AVB_SINK_STATE_DISABLED) {

      master {
        *c <: AVB1722_DISABLE_LISTENER_STREAM;
        *c <: (int)sink->stream.local_id;
      }

      i_srp.deregister_attach_request(sink->reservation.stream_id);

      avb_1722_remove_stream_mapping(c_mac_tx, sink->reservation.stream_id);

  #if NUM_MRP_PORTS == 1
      if (sink->reservation.vlan_id) {
        avb_leave_vlan(sink->reservation.vlan_id);
      }
  #endif
    }
    sink->stream.state = state;
  }
}

static unsigned avb_srp_calculate_max_framesize(avb_source_info_t *source_info)
{
#if defined(AVB_1722_FORMAT_61883_6) || defined(AVB_1722_FORMAT_SAF)
  const unsigned samples_per_packet = (AVB_MAX_AUDIO_SAMPLE_RATE + (AVB1722_PACKET_RATE-1))/AVB1722_PACKET_RATE;
  return AVB1722_PLUS_SIP_HEADER_SIZE + (source_info->stream.num_channels * samples_per_packet * 4);
#endif
#if defined(AVB_1722_FORMAT_61883_4)
  return AVB1722_PLUS_SIP_HEADER_SIZE + (192 * MAX_TS_PACKETS_PER_1722);
#endif
}

static void local_set_source_state(unsigned source_num,
                                  enum avb_source_state_t state,
                                  chanend c_mac_tx,
                                  chanend ?c_media_clock_ctl,
                                  client interface srp_interface i_srp) {
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

        master {
          *c <: AVB1722_CONFIGURE_TALKER_STREAM;
          *c <: (int)source->stream.local_id;
          *c <: (int)source->stream.format;

          for (int i=0; i < 6;i++) {
            *c <: (int)source->reservation.dest_mac_addr[i];
          }

          *c <: source_num;
          *c <: (int)source->stream.num_channels;
          *c <: fifo_mask;

          for (int i=0;i<source->stream.num_channels;i++) {
            *c <: inputs[source->map[i]].fifo;
          }
          *c <: (int)source->stream.rate;

          if (source->presentation)
            *c <: source->presentation;
          else
            *c <: AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS;
        }

        for (int i=0; i < MRP_NUM_PORTS; i++) {
          if (source->reservation.vlan_id) {
            avb_join_vlan(source->reservation.vlan_id, i);
          }
        }

        source->reservation.tspec_max_frame_size = avb_srp_calculate_max_framesize(source);
        i_srp.register_stream_request(source->reservation);

        if (!isnull(c_media_clock_ctl)) {
          media_clock_register(c_media_clock_ctl, clk_ctl, source->stream.sync);
        }

    #if defined(AVB_TRANSMIT_BEFORE_RESERVATION)
        master {
          *c <: AVB1722_TALKER_GO;
          *c <: (int)source->stream.local_id;

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

        master {
          *c <: AVB1722_TALKER_STOP;
          *c <: (int)source->stream.local_id;
        }

        printstr(stream_string); simple_printf("#%d off\n", source_num);
    }
    else if (source->stream.state == AVB_SOURCE_STATE_POTENTIAL &&
             state == AVB_SOURCE_STATE_ENABLED) {
      // start transmitting

      printstr(stream_string); simple_printf("#%d on\n", source_num);
      master {
        *c <: AVB1722_TALKER_GO;
        *c <: (int)source->stream.local_id;
      }
    }
    else if (source->stream.state != AVB_SOURCE_STATE_DISABLED &&
             state == AVB_SOURCE_STATE_DISABLED) {
      // disabled the source
        for (int i=0;i<source->stream.num_channels;i++) {
          inputs[source->map[i]].mapped_to = UNMAPPED;
        }

        master {
          *c <: AVB1722_TALKER_STOP;
          *c <: (int)source->stream.local_id;
        }

        printstr(stream_string); simple_printf("#%d off\n", source_num);

    #if MRP_NUM_PORTS == 1
      if (source->reservation.vlan_id) {
        avb_leave_vlan(source->reservation.vlan_id);
      }
    #endif

      i_srp.deregister_stream_request(source->reservation.stream_id);
        
    }
    source->stream.state = state;
  }
}

// Wrappers for interface calls from C
int avb_get_source_state(client interface avb_interface avb, unsigned source_num, enum avb_source_state_t &state) {
  return avb.get_source_state(source_num, state);
}

int avb_set_source_state(client interface avb_interface avb, unsigned source_num, enum avb_source_state_t state) {
  return avb.set_source_state(source_num, state);
}

// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

[[combinable]]
void avb_manager(server interface avb_interface avb[num_avb_clients], unsigned num_avb_clients,
                 client interface srp_interface i_srp,
                 chanend c_media_ctl[],  
                 chanend (&?c_listener_ctl)[],
                 chanend (&?c_talker_ctl)[],
                 chanend c_mac_tx,
                 chanend ?c_media_clock_ctl,
                 chanend c_ptp) {

  register_media(c_media_ctl);
  init_media_clock_server(c_media_clock_ctl);

  while (1) {
    select {
      case avb[int i].initialise(void): {
        unsafe {
          avb_init(c_media_ctl, c_listener_ctl, c_talker_ctl, c_media_clock_ctl, c_ptp, c_mac_tx);
        }
        break;
      }
      case avb[int i].get_source_format(unsigned source_num, enum avb_stream_format_t &format, int &rate) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          format = source->stream.format;
          rate = source->stream.rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_format(unsigned source_num, enum avb_stream_format_t format, int rate) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->stream.format = format;
          source->stream.rate = rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_channels(unsigned source_num, int &channels) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          channels = source->stream.num_channels;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_channels(unsigned source_num, int channels) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->stream.num_channels = channels;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_sync(unsigned source_num, int &sync) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          sync = source->stream.sync;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_sync(unsigned source_num, int sync) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->stream.sync = sync;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_presentation(unsigned source_num, int &presentation) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          presentation = source->presentation;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_presentation(unsigned source_num, int presentation) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->presentation = presentation;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_vlan(unsigned source_num, int &vlan) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          vlan = source->reservation.vlan_id;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_vlan(unsigned source_num, int vlan) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED) {
          avb_source_info_t *source = &sources[source_num];
          source->reservation.vlan_id = vlan;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }
      case avb[int i].get_source_state(unsigned source_num, enum avb_source_state_t &state) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          state = source->stream.state;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_state(unsigned source_num, enum avb_source_state_t state) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          unsafe {
            local_set_source_state(source_num, state, c_mac_tx, c_media_clock_ctl, i_srp);
          }
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_map(unsigned source_num, int map[], int &len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          len = source[source_num].stream.num_channels;
          memcpy(map, source->map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }      
      case avb[int i].set_source_map(unsigned source_num, int map[len], unsigned len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED &&
          len <= AVB_MAX_CHANNELS_PER_TALKER_STREAM) {
          avb_source_info_t *source = &sources[source_num];
          memcpy(source->map, map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_dest(unsigned source_num, unsigned char addr[], int &len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          len = 6;
          memcpy(addr, source->reservation.dest_mac_addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_source_dest(unsigned source_num, unsigned char addr[len], unsigned len) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES && sources[source_num].stream.state == AVB_SOURCE_STATE_DISABLED &&
          len == 6) {
          avb_source_info_t *source = &sources[source_num];
          memcpy(source->reservation.dest_mac_addr, addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_source_id(unsigned source_num, unsigned int id[2]) -> int return_val: {
        if (source_num < AVB_NUM_SOURCES) {
          avb_source_info_t *source = &sources[source_num];
          memcpy(id, source->reservation.stream_id, 8);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_sink_id(unsigned sink_num, unsigned int stream_id[2]) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(stream_id, sink->reservation.stream_id, 8);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_sink_id(unsigned sink_num, unsigned int stream_id[2]) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(sink->reservation.stream_id, stream_id, 8);
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_sink_format(unsigned sink_num, enum avb_stream_format_t &format, int &rate) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          format = sink->stream.format;
          rate = sink->stream.rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_sink_format(unsigned sink_num, enum avb_stream_format_t format, int rate) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->stream.format = format;
          sink->stream.rate = rate;
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_sink_channels(unsigned sink_num, int &channels) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          channels = sink->stream.num_channels;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }
      case avb[int i].set_sink_channels(unsigned sink_num, int channels) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->stream.num_channels = channels;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }
      case avb[int i].get_sink_sync(unsigned sink_num, int &sync) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sync = sink->stream.sync;
          return_val = 1;
        }
        else return_val = 0;
        break;        
      }
      case avb[int i].set_sink_sync(unsigned sink_num, int sync) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->stream.sync = sync;
          return_val = 1;
        }
        else return_val = 0;
        break;      
      }      
      case avb[int i].get_sink_vlan(unsigned sink_num, int &vlan) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          vlan = sink->reservation.vlan_id;
          return_val = 1;
        }
        else return_val = 0;
        break;       
      }
      case avb[int i].set_sink_vlan(unsigned sink_num, int vlan) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED) {
          avb_sink_info_t *sink = &sinks[sink_num];
          sink->reservation.vlan_id = vlan;
          return_val = 1;
        }
        else return_val = 0;
        break;       
      }
      case avb[int i].get_sink_addr(unsigned sink_num, unsigned char addr[], int &len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          len = 6;
          memcpy(addr, sink->reservation.dest_mac_addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;        
      }
      case avb[int i].set_sink_addr(unsigned sink_num, unsigned char addr[len], unsigned len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED && len == 6) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(sink->reservation.dest_mac_addr, addr, 6);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb[int i].get_sink_state(unsigned sink_num, enum avb_sink_state_t &state) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          state = sink->stream.state; 
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].set_sink_state(unsigned sink_num, enum avb_sink_state_t state) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          unsafe {
            set_sink_state0(sink_num, state, c_mac_tx, c_media_clock_ctl, i_srp);
          }
          return_val = 1;
        }
        else return_val = 0;
        break;
      }
      case avb[int i].get_sink_map(unsigned sink_num, int map[], int &len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS) {
          avb_sink_info_t *sink = &sinks[sink_num];
          len = sinks->stream.num_channels;
          memcpy(map, sink->map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;     
      }
      case avb[int i].set_sink_map(unsigned sink_num, int map[len], unsigned len) -> int return_val: {
        if (sink_num < AVB_NUM_SINKS && sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED && 
            len <= AVB_MAX_CHANNELS_PER_LISTENER_STREAM) {
          avb_sink_info_t *sink = &sinks[sink_num];
          memcpy(sink->map, map, len<<2);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb[int i].get_device_media_clock_rate(int clock_num, int &rate) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          rate = media_clock_get_rate(c_media_clock_ctl, clock_num);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb[int i].set_device_media_clock_rate(int clock_num, int rate) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_rate(c_media_clock_ctl, clock_num, rate);
          return_val = 1;
        }
        else return_val = 0;
        break;          
      }
      case avb[int i].get_device_media_clock_state(int clock_num, enum device_media_clock_state_t &state) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          state = media_clock_get_state(c_media_clock_ctl, clock_num);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb[int i].set_device_media_clock_state(int clock_num, enum device_media_clock_state_t state) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_state(c_media_clock_ctl, clock_num, state);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb[int i].get_device_media_clock_source(int clock_num, int &source) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          int source0 = source;
          media_clock_get_source(c_media_clock_ctl, clock_num, source0);
          source = source0;
          return_val = 1;
        }
        else return_val = 0;
        break;         
      }
      case avb[int i].set_device_media_clock_source(int clock_num, int source) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          media_clock_set_source(c_media_clock_ctl, clock_num, source);
          return_val = 1;
        }
        else return_val = 0;
        break;          
      }
      case avb[int i].get_device_media_clock_type(int clock_num, enum device_media_clock_type_t &clock_type) -> int return_val: {
        if (clock_num < AVB_NUM_MEDIA_CLOCKS && !isnull(c_media_clock_ctl)) {
          clock_type = media_clock_get_type(c_media_clock_ctl, clock_num);
          return_val = 1;
        }
        else return_val = 0;
        break;  
      }
      case avb[int i].set_device_media_clock_type(int clock_num, enum device_media_clock_type_t clock_type) -> int return_val: {
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

int set_avb_source_port(unsigned source_num,
                        int srcport) {
  unsafe {
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num];
    chanend *unsafe c = source->talker_ctl;
    master {
      *c <: AVB1722_SET_PORT;
      *c <: (int)source->stream.local_id;
      *c <: srcport;
    }

    return 1;
  }
  else
    return 0;
  }
}

#ifdef MEDIA_OUTPUT_FIFO_VOLUME_CONTROL
void set_avb_source_volumes(unsigned sink_num, int volumes[], int count)
{
	if (sink_num < AVB_NUM_SINKS) {
    unsafe {
      avb_sink_info_t *sink = &sinks[sink_num];
      chanend *unsafe c = sink->listener_ctl;
      *c <: AVB1722_ADJUST_LISTENER_STREAM;
      *c <: sink->stream.local_id;
      *c <: AVB1722_ADJUST_LISTENER_VOLUME;
      *c <: count;
      for (int i=0;i<count;i++) {
        *c <:  volumes[i];
      }
    }
	}
}
#endif


void avb_process_1722_control_packet(unsigned int buf0[],
                                     unsigned nbytes,
                                     chanend c_tx,
                                     client interface avb_interface i_avb,
                                     client interface avb_1722_1_control_callbacks i_1722_1_entity,
                                     client interface spi_interface i_spi) {
  if (nbytes == STATUS_PACKET_LEN) {
    #if 0
    if (((unsigned char *)buf0)[0]) { // Link up
      avb_1722_1_adp_init();
      avb_1722_1_adp_depart_then_announce();
      avb_1722_1_adp_discover_all();
    }
    #endif
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

    switch (etype) {
      case AVB_1722_ETHERTYPE:
        avb_1722_1_process_packet(&buf[eth_hdr_size], len, ethernet_hdr->src_addr, c_tx, i_avb, i_1722_1_entity, i_spi);
        avb_1722_maap_process_packet(&buf[eth_hdr_size], len, ethernet_hdr->src_addr, c_tx);
        break;
    }
  }  
}

void avb_process_control_packet(client interface avb_interface avb, unsigned int buf0[], unsigned nbytes, chanend c_tx, unsigned int port_num)
{
  if (nbytes == STATUS_PACKET_LEN) {
    if (((unsigned char *)buf0)[0]) { // Link up
      srp_domain_join();
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

    switch (etype) {
      /* fallthrough intended */
      case AVB_SRP_ETHERTYPE:
      case AVB_MVRP_ETHERTYPE:
        avb_mrp_process_packet(&buf[eth_hdr_size], etype, len, port_num);
        break;
    }
  }

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

void avb_get_control_packet(chanend c_rx, 
                            unsigned int buf[],
                            unsigned int &nbytes,
                            unsigned int &port_num)
{
  safe_mac_rx(c_rx, 
              (buf, unsigned char[]), 
              nbytes,
              port_num,                       
              MAX_AVB_CONTROL_PACKET_SIZE);
}

int avb_register_listener_streams(chanend listener_ctl,
                                   int num_streams)
{
  int tile_id;
  int link_id;
  tile_id = get_local_tile_id();
  listener_ctl <: tile_id;
  listener_ctl <: num_streams;
  listener_ctl :> link_id;
  return link_id;
}

void avb_register_talker_streams(chanend talker_ctl,
                                 int num_streams)
{
  int tile_id;
  tile_id = get_local_tile_id();
  talker_ctl <: tile_id;
  talker_ctl <: num_streams;
}
