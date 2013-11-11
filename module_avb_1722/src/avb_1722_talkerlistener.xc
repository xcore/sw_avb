#include <xs1.h>
#include "avb_1722_def.h"
#include "avb_conf.h"
#include "gptp.h"
#include "media_input_fifo.h"
#include "avb_1722.h"
#include "avb_1722_listener.h"
#include "avb_1722_talker.h"
#include "media_fifo.h"
#include "ethernet_server_def.h"
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"

#if AVB_NUM_SOURCES != 0


void avb_1722_talker_init(chanend c_talker_ctl,
                          chanend c_mac_tx,
                          avb_1722_talker_state_t &st,
                          int num_streams);

#pragma select handler
void avb_1722_talker_handle_cmd(chanend c_talker_ctl,
                                avb_1722_talker_state_t &st);

void avb_1722_talker_send_packets(chanend c_mac_tx,
                                  avb_1722_talker_state_t &st,
                                  ptp_time_info_mod64 &timeInfo,
                                  timer tmr);

void avb_1722_listener_init(chanend c_mac_rx,
                            chanend c_listener_ctl,
                            avb_1722_listener_state_t &st,
                            int num_streams);

#pragma select handler
void avb_1722_listener_handle_packet(chanend c_mac_rx,
                                     chanend c_buf_ctl,
                                     avb_1722_listener_state_t &st,
                                     ptp_time_info_mod64 &?timeInfo);

#pragma select handler
void avb_1722_listener_handle_cmd(chanend c_listener_ctl,
                                  avb_1722_listener_state_t &st);


#define TIMEINFO_UPDATE_INTERVAL 50000000

#pragma unsafe arrays
void avb_1722_talkerlistener(chanend c_ptp,
                             chanend c_mac_rx,
                             chanend c_mac_tx,
                             chanend c_listener_ctl,
                             chanend c_talker_ctl,
                             chanend c_buf_ctl,
                             int num_listener_streams,
                             int num_talker_streams)
{
  avb_1722_talker_state_t talker_state;
  avb_1722_listener_state_t listener_state;
  ptp_time_info_mod64 timeInfo;
  timer tmr;
  unsigned t;
  int pending_timeinfo = 0;

  set_thread_fast_mode_on();
  avb_1722_talker_init(c_talker_ctl, c_mac_tx,
                       talker_state, num_talker_streams);
  avb_1722_listener_init(c_mac_rx, c_listener_ctl,
                         listener_state, num_listener_streams);

  ptp_request_time_info_mod64(c_ptp);
  ptp_get_requested_time_info_mod64_use_timer(c_ptp, timeInfo, tmr);

  tmr	:> t;
  t+=TIMEINFO_UPDATE_INTERVAL;

  // main loop.
  while (1)
    {
      #pragma ordered
      select
        {
        case c_buf_ctl :> int stream_num:
          media_output_fifo_handle_buf_ctl(c_buf_ctl,  stream_num,
                                           listener_state.notified_buf_ctl,
                                           tmr);
          break;

          // The PTP server has sent new time information
        case ptp_get_requested_time_info_mod64_use_timer(                                                   c_ptp, timeInfo, tmr):
          pending_timeinfo = 0;
          break;

        case avb_1722_listener_handle_packet(c_mac_rx,
                                             c_buf_ctl,
                                             listener_state,
                                             timeInfo):
          break;


          // Periodically ask the PTP server for new time information
        case tmr when timerafter(t) :> t:
          if (!pending_timeinfo) {
            ptp_request_time_info_mod64(c_ptp);
            pending_timeinfo = 1;
          }
          t+=TIMEINFO_UPDATE_INTERVAL;
          break;


          // Process commands from the AVB control/application thread
        case avb_1722_talker_handle_cmd(c_talker_ctl, talker_state): break;
        case avb_1722_listener_handle_cmd(c_listener_ctl, listener_state):  break;

          // Call the 1722 packet construction
        default:
          avb_1722_talker_send_packets(c_mac_tx, talker_state, timeInfo, tmr);
          break;
        }
    }
}


#endif // AVB_NUM_SOURCES != 0
