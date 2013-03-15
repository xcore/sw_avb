/**
 * \file avb_1722_listener.xc
 * \brief AVB1722 Listener
 */

#include <platform.h>
#include <xs1.h>
#include <xclib.h>

#include "avb_1722_def.h"
#include "avb_1722.h"
#include "avb_1722_listener.h"
#include "media_fifo.h"
#include "ethernet_server_def.h"
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"

#include "avb_srp.h"
#include "avb_unit.h"
#include "mac_filter.h"
#include "avb_conf.h"

#define TIMEINFO_UPDATE_INTERVAL 50000000

// Max. packet size for AVB AVB1722 listener
#ifdef AVB_1722_FORMAT_SAF
#define MAX_PKT_BUF_SIZE_LISTENER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT * AVB_MAX_CHANNELS_PER_LISTENER_STREAM * 4 + 4)
#endif

#ifdef AVB_1722_FORMAT_61883_4
#define MAX_PKT_BUF_SIZE_LISTENER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + 192*MAX_TS_PACKETS_PER_1722 + 4)
#endif

#ifdef AVB_1722_FORMAT_61883_6
#define MAX_PKT_BUF_SIZE_LISTENER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT * AVB_MAX_CHANNELS_PER_LISTENER_STREAM * 4 + 4)
#endif


static void configure_stream(chanend c,
                             avb_1722_stream_info_t &s)
{
	int media_clock;

	c :> media_clock;
	c :> s.rate;
	c :> s.num_channels;
    
	for(int i=0;i<s.num_channels;i++) {
		c :> s.map[i];
		if (s.map[i])
		{
			enable_media_output_fifo(s.map[i], media_clock);
		}
	}

	s.active = 1;
	s.state = 0;
	s.num_channels_in_payload = 0;
	s.chan_lock = 0;
	s.prev_num_samples = 0;
	s.dbc = -1;
}

static void adjust_stream(chanend c,
        avb_1722_stream_info_t &s)
{
	int cmd;
	c :> cmd;
	switch (cmd) {
	case AVB1722_ADJUST_LISTENER_VOLUME:
		{
#ifdef MEDIA_OUTPUT_FIFO_VOLUME_CONTROL
			int volume, count;
			c :> count;
			for(int i=0;i<count;i++) {
				c :> volume;
				if (i < s.num_channels) media_output_fifo_set_volume(s.map[i], volume);
			}
#endif
		}
		break;
	}
}


static void disable_stream(avb_1722_stream_info_t &s)
{
	for(int i=0;i<s.num_channels;i++) {
		if (s.map[i])
		{
			disable_media_output_fifo(s.map[i]);
		}
	}

	s.active = 0;
	s.state = 0;
}

void avb_1722_listener_init(chanend c_mac_rx,
                            chanend c_listener_ctl,
                            avb_1722_listener_state_t &st,
                            int num_streams)
{
  // register how many streams this listener unit has
  st.router_link = avb_register_listener_streams(c_listener_ctl, num_streams);

  st.notified_buf_ctl = 0;

  for (int i=0;i<MAX_AVB_STREAMS_PER_LISTENER;i++) {
    st.listener_streams[i].active = 0;
    st.listener_streams[i].state = 0;
  }

  // initialisation
  mac_set_queue_size(c_mac_rx, num_streams+2);
  mac_set_custom_filter(c_mac_rx, ROUTER_LINK(st.router_link));
}



#pragma select handler
void avb_1722_listener_handle_packet(chanend c_mac_rx,
                                     chanend c_buf_ctl,
                                     avb_1722_listener_state_t &st,
                                     ptp_time_info_mod64 &?timeInfo)
{
  unsigned pktByteCnt;
  unsigned int RxBuf[(MAX_PKT_BUF_SIZE_LISTENER+3)/4];
  unsigned int src_port;
  int stream_id;

  mac_rx_offset2(c_mac_rx,
                 (RxBuf, unsigned char[]),
                 pktByteCnt,
                 stream_id,
                 src_port);
  pktByteCnt -= 4;

  // process the audio packet if enabled.
  if (stream_id < MAX_AVB_STREAMS_PER_LISTENER &&
      st.listener_streams[stream_id].active) {
    // process the current packet
    avb_1722_listener_process_packet(c_buf_ctl,
                                     (RxBuf, unsigned char[]),
                                     pktByteCnt,
                                     st.listener_streams[stream_id],
                                     timeInfo,
                                     stream_id,
                                     st.notified_buf_ctl);
  }
}


#pragma select handler
void avb_1722_listener_handle_cmd(chanend c_listener_ctl,
                                  avb_1722_listener_state_t &st)
{
  int cmd;
  c_listener_ctl :> cmd;
  // perform the command.
  switch (cmd)
    {
    case AVB1722_CONFIGURE_LISTENER_STREAM:
      {
        int stream_num;
        c_listener_ctl :> stream_num;
        configure_stream(c_listener_ctl,
                         st.listener_streams[stream_num]);
        c_listener_ctl <: AVB1722_ACK;
        break;
      }
    case AVB1722_ADJUST_LISTENER_STREAM:
      {
        int stream_num;
        c_listener_ctl :> stream_num;
        adjust_stream(c_listener_ctl,
                      st.listener_streams[stream_num]);
        c_listener_ctl <: AVB1722_ACK;
        break;
      }
    case AVB1722_DISABLE_LISTENER_STREAM:
      {
        int stream_num;
        c_listener_ctl :> stream_num;
        disable_stream(st.listener_streams[stream_num]);
        c_listener_ctl <: AVB1722_ACK;
        break;
      }
    case AVB1722_GET_ROUTER_LINK:
      c_listener_ctl <: st.router_link;
      break;
    default:
      // sent NACK out
      c_listener_ctl <: AVB1722_NACK;
      break;
    }
}


#pragma unsafe arrays
void avb_1722_listener(chanend c_mac_rx,
                       chanend? c_buf_ctl,
                       chanend? c_ptp,
                       chanend c_listener_ctl,
                       int num_streams)
{
  avb_1722_listener_state_t st;
  timer tmr;

#if defined(AVB_1722_FORMAT_61883_4)
  // Conditional due to compiler bug 11998.
  unsigned t;
  int pending_timeinfo = 0;
  ptp_time_info_mod64 timeInfo;
#endif
  set_thread_fast_mode_on();
  avb_1722_listener_init(c_mac_rx, c_listener_ctl, st, num_streams);

#if defined(AVB_1722_FORMAT_61883_4)
  // Conditional due to compiler bug 11998.
  ptp_request_time_info_mod64(c_ptp);
  ptp_get_requested_time_info_mod64(c_ptp, timeinfo);
  tmr	:> t;
  t+=TIMEINFO_UPDATE_INTERVAL;
#endif

  // main loop.
  while (1) {

#pragma ordered
    select
      {
#if !defined(AVB_1722_FORMAT_61883_4)
        // Conditional due to compiler bug 11998.
      case !isnull(c_buf_ctl) => c_buf_ctl :> int stream_num:
          media_output_fifo_handle_buf_ctl(c_buf_ctl,  stream_num, st.notified_buf_ctl, tmr);
        break;
#endif

#if defined(AVB_1722_FORMAT_61883_4)
        // The PTP server has sent new time information
      case !isnull(c_ptp) => ptp_get_requested_time_info_mod64(c_ptp, timeInfo):
        pending_timeinfo = 0;
        break;
#endif

      case avb_1722_listener_handle_packet(c_mac_rx,
                                           c_buf_ctl,
                                           st,
                                           #ifdef AVB_1722_FORMAT_61883_4
                                           timeInfo
                                           #else
                                           null
                                           #endif
                                           ):
        break;


#if defined(AVB_1722_FORMAT_61883_4)
        // Conditional due to compiler bug 11998
        // Periodically ask the PTP server for new time information
      case !isnull(c_ptp) => tmr when timerafter(t) :> t:
        if (!pending_timeinfo) {
          ptp_request_time_info_mod64(c_ptp);
          pending_timeinfo = 1;
        }
        t+=TIMEINFO_UPDATE_INTERVAL;
        break;
#endif

      case avb_1722_listener_handle_cmd(c_listener_ctl, st):
        break;
      }
  }
}
