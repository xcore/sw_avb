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
#include "mac_custom_filter.h"
#include "avb_conf.h"

#define TIMEINFO_UPDATE_INTERVAL 50000000

// Max. packet size for AVB AVB1722 listener
#ifdef AVB_1722_FORMAT_SAF
#define MAX_PKT_BUF_SIZE_LISTENER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT * AVB_MAX_CHANNELS_PER_STREAM * 4 + 4)
#endif

#ifdef AVB_1722_FORMAT_61883_4
#define MAX_PKT_BUF_SIZE_LISTENER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + 192*MAX_TS_PACKETS_PER_1722 + 4)
#endif

#ifdef AVB_1722_FORMAT_61883_6
#define MAX_PKT_BUF_SIZE_LISTENER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT * AVB_MAX_CHANNELS_PER_STREAM * 4 + 4)
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
			enable_media_output_fifo(s.map[i], media_clock);
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
			disable_media_output_fifo(s.map[i]);
	}

	s.active = 0;
	s.state = 0;
}

#pragma unsafe arrays
void avb_1722_listener(chanend ethernet_rx_svr,
                       chanend? buf_ctl,
                       chanend? ptp_ctl,
                       chanend listener_ctl,
                       int num_streams)
{
	int listenerEnabled;
	unsigned pktByteCnt;
	unsigned cmd;
	unsigned int avb_hash;
	unsigned int RxBuf[(MAX_PKT_BUF_SIZE_LISTENER+3)/4];
	avb_1722_stream_info_t listener_streams[MAX_AVB_STREAMS_PER_LISTENER];
	ptp_time_info_mod64 timeInfo;
	int router_link = 0;
	int notified_buf_ctl = 0;
	unsigned int src_port;
	int valid_timeinfo = 1;

#if defined(AVB_1722_FORMAT_61883_4)
	// Conditional due to compiler bug 11998.
	timer tmr;
	unsigned t;
	int pending_timeinfo = 0;
	valid_timeinfo = 0;
#endif

	set_thread_fast_mode_on();

	// register how many streams this listener unit has
	router_link = avb_register_listener_streams(listener_ctl, num_streams);

	for (int i=0;i<MAX_AVB_STREAMS_PER_LISTENER;i++) {
		listener_streams[i].active = 0;
		listener_streams[i].state = 0;
	}

	// initialisation
	listenerEnabled = 0;

	mac_set_queue_size(ethernet_rx_svr, num_streams+2);
	mac_set_custom_filter(ethernet_rx_svr, ROUTER_LINK(router_link));

#if defined(AVB_1722_FORMAT_61883_4)
	// Conditional due to compiler bug 11998.
	tmr	:> t;
	t+=TIMEINFO_UPDATE_INTERVAL;
#endif

	// main loop.
	while (1) {

#pragma ordered
		select {
		case mac_rx_offset2(ethernet_rx_svr, (RxBuf, unsigned char[]), pktByteCnt, src_port): {
			pktByteCnt -= 4;
			avb_hash = RxBuf[1];

			// process the audio packet if enabled.
			if (avb_hash < MAX_AVB_STREAMS_PER_LISTENER && listener_streams[avb_hash].active && valid_timeinfo)
            {
				// process the current packet
				avb_1722_listener_process_packet(buf_ctl,
					(RxBuf, unsigned char[]),
					pktByteCnt,
					listener_streams[avb_hash],
					timeInfo,
					avb_hash,
					notified_buf_ctl);
            }
          break;
        }

#if !defined(AVB_1722_FORMAT_61883_4)
		// Conditional due to compiler bug 11998.
		case !isnull(buf_ctl) => buf_ctl :> int stream_num:
			media_output_fifo_handle_buf_ctl(buf_ctl,  stream_num, notified_buf_ctl);
			break;
#endif

#if defined(AVB_1722_FORMAT_61883_4)
		// Conditional due to compiler bug 11998

		// Periodically ask the PTP server for new time information
		case !isnull(ptp_ctl) => tmr when timerafter(t) :> t:
			if (!pending_timeinfo) {
				ptp_request_time_info_mod64(ptp_ctl);
				pending_timeinfo = 1;
			}
			t+=TIMEINFO_UPDATE_INTERVAL;
			break;

		// The PTP server has sent new time information
		case !isnull(ptp_ctl) => ptp_get_requested_time_info_mod64(ptp_ctl, timeInfo):
			pending_timeinfo = 0;
			valid_timeinfo = 1;
			break;
#endif

		case listener_ctl :> cmd: {
			// perform the command.
			switch (cmd)
			{
			case AVB1722_CONFIGURE_LISTENER_STREAM:
				{
				int stream_num;
				listener_ctl :> stream_num;
				configure_stream(listener_ctl,
				listener_streams[stream_num]);
				listener_ctl <: AVB1722_ACK;
				break;
				}
			case AVB1722_ADJUST_LISTENER_STREAM:
				{
				int stream_num;
				listener_ctl :> stream_num;
				adjust_stream(listener_ctl,
				listener_streams[stream_num]);
				listener_ctl <: AVB1722_ACK;
				break;
				}
			case AVB1722_DISABLE_LISTENER_STREAM:
				{
				int stream_num;
				listener_ctl :> stream_num;
				disable_stream(listener_streams[stream_num]);
				listener_ctl <: AVB1722_ACK;
				break;
				}
			case AVB1722_GET_ROUTER_LINK:
				listener_ctl <: router_link;
				break;
			default:
				// sent NACK out
				listener_ctl <: AVB1722_NACK;
				break;
			}
		}

		break;
		}
	}
}

