#include <xs1.h>
#include <print.h>
#include "media_clock_client.h"
#include "media_clock_server.h"

void notify_buf_ctl_of_info(chanend buf_ctl, int fifo)
{
  outuchar(buf_ctl, BUF_CTL_GOT_INFO);
  outuchar(buf_ctl,fifo>>8);
  outuchar(buf_ctl,fifo&0xff);
  outct(buf_ctl, XS1_CT_END);
}

void notify_buf_ctl_of_new_stream(chanend buf_ctl,
                                  int fifo)
{
  outuchar(buf_ctl, BUF_CTL_NEW_STREAM);
  outuchar(buf_ctl,fifo>>8);
  outuchar(buf_ctl,fifo&0xff);
  outct(buf_ctl, XS1_CT_END);
}

void buf_ctl_ack(chanend buf_ctl)
{
  outct(buf_ctl, XS1_CT_END);
}

int get_buf_ctl_adjust(chanend buf_ctl) {
  int adjust;
  buf_ctl :> adjust;
  return adjust;
}

int get_buf_ctl_cmd(chanend buf_ctl) {
  int cmd;
  buf_ctl :> cmd;
  return cmd;
}

void send_buf_ctl_info(chanend buf_ctl,
                       int active,
                       unsigned int ptp_ts,
                       unsigned int local_ts,
                       unsigned int rdptr,
                       unsigned int wrptr,
                       timer tmr) {
  int thiscore_now;
  int tile_id = get_local_tile_id();
  slave {
    buf_ctl :>  int;
    tmr :> thiscore_now;
    buf_ctl <: thiscore_now;
    buf_ctl <: active;
    buf_ctl <: ptp_ts;
    buf_ctl <: local_ts;
    buf_ctl <: rdptr;
    buf_ctl <: wrptr;
    buf_ctl <: tile_id;
  }
}

void send_buf_ctl_new_stream_info(chanend buf_ctl,
                                  int media_clock)
{
  slave {
    buf_ctl <: media_clock;
  }
}

