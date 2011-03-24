#include <xs1.h>
#include <print.h>
#include "media_clock_client.h"
#include "media_clock_server.h"

void attach_to_media_clock(chanend c, int id) {
  c <: CLK_CTL_ATTACH_TO_CLOCK;
  c <: id;
  return;
}


void notify_buf_ctl_of_info(chanend buf_ctl, int fifo)
{
  outuchar(buf_ctl, BUF_CTL_GOT_INFO); 
  outuchar(buf_ctl,fifo>>8);
  outuchar(buf_ctl,fifo&0xff);  
  outct(buf_ctl, XS1_CT_END);            
  // (void) inuint(buf_ctl);
  //  outuint(buf_ctl, stream_num);
  //  (void) inct(buf_ctl);
}

void notify_buf_ctl_of_new_stream(chanend buf_ctl,
                                  int fifo)
{
  outuchar(buf_ctl, BUF_CTL_NEW_STREAM);
  outuchar(buf_ctl,fifo>>8);
  outuchar(buf_ctl,fifo&0xff);  
  outct(buf_ctl, XS1_CT_END);            
  //  (void) inuint(buf_ctl);
  //  outuint(buf_ctl, fifo);
  //  (void) inct(buf_ctl);
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
                       unsigned int sample_count, 
                       unsigned int ptp_ts, 
                       unsigned int local_ts,
                       unsigned int rdptr,
                       unsigned int wrptr) {
  timer tmr;
  int now;
  slave {
    buf_ctl :>  int;
    tmr :> now;
    buf_ctl <: now;
    buf_ctl <: active;
    buf_ctl <: ptp_ts;
    buf_ctl <: local_ts;
    buf_ctl <: sample_count;
    buf_ctl <: rdptr;
    buf_ctl <: wrptr;
  }
}

void send_buf_ctl_new_stream_info(chanend buf_ctl,
                                  int media_clock)
{
  slave {
    buf_ctl <: media_clock;
  }
}


void configure_ptp_derived_clock(chanend clk_svr,
                                 int clock_num,
                                 int rate)
{
  clk_svr <: CLK_CTL_CONFIGURE_CLOCK;
  master {
    clk_svr <: clock_num;
    clk_svr <: PTP_DERIVED;
    clk_svr <: 0;
    clk_svr <: 0;
    clk_svr <: rate;
  }    
}

void configure_local_stream_derived_clock(chanend clk_svr,
                                          int clock_num,
                                          int buf_ctl_index,
                                          int stream_num,
                                          int rate)
{
  clk_svr <: CLK_CTL_CONFIGURE_CLOCK;
  master {
    clk_svr <: clock_num;
    clk_svr <: MEDIA_FIFO_DERIVED;
    clk_svr <: buf_ctl_index;
    clk_svr <: stream_num;
    clk_svr <: rate;
  }    
  return;
}

