#include <xccompat.h>

#define __AVB_C__
#include "avb.h"
#include "avb_srp.h"
#include "avb_mmrp.h"
#include "avb_mvrp.h"
#include "avb_mrp.h"
#include "avb_stream.h"
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

#ifdef AVB_ENABLE_1722_1
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
  char name[AVB_MAX_NAME_LEN];
#ifdef AVB_STORE_IO_TYPE_NAMES
  char type[AVB_MAX_NAME_LEN];
#endif
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

static mrp_attribute_state *domain_attr;

static void register_talkers(chanend talker_ctl[])
{
  for (int i=0;i<AVB_NUM_TALKER_UNITS;i++) {
    int core_id = xc_abi_inuint(talker_ctl[i]);
    int num_streams = xc_abi_inuint(talker_ctl[i]);
    for (int j=0;j<num_streams;j++) {
      avb_source_info_t *source = &sources[max_talker_stream_id];
      source->stream.state = AVB_SOURCE_STATE_DISABLED;
      source->talker_ctl = talker_ctl[i];
      source->stream.core_id = core_id;
      source->stream.local_id = j;
      source->stream.flags = 0;
      source->stream.streamId[0] = (mac_addr[0] << 24) | (mac_addr[1] << 16) | (mac_addr[2] <<  8) | (mac_addr[3] <<  0);
      source->stream.streamId[1] = (mac_addr[4] << 24) | (mac_addr[5] << 16) | ((source->stream.local_id & 0xffff)<<0);
      source->presentation = AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS;
      source->stream.vlan = AVB_DEFAULT_VLAN;
      source->stream.srp_talker_attr = mrp_get_attr();
      source->stream.srp_talker_failed_attr = mrp_get_attr();
      source->stream.srp_listener_attr = mrp_get_attr();
      mrp_attribute_init(source->stream.srp_talker_attr, MSRP_TALKER_ADVERTISE, source);
      mrp_attribute_init(source->stream.srp_talker_failed_attr, MSRP_TALKER_FAILED, source);
      mrp_attribute_init(source->stream.srp_listener_attr, MSRP_LISTENER, source);
      max_talker_stream_id++;
    }       
  }
  return;
}


static int max_link_id = 0;


static void register_listeners(chanend listener_ctl[])
{
  for (int i=0;i<AVB_NUM_LISTENER_UNITS;i++) {
    int core_id = xc_abi_inuint(listener_ctl[i]);
    int num_streams = xc_abi_inuint(listener_ctl[i]);
    for (int j=0;j<num_streams;j++) {
      avb_sink_info_t *sink = &sinks[max_listener_stream_id];
      sink->stream.state = AVB_SINK_STATE_DISABLED;
      sink->listener_ctl = listener_ctl[i];
      sink->stream.core_id = core_id;
      sink->stream.local_id = j;
      sink->stream.flags = 0;
      sink->stream.vlan = AVB_DEFAULT_VLAN;
      sink->stream.srp_talker_attr = mrp_get_attr();
      sink->stream.srp_talker_failed_attr = mrp_get_attr();
      sink->stream.srp_listener_attr = mrp_get_attr();
      mrp_attribute_init(sink->stream.srp_talker_attr, MSRP_TALKER_ADVERTISE, sink);
      mrp_attribute_init(sink->stream.srp_talker_failed_attr, MSRP_TALKER_FAILED, sink);
      mrp_attribute_init(sink->stream.srp_listener_attr, MSRP_LISTENER, sink);
      max_listener_stream_id++;
    }    
    xc_abi_outuint(listener_ctl[i], max_link_id);
    max_link_id++;
  }
  return;
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
      strcpy(inputs[input_id].name,"");
#ifdef AVB_STORE_IO_TYPE_NAMES
      strcpy(inputs[input_id].type,"");
#endif
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
      strcpy(outputs[output_id].name,"");
#ifdef AVB_STORE_IO_TYPE_NAMES
      strcpy(outputs[output_id].type,"");
#endif
      outputs[output_id].core_id = core_id;
      outputs[output_id].clk_ctl = clk_ctl;
      outputs[output_id].local_id = j;
      outputs[output_id].mapped_to = UNMAPPED;
      outputs[output_id].fifo = xc_abi_inuint(media_ctl[i]);
      output_id++;
    }
  }
  return;
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

#ifdef AVB_ENABLE_1722_1
  {
	  unsigned char serial_number[2] = { 0,1 };
	  avb_1722_1_init(mac_addr, serial_number);
  }
#endif

  c_mac_rx = c_mac_rx0;
  c_mac_tx = c_mac_tx0;
  c_ptp = c_ptp0;

  domain_attr = mrp_get_attr();
  mrp_attribute_init(domain_attr, MSRP_DOMAIN_VECTOR, NULL);

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

void avb_periodic(avb_status_t *status)
{
	mrp_periodic(status);
#ifdef AVB_ENABLE_1722_1
	avb_1722_1_periodic(status, c_mac_tx, c_ptp);
#endif
	avb_1722_maap_periodic(status, c_mac_tx);
}

void avb_start(void)
{
#if AVB_ENABLE_1722_1
  avb_1722_1_adp_announce();
#endif
  // Request a multicast addresses for stream transmission
  avb_1722_maap_request_addresses(AVB_NUM_SOURCES, NULL);

  mrp_mad_begin(domain_attr);
  mrp_mad_join(domain_attr, 1);

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
                             enum avb_source_format_t *format,
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
    source->stream.rate = *rate;
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
      source->stream.vlan = *vlan;
    }
    *vlan = source->stream.vlan;
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

        clk_ctl = inputs[source->stream.map[0]].clk_ctl;

        // check that the map is ok
        for (int i=0;i<source->stream.num_channels;i++) {
          if (inputs[source->stream.map[i]].mapped_to != UNMAPPED)
            valid = 0;
          if (inputs[source->stream.map[i]].clk_ctl != clk_ctl)
            valid = 0;
        }
        

        if (valid) {
          chanend c = source->talker_ctl;
          unsigned fifo_mask = 0;

          for (int i=0;i<source->stream.num_channels;i++) {
            inputs[source->stream.map[i]].mapped_to = source_num;
            fifo_mask |= (1 << source->stream.map[i]);
          }
          
          xc_abi_outuint(c, AVB1722_CONFIGURE_TALKER_STREAM);
          xc_abi_outuint(c, source->stream.local_id);
          xc_abi_outuint(c, source->stream.format);
          for (int i=0; i < 6;i++) {
            xc_abi_outuint(c, source->dest[i]);
          }
          xc_abi_outuint(c, source_num);
          xc_abi_outuint(c, source->stream.num_channels);
          xc_abi_outuint(c, fifo_mask);
          for (int i=0;i<source->stream.num_channels;i++) {
            xc_abi_outuint(c, inputs[source->stream.map[i]].fifo);
          }
          xc_abi_outuint(c, source->stream.rate);
          if (source->presentation)
            xc_abi_outuint(c, source->presentation);
          else
            xc_abi_outuint(c, AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS);
          
          (void) xc_abi_inuint(c);

          

          mrp_mad_begin(source->stream.srp_talker_attr);
          mrp_mad_begin(source->stream.srp_talker_failed_attr);
          mrp_mad_begin(source->stream.srp_listener_attr);
          mrp_mad_join(source->stream.srp_talker_attr, 1);

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
            inputs[source->stream.map[i]].mapped_to = UNMAPPED;
          }
          chanend c = source->talker_ctl;
          xc_abi_outuint(c, AVB1722_TALKER_STOP);
          xc_abi_outuint(c, source->stream.local_id);
          (void) xc_abi_inuint(c); //ACK

          // And remove the group
          mrp_mad_leave(source->stream.srp_talker_attr);
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

int getset_avb_source_name(int set, int source_num, char a2[])
{
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {
      strncpy(source->name, a2, AVB_MAX_NAME_LEN);
    }
    strncpy(a2, source->name, AVB_MAX_NAME_LEN);
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
        *map_len <= AVB_MAX_CHANNELS_PER_STREAM))) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {      
      memcpy(source->stream.map, map, *map_len<<2);
    }      
    else {
      *map_len = source[source_num].stream.num_channels;
    }
    memcpy(map, source->stream.map, *map_len<<2);
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
      for(int i=0;i<6;i++)
        source->dest[i]=dest[i];
    }      
    *dest_len = 6;
    for(int i=0;i<6;i++)
      dest[i]=source->dest[i];
    return 1;
  }
  else 
    return 0;
}

int get_avb_source_id(int source_num, unsigned int a1[2])
{
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num];    

    memcpy(a1, source->stream.streamId, 8);
      
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
      memcpy(sink->stream.streamId, a1, 8);
    }      
    memcpy(a1, sink->stream.streamId, 8);
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
      sink->stream.vlan = *vlan;
    }
    *vlan = sink->stream.vlan;
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
      for(int i=0;i<6;i++)
        sink->addr[i]=addr[i];
    }      
    *addr_len = 6;
    for(int i=0;i<6;i++)
      addr[i]=sink->addr[i];
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
        int clk_ctl = -1;
        simple_printf("Listener sink #%d chan map:\n", sink_num);
        xc_abi_outuint(c, AVB1722_CONFIGURE_LISTENER_STREAM);
        xc_abi_outuint(c, sink->stream.local_id);
        xc_abi_outuint(c, sink->stream.sync);
        xc_abi_outuint(c, sink->stream.rate);
        xc_abi_outuint(c, sink->stream.num_channels);
        for (int i=0;i<sink->stream.num_channels;i++)
        {
          if (sink->stream.map[i] == AVB_CHANNEL_UNMAPPED)
          {
            xc_abi_outuint(c, 0);
            simple_printf("  %d unmapped\n", i);
          }
          else
          {
            if (clk_ctl == -1)
            {
              clk_ctl = outputs[sink->stream.map[i]].clk_ctl;
            }
            xc_abi_outuint(c, outputs[sink->stream.map[i]].fifo);
            simple_printf("  %d -> %x\n", i, sink->stream.map[i]);
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
                                      sink->stream.streamId,
                                      router_link,
                                      sink->stream.local_id);
        }

#ifndef AVB_EXCLUDE_MVRP
        if (sink->stream.vlan)
          avb_join_vlan(sink->stream.vlan);
#endif

#ifdef AVB_INCLUDE_MMRP
        if (sink->addr[0] & 1) 
          avb_join_multicast_group(sink->addr);
#endif

        mrp_mad_begin(sink->stream.srp_talker_attr);
        mrp_mad_begin(sink->stream.srp_talker_failed_attr);
        mrp_mad_begin(sink->stream.srp_listener_attr);
        mrp_mad_join(sink->stream.srp_listener_attr, 1);
      }
      else if (sink->stream.state != AVB_SINK_STATE_DISABLED &&
              *state == AVB_SINK_STATE_DISABLED) {

		  chanend c = sink->listener_ctl;
		  xc_abi_outuint(c, AVB1722_DISABLE_LISTENER_STREAM);
		  xc_abi_outuint(c, sink->stream.local_id);
		  (void) xc_abi_inuint(c);

    	  mrp_mad_leave(sink->stream.srp_listener_attr);

#ifdef AVB_INCLUDE_MMRP
        if (sink->addr[0] & 1)
          avb_leave_multicast_group(sink->addr);
#endif

#ifndef AVB_EXCLUDE_MVRP
        if (sink->stream.vlan)
          avb_leave_vlan(sink->stream.vlan);
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

int getset_avb_sink_name(int set, int sink_num, char a2[])
{
  if (sink_num < AVB_NUM_SINKS) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {
      strncpy(sink->name, a2, AVB_MAX_NAME_LEN);
    }
    strncpy(a2, sink->name,  AVB_MAX_NAME_LEN);
    return 1;
  }
  else 
    return 0;
}

int getset_avb_sink_map(int set, int sink_num, int map[], int *map_len) 
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || (sinks[sink_num].stream.state == AVB_SINK_STATE_DISABLED
                && *map_len <= AVB_MAX_CHANNELS_PER_STREAM))) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {      
      memcpy(sink->stream.map, map, *map_len<<2);
    }      
    else
      *map_len = sinks->stream.num_channels;
    memcpy(map, sink->stream.map, *map_len<<2);
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

int getset_media_in_name(int set, int input_num, char a2[])
{
  if (input_num < AVB_NUM_MEDIA_INPUTS) {
    media_info_t *media = &inputs[input_num];
    if (set) {
      strncpy(media->name, a2, AVB_MAX_NAME_LEN);
    }
    strncpy(a2, media->name, AVB_MAX_NAME_LEN);
    return 1;
  }
  else 
    return 0;
}

int getset_media_in_type(int set, int input_num, char a2[])
{
#ifndef AVB_STORE_IO_TYPE_NAMES
  return 0;
#else
  if (input_num < AVB_NUM_MEDIA_INPUTS) {
    media_info_t *media = &inputs[input_num];
    if (set) {
      strncpy(media->type, a2, AVB_MAX_NAME_LEN);
      return 0;
    }
    strncpy(a2, media->type,AVB_MAX_NAME_LEN);
    return 1;
  }
  else 
    return 0;

#endif
}

int get_media_in_type(int h, char a2[]) {
  return getset_media_in_type(0, h, a2);
}


int get_media_outs(int *a0)
{
  *a0 = AVB_NUM_MEDIA_OUTPUTS;
  return 1;
}

int getset_media_out_name(int set, int output_num, char a2[])
{
  if (output_num < AVB_NUM_MEDIA_OUTPUTS) {
    media_info_t *media = &outputs[output_num];
    if (set) {
      strncpy(media->name, a2, AVB_MAX_NAME_LEN);
    }
    strncpy(a2, media->name, AVB_MAX_NAME_LEN);
    return 1;
  }
  else 
    return 0;
}

int getset_media_out_type(int set, int output_num, char a2[])
{
#ifndef AVB_STORE_IO_TYPE_NAMES
  return 0;
#else
  if (output_num < AVB_NUM_MEDIA_OUTPUTS) {
    media_info_t *media = &outputs[output_num];
    if (set) {
      strncpy(media->type, a2, AVB_MAX_NAME_LEN);
    }
    strncpy(a2, media->type, AVB_MAX_NAME_LEN);

    return 1;
  }
  else 
    return 0;
#endif
}

int get_media_out_type(int h, char a2[]) {
  return getset_media_out_type(0, h, a2);
}


void avb_set_legacy_mode(int mode)
{
  ptp_set_legacy_mode(c_ptp, mode);
  avb_mrp_set_legacy_mode(mode);
}

void avb_process_control_packet(avb_status_t *status, unsigned int buf0[], int nbytes, chanend c_tx)
{
  if (nbytes == STATUS_PACKET_LEN)
  {
    if (buf0[0]) // Link up
    {
      avb_start();
    }
    else // Link down
    {
      for(int i=0; i < AVB_NUM_SOURCES; i++)
      {
        set_avb_source_state(i, AVB_SOURCE_STATE_DISABLED);
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
        avb_mrp_process_packet(status, &buf[eth_hdr_size], etype, len);
        break;
      case AVB_1722_ETHERTYPE:
        // We know that the cd field is true because the MAC filter only forwards
        // 1722 control to this thread
      #ifdef AVB_ENABLE_1722_1
        avb_1722_1_process_packet(status, &buf[eth_hdr_size], &(ethernet_hdr->src_addr[0]), len, c_tx);
      #endif
        avb_1722_maap_process_packet(status, &buf[eth_hdr_size], &(ethernet_hdr->src_addr[0]), len, c_tx);
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


int get_avb_ptp_gm(unsigned char a0[], int *a0_len)
{
  return 0;
}

int get_avb_ptp_ports(int *a0)
{
  return 0;
}

int get_avb_ptp_rateratio(int *a0)
{
  return 0;
}

int get_avb_ptp_port_pdelay(int h0,int *a0)
{
  return 0;
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
