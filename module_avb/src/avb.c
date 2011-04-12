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
#include "osc_types.h"
#include "osc_tree.h"
#include "mac_custom_filter.h"
#include "avb_1722_maap.h"

//#define AVB_TRANSMIT_BEFORE_RESERVATION 1

#define UNMAPPED (-1)

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
      source->state = AVB_SOURCE_STATE_DISABLED;
      source->talker_ctl = talker_ctl[i];
      source->core_id = core_id;
      source->local_id = j;
      source->presentation = AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS;
      source->vlan = AVB_DEFAULT_VLAN;
      source->srp_attr = mrp_get_attr();
      mrp_attribute_init(source->srp_attr, 
                         MSRP_TALKER_ADVERTISE, 
                         source);      
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
      sink->state = AVB_SINK_STATE_DISABLED;
      sink->listener_ctl = listener_ctl[i];
      sink->core_id = core_id;
      sink->local_id = j;
      sink->srp_attr = mrp_get_attr();
      mrp_attribute_init(sink->srp_attr,
    		  MSRP_LISTENER,
    		  sink);
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
  for (int i=0;i<AVB_NUM_MEDIA_OUTPUTS;i++) {
    xc_abi_outuint(media_clock_svr, outputs[i].fifo);
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

#if AVB_OSC
  OSC_SET_RANGE(avb_source, 0, AVB_NUM_SOURCES-1);
  OSC_SET_RANGE(device_media_clock, 0, AVB_NUM_MEDIA_CLOCKS-1);
  OSC_SET_RANGE(avb_sink, 0, AVB_NUM_SINKS-1);
  OSC_SET_RANGE(media_in, 0, AVB_NUM_MEDIA_INPUTS-1);
  OSC_SET_RANGE(media_out, 0, AVB_NUM_MEDIA_OUTPUTS-1);
#endif

  mac_get_macaddr(c_mac_tx0, mac_addr);

  mrp_init((char *)mac_addr);
  register_talkers(talker_ctl);
  register_listeners(listener_ctl);
  register_media(media_ctl);
  init_media_clock_server(media_clock_ctl);

  avb_1722_maap_init(mac_addr);

  c_mac_rx = c_mac_rx0;
  c_mac_tx = c_mac_tx0;
  c_ptp = c_ptp0;

  domain_attr = mrp_get_attr();
  mrp_attribute_init(domain_attr, 
                     MSRP_DOMAIN_VECTOR, 
                     NULL);      

  avb_mmrp_init();
  avb_mvrp_init();

  xc_abi_outuint(c_mac_tx, ETHERNET_TX_INIT_AVB_ROUTER);

  mac_set_custom_filter(c_mac_rx, MAC_FILTER_AVB_CONTROL);
}

avb_status_t avb_periodic(void) {
  mrp_periodic();
  return avb_1722_maap_periodic(c_mac_tx);  
}



void avb_start(void) {
  avb_1722_maap_rerequest_addresses();
  
  
  //  avb_srp_domain_start();
  mrp_mad_new(domain_attr);
  mrp_mad_join(domain_attr);       
}

static void avb_set_talker_bandwidth() 
{
  int data_size = 0;
  for (int i=0;i<AVB_NUM_SOURCES;i++) {
    avb_source_info_t *source = &sources[i];
    if (source->state == AVB_SOURCE_STATE_POTENTIAL ||
        source->state == AVB_SOURCE_STATE_ENABLED)
      {
        int samples_per_packet = (source->rate + (AVB1722_PACKET_RATE-1))/AVB1722_PACKET_RATE;
        data_size += 18 + 32 + (source->num_channels * samples_per_packet * 4);
      }
  }
  mac_set_qav_bandwidth(c_mac_tx, (data_size*8*AVB1722_PACKET_RATE*102)/100);  
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
      (!set || sources[source_num].state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num]; 
    if (set) {
      source->format = *format;
      source->rate = *rate;
    }
    *format = source->format;
    source->rate = *rate;
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
      (!set || sources[source_num].state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num]; 
    if (set) {
      source->num_channels = *channels;
    }
    *channels = source->num_channels;
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
      (!set || sources[source_num].state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num]; 
    if (set) {
      source->sync = *sync;
    }
    *sync = source->sync;
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
      (!set || sources[source_num].state == AVB_SOURCE_STATE_DISABLED)) {
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
      (!set || sources[source_num].state == AVB_SOURCE_STATE_DISABLED)) {
    avb_source_info_t *source = &sources[source_num]; 
    if (set) {
      source->vlan = *vlan;
    }
    *vlan = source->vlan;
    return 1;
  }
  else 
    return 0;
}

int getset_avb_source_state(int set, 
                            int source_num, 
                            enum avb_source_state_t *state)
{
  if (source_num < AVB_NUM_SOURCES) {
    avb_source_info_t *source = &sources[source_num]; 
    if (set) {      
      if (source->state == AVB_SOURCE_STATE_DISABLED &&
          *state == AVB_SOURCE_STATE_POTENTIAL) {
        // enable the source
        int valid = 1;
        int clk_ctl;

        if (source->num_channels <= 0)
          valid = 0;

        clk_ctl = inputs[source->map[0]].clk_ctl;

        // check that the map is ok
        for (int i=0;i<source->num_channels;i++) {
          if (inputs[source->map[i]].mapped_to != UNMAPPED)
            valid = 0;
          if (inputs[source->map[i]].clk_ctl != clk_ctl)
            valid = 0;
        }
        

        if (valid) {
          chanend c = source->talker_ctl;
          for (int i=0;i<source->num_channels;i++) {
            inputs[source->map[i]].mapped_to = source_num;
          }
          
          xc_abi_outuint(c, AVB1722_CONFIGURE_TALKER_STREAM);
          xc_abi_outuint(c, source->local_id);
          xc_abi_outuint(c, source->format);
          for (int i=0; i < 6;i++) {
            xc_abi_outuint(c, source->dest[i]);
          }
          xc_abi_outuint(c, source_num);
          xc_abi_outuint(c, source->num_channels);
          for (int i=0;i<source->num_channels;i++) {
            xc_abi_outuint(c, inputs[source->map[i]].fifo);
          }
          xc_abi_outuint(c, source->rate);
          if (source->presentation)
            xc_abi_outuint(c, source->presentation);
          else
            xc_abi_outuint(c, AVB_DEFAULT_PRESENTATION_TIME_DELAY_NS);
          
          (void) xc_abi_inuint(c);

          

          mrp_mad_new(source->srp_attr);         
          mrp_mad_join(source->srp_attr);         

          media_clock_register(media_clock_svr, clk_ctl, source->sync);

#if defined(AVB_TRANSMIT_BEFORE_RESERVATION)
          {chanend c = source->talker_ctl;
            xc_abi_outuint(c, AVB1722_TALKER_GO);
            xc_abi_outuint(c, source->local_id);
            (void) xc_abi_inuint(c); //ACK        
          }
#endif

        }
      }
      else if (source->state == AVB_SOURCE_STATE_POTENTIAL &&
               *state == AVB_SOURCE_STATE_ENABLED) {
        // start transmitting

        simple_printf("Enabling stream %d\n", source_num);
#if 1
        chanend c = source->talker_ctl;
        xc_abi_outuint(c, AVB1722_TALKER_GO);
        xc_abi_outuint(c, source->local_id);
        (void) xc_abi_inuint(c); //ACK        
#endif
      }
      else if (source->state != AVB_SOURCE_STATE_DISABLED &&
               *state == AVB_SOURCE_STATE_DISABLED) {
        // disabled the source
          for (int i=0;i<source->num_channels;i++) {
            inputs[source->map[i]].mapped_to = UNMAPPED;
          }
      }
      avb_set_talker_bandwidth();
      source->state = *state;      
    }
    *state = source->state;
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
       (sources[source_num].state == AVB_SOURCE_STATE_DISABLED &&
        *map_len <= AVB_MAX_CHANNELS_PER_STREAM))) {
    avb_source_info_t *source = &sources[source_num];
    if (set) {      
      memcpy(source->map, map, *map_len<<2);        
    }      
    else {
      *map_len = source[source_num].num_channels;
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
       (sources[source_num].state == AVB_SOURCE_STATE_DISABLED &&
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

    a1[0] = 
      (mac_addr[0] << 24) |
      (mac_addr[1] << 16) |
      (mac_addr[2] <<  8) |
      (mac_addr[3] <<  0);

    a1[1] = 
      (mac_addr[4] << 24) |
      (mac_addr[5] << 16) |
      ((source->local_id & 0xffff)<<0);
      
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
      (!set || sinks[sink_num].state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {      
      memcpy(sink->streamId, a1, 8);        
    }      
    memcpy(a1, sink->streamId, 8);
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
      (!set || sinks[sink_num].state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num]; 
    if (set) {
      sink->num_channels = *channels;
    }
    *channels = sink->num_channels;
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
      (!set || sinks[sink_num].state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num]; 
    if (set) {
      sink->sync = *sync;
    }
    *sync = sink->sync;
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
      (!set || sinks[sink_num].state == AVB_SINK_STATE_DISABLED)) {
    avb_sink_info_t *sink = &sinks[sink_num]; 
    if (set) {
      sink->vlan = *vlan;
    }
    *vlan = sink->vlan;
    return 1;
  }
  else 
    return 0;
}


int getset_avb_sink_addr(int set, int sink_num, unsigned char addr[], int *addr_len) 
{
  if (sink_num < AVB_NUM_SINKS &&
      (!set || 
       (sinks[sink_num].state == AVB_SINK_STATE_DISABLED &&
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
      if (sink->state == AVB_SINK_STATE_DISABLED &&
          *state == AVB_SINK_STATE_POTENTIAL) {
        chanend c = sink->listener_ctl;
        int clk_ctl;
        xc_abi_outuint(c, AVB1722_CONFIGURE_LISTENER_STREAM);
        xc_abi_outuint(c, sink->local_id);
        xc_abi_outuint(c, sink->sync);
        xc_abi_outuint(c, sink->num_channels);
        for (int i=0;i<sink->num_channels;i++) {
          //simple_printf("m:%d, %x\n",sink->map[i],outputs[sink->map[i]].fifo);
          xc_abi_outuint(c, outputs[sink->map[i]].fifo);
        }                       
        (void) xc_abi_inuint(c);

        clk_ctl = outputs[sink->map[0]].clk_ctl;
        media_clock_register(media_clock_svr, clk_ctl, sink->sync);

        { int router_link;
         
          xc_abi_outuint(c, AVB1722_GET_ROUTER_LINK);
          router_link = xc_abi_inuint(c);
          
          avb_1722_add_stream_mapping(c_mac_tx,
                                      sink->streamId,
                                      router_link,
                                      sink->local_id); 
        }

#ifndef AVB_NO_MVRP
        if (sink->vlan)
          avb_join_vlan(sink->vlan);
#endif

#ifndef AVB_NO_MMRP
        if (sink->addr[0] & 1) 
          avb_join_multicast_group(sink->addr);
#endif

        mrp_mad_new(sink->srp_attr);         
        mrp_mad_join(sink->srp_attr);         
      }
      sink->state = *state;
    }
    *state = sink->state;
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
      (!set || (sinks[sink_num].state == AVB_SINK_STATE_DISABLED
                && *map_len <= AVB_MAX_CHANNELS_PER_STREAM))) {
    avb_sink_info_t *sink = &sinks[sink_num];
    if (set) {      
      memcpy(sink->map, map, *map_len<<2);        
    }      
    else
      *map_len = sinks->num_channels;
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


int getset_device_media_clock_rate(int set, int media_clock_num, int *a2)
{
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS) {
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
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS) {
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
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS) {
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
  if (media_clock_num < AVB_NUM_MEDIA_CLOCKS) {
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

avb_status_t avb_process_control_packet(unsigned int buf[], 
                                        int nbytes,
                                        chanend c_tx)
{
  avb_status_t status;
  
  status = avb_mrp_process_packet(buf, nbytes);
  if (status != AVB_SRP_OK && status != AVB_NO_STATUS)
    return status;

  status = avb_1722_maap_process_packet_(buf, nbytes, c_tx);

  return status;
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
