/** \file avb_1722_talker.xc
 *  \brief IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 */

#include <platform.h>
#include <xs1.h>
#include <xclib.h>
#include <print.h>

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

#pragma unsafe arrays
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

#pragma unsafe arrays
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


void avb_1722_talker_init(chanend c_talker_ctl,
                          chanend c_mac_tx,
                          avb_1722_talker_state_t &st,
                          int num_streams)
 {
  st.vlan = 2;
  st.cur_avb_stream = 0;
  st.max_active_avb_stream = 0;

  for (unsigned n=0; n<(MAX_PKT_BUF_SIZE_TALKER + 3) / 4; ++n)
    st.TxBuf[n] = 0;

  // register how many streams this talker unit has
  avb_register_talker_streams(c_talker_ctl, num_streams);

  // Initialise local data structure.
  mac_get_macaddr(c_mac_tx, st.mac_addr);

  for (int i = 0; i < AVB_MAX_STREAMS_PER_TALKER_UNIT; i++)
    st.talker_streams[i].active = 0;
}


#pragma unsafe arrays
#pragma select handler
void avb_1722_talker_handle_cmd(chanend c_talker_ctl,
                                avb_1722_talker_state_t &st)
{
  int cmd;
  c_talker_ctl :> cmd;
  switch (cmd)
    {
    case AVB1722_CONFIGURE_TALKER_STREAM:
      {
        int stream_num;
        c_talker_ctl :> stream_num;
        configure_stream(c_talker_ctl,
                         st.talker_streams[stream_num],
                         st.mac_addr);
        if (stream_num > st.max_active_avb_stream)
          st.max_active_avb_stream = stream_num;

        // Eventually this will have to be changed
        // to create per-stream headers
        // for now assume sampling rate etc. the same
        // on all streams
        AVB1722_Talker_bufInit((st.TxBuf,unsigned char[]),
                               st.talker_streams[stream_num],
                               st.vlan);

        c_talker_ctl <: AVB1722_ACK;
      }
      break;
    case AVB1722_DISABLE_TALKER_STREAM:
      {
        int stream_num;
        c_talker_ctl :> stream_num;
        disable_stream(st.talker_streams[stream_num]);
        c_talker_ctl <: AVB1722_ACK;
      }
      break;
    case AVB1722_TALKER_GO:
      {
        int stream_num;
        c_talker_ctl :> stream_num;
        start_stream(st.talker_streams[stream_num]);
        c_talker_ctl <: AVB1722_ACK;
      }
      break;
    case AVB1722_TALKER_STOP:
      {
        int stream_num;
        c_talker_ctl :> stream_num;
        stop_stream(st.talker_streams[stream_num]);
        c_talker_ctl <: AVB1722_ACK;
      }
      break;

    case AVB1722_SET_VLAN:
      c_talker_ctl :> st.vlan;
      avb1722_set_buffer_vlan(st.vlan,(st.TxBuf,unsigned char[]));
      break;
    default:
      c_talker_ctl <: AVB1722_NACK;
      break;
    }
}


void avb_1722_talker_send_packets(chanend c_mac_tx,
                                  avb_1722_talker_state_t &st,
                                  ptp_time_info_mod64 &timeInfo,
                                  timer tmr)
{
  if (st.max_active_avb_stream != -1 &&
      st.talker_streams[st.cur_avb_stream].active==2) {
    int packet_size;
    int t;

    tmr :> t; 
    packet_size =
      avb1722_create_packet((st.TxBuf, unsigned char[]),
                            st.talker_streams[st.cur_avb_stream],
                            timeInfo,
                            t);
    if (packet_size) {
      if (packet_size < 60) packet_size = 60;
      ethernet_send_frame_offset2(c_mac_tx,
                                  st.TxBuf,
                                  packet_size,
                                  0);
      st.talker_streams[st.cur_avb_stream].last_transmit_time = t;
    }
  }
  if (st.max_active_avb_stream != -1) {
    st.cur_avb_stream++;
    if (st.cur_avb_stream>st.max_active_avb_stream)
      st.cur_avb_stream=0;
  }
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
void avb_1722_talker(chanend c_ptp, chanend c_mac_tx,
                     chanend c_talker_ctl, int num_streams) {
  avb_1722_talker_state_t st;
  ptp_time_info_mod64 timeInfo;
  timer tmr;
  unsigned t;
  int pending_timeinfo = 0;

  set_thread_fast_mode_on();
  avb_1722_talker_init(c_talker_ctl, c_mac_tx, st, num_streams);

  ptp_request_time_info_mod64(c_ptp);
  ptp_get_requested_time_info_mod64_use_timer(c_ptp, timeInfo, tmr);

  tmr :> t;
  t+=TIMEINFO_UPDATE_INTERVAL;

  // main loop.
  while (1)
  {
    select
    {
        // Process commands from the AVB control/application thread
      case avb_1722_talker_handle_cmd(c_talker_ctl, st): break;

        // Periodically ask the PTP server for new time information
      case tmr when timerafter(t) :> t:
        if (!pending_timeinfo) {
          ptp_request_time_info_mod64(c_ptp);
          pending_timeinfo = 1;
        }
        t+=TIMEINFO_UPDATE_INTERVAL;
        break;

        // The PTP server has sent new time information
      case ptp_get_requested_time_info_mod64_use_timer(c_ptp, timeInfo, tmr):
        pending_timeinfo = 0;
        break;


        // Call the 1722 packet construction
      default:
        avb_1722_talker_send_packets(c_mac_tx, st, timeInfo, tmr);
        break;
    }
  }
}

#endif // AVB_NUM_SOURCES != 0
