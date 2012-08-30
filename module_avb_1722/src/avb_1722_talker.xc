/** \file avb_1722_talker.xc
 *  \brief IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 */

#include <platform.h>
#include <xs1.h>
#include <xclib.h>

#include "avb_1722_def.h"
#include "avb_1722.h"
#include "avb_1722_listener.h"
#include "avb_1722_talker.h"
#include "media_fifo.h"
#include "ethernet_server_def.h"
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"
#include "avb_srp.h"
#include "avb_unit.h"
#include "avb_conf.h"

#if AVB_NUM_SOURCES != 0

// Max. packet size for AVB 1722 talker
#ifdef AVB_1722_FORMAT_SAF
#define MAX_PKT_BUF_SIZE_TALKER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT * AVB_MAX_CHANNELS_PER_STREAM * 4 + 4)
#endif

#ifdef AVB_1722_FORMAT_61883_4
#define MAX_PKT_BUF_SIZE_TALKER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + 192*MAX_TS_PACKETS_PER_1722 + 4)
#endif

#ifdef AVB_1722_FORMAT_61883_6
#define MAX_PKT_BUF_SIZE_TALKER (AVB_ETHERNET_HDR_SIZE + AVB_TP_HDR_SIZE + AVB_CIP_HDR_SIZE + TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT * AVB_MAX_CHANNELS_PER_STREAM * 4 + 4)
#endif

static avb1722_Talker_StreamConfig_t talker_streams[AVB_MAX_STREAMS_PER_TALKER_UNIT];


static void configure_stream(chanend avb1722_tx_config,
							 avb1722_Talker_StreamConfig_t &stream,
							 unsigned char mac_addr[]) {
	unsigned int streamIdExt;
	unsigned int rate;
	unsigned int tmp;
	int samplesPerPacket;
	avb1722_tx_config :> stream.sampleType;

	for (int i = 0; i < MAC_ADRS_BYTE_COUNT; i++) {
		int x;
		avb1722_tx_config :> x;
		stream.destMACAdrs[i] = x;
		stream.srcMACAdrs[i] = mac_addr[i];
	}

	stream.streamId[1] =
	((unsigned) stream.srcMACAdrs[0] << 24) |
	((unsigned) stream.srcMACAdrs[1] << 16) |
	((unsigned) stream.srcMACAdrs[2] << 8) |
	((unsigned) stream.srcMACAdrs[3]);

	stream.streamId[0] =
	((unsigned) stream.srcMACAdrs[4] << 24) |
	((unsigned) stream.srcMACAdrs[5] << 16);

	avb1722_tx_config :> streamIdExt;

	stream.streamId[0] |= streamIdExt;

	avb1722_tx_config :> stream.num_channels;

	avb1722_tx_config :> stream.fifo_mask;

	for (int i=0;i<stream.num_channels;i++) {
		avb1722_tx_config :> stream.map[i];
	}

	avb1722_tx_config :> rate;

	for (int i=0;i<stream.num_channels;i++) {
		samplesPerPacket = media_input_fifo_enable(stream.map[i], rate);
	}

	avb1722_tx_config :> stream.presentation_delay;

	stream.samples_per_fifo_packet = samplesPerPacket;

	tmp = ((rate / 100) << 16) / (AVB1722_PACKET_RATE / 100);
	stream.samples_per_packet_base = tmp >> 16;
	stream.samples_per_packet_fractional = tmp & 0xffff;
	stream.rem = 0;

	stream.samples_left_in_fifo_packet = 0;
	stream.initial = 1;
	stream.dbc_at_start_of_last_fifo_packet = 0;
	stream.active = 1;
	stream.transmit_ok = 1;
	stream.sequence_number = 0;
}

static void disable_stream(avb1722_Talker_StreamConfig_t &stream) {

	stream.streamId[1] = 0;
	stream.streamId[0] = 0;

	media_input_fifo_disable_fifos(stream.fifo_mask);

	for (int i=0;i<stream.num_channels;i++) {
		media_input_fifo_disable(stream.map[i]);
	}

	stream.active = 0;
}

static void start_stream(avb1722_Talker_StreamConfig_t &stream) {
	media_input_fifo_enable_fifos(stream.fifo_mask);
	stream.samples_left_in_fifo_packet = 0;
	stream.sequence_number = 0;
	stream.initial = 1;
	stream.active = 2;
}

static void stop_stream(avb1722_Talker_StreamConfig_t &stream) {
	media_input_fifo_disable_fifos(stream.fifo_mask);
	stream.active = 1;
}

#define TIMEINFO_UPDATE_INTERVAL 50000000
/** This packetizes Audio samples into an AVB payload and transmit it across
 *  Ethernet.
 *
 *  1. Get audio samples from ADC fifo.
 *  2. Convert the local timer value to global PTP timestamp.
 *  3. AVB payload generation and transmit to Ethernet.
 */
#pragma unsafe arrays
void avb_1722_talker(chanend ptp_svr, chanend ethernet_tx_svr,
		chanend talker_ctl, int num_streams) {
	ptp_time_info_mod64 timeInfo;
	unsigned int TxBuf[(MAX_PKT_BUF_SIZE_TALKER + 3) / 4];
	int max_active_avb_stream = 0;
	int cur_avb_stream = 0;
	unsigned char mac_addr[6];
	timer tmr;
	unsigned t;
	int pending_timeinfo = 0;
	int vlan = 2;
	set_thread_fast_mode_on();

	for (unsigned n=0; n<(MAX_PKT_BUF_SIZE_TALKER + 3) / 4; ++n) TxBuf[n] = 0;

	// register how many streams this talker unit has
	avb_register_talker_streams(talker_ctl, num_streams);

	// Initialise local data structure.
	ethernet_get_my_mac_adrs(ethernet_tx_svr, mac_addr);

	for (int i = 0; i < AVB_MAX_STREAMS_PER_TALKER_UNIT; i++)
		talker_streams[i].active = 0;

	tmr	:> t;
	t+=TIMEINFO_UPDATE_INTERVAL;

	// main loop.
	while (1)
	{
		select
		{
			// Process commands from the AVB control/application thread
			case talker_ctl :> int cmd:
			switch (cmd)
			{
				case AVB1722_CONFIGURE_TALKER_STREAM:
				{
					int stream_num;
					talker_ctl :> stream_num;
					configure_stream(talker_ctl,
							talker_streams[stream_num],
							mac_addr);
					if (stream_num > max_active_avb_stream)
					max_active_avb_stream = stream_num;

					// Eventually this will have to be changed
					// to create per-stream headers
					// for now assume sampling rate etc. the same
					// on all streams
					AVB1722_Talker_bufInit((TxBuf,unsigned char[]),
							talker_streams[stream_num],
							vlan);

					talker_ctl <: AVB1722_ACK;
				}
				break;
				case AVB1722_DISABLE_TALKER_STREAM:
				{
					int stream_num;
					talker_ctl :> stream_num;
					disable_stream(talker_streams[stream_num]);
					talker_ctl <: AVB1722_ACK;
				}
				break;
				case AVB1722_TALKER_GO:
				{
					int stream_num;
					talker_ctl :> stream_num;
					start_stream(talker_streams[stream_num]);
					talker_ctl <: AVB1722_ACK;
				}
				break;
				case AVB1722_TALKER_STOP:
				{
					int stream_num;
					talker_ctl :> stream_num;
					stop_stream(talker_streams[stream_num]);
					talker_ctl <: AVB1722_ACK;
				}
				break;

				case AVB1722_SET_VLAN:
				talker_ctl :> vlan;
				avb1722_set_buffer_vlan(vlan,(TxBuf,unsigned char[]));
				break;
				default:
				talker_ctl <: AVB1722_NACK;
				break;
			}
			break;

			// Periodically ask the PTP server for new time information
			case tmr when timerafter(t) :> t:
				if (!pending_timeinfo) {
					ptp_request_time_info_mod64(ptp_svr);
					pending_timeinfo = 1;
				}
				t+=TIMEINFO_UPDATE_INTERVAL;
				break;

			// The PTP server has sent new time information
			case ptp_get_requested_time_info_mod64(ptp_svr, timeInfo):
				pending_timeinfo = 0;
				break;

			// Call the 1722 packet construction
			default:
			if (max_active_avb_stream != -1 &&
					talker_streams[cur_avb_stream].active==2) {
				int packet_size;
				int t;
				tmr :> t;

				packet_size =
				avb1722_create_packet((TxBuf, unsigned char[]),
						talker_streams[cur_avb_stream],
						timeInfo,
						t);

				if (packet_size) {
					if (packet_size < 60) packet_size = 60;
					ethernet_send_frame_offset2(ethernet_tx_svr,
							TxBuf,
							packet_size,
							ETH_BROADCAST);
				}
			}
			if (max_active_avb_stream != -1) {
				cur_avb_stream++;
				if (cur_avb_stream>max_active_avb_stream)
				cur_avb_stream=0;
			}

			break;
		}
	}
}

#endif // AVB_NUM_SOURCES != 0
