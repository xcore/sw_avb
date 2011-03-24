#include <xs1.h>
#include "gptp.h"
#include "gptp_cmd.h"

static void send_cmd(chanend c, char cmd)
{
  outuchar(c, cmd);
  outuchar(c, cmd);
  outuchar(c, cmd);
  outct(c, XS1_CT_END);
}



void ptp_request_time_info(chanend c)
{
  send_cmd(c, PTP_GET_TIME_INFO);
}


void ptp_get_requested_time_info(chanend c, 
                                 ptp_time_info &info)
{
  timer tmr;
  signed t1,t2;
  slave {
    tmr :> t1;
    c :> t2;    
    c :> info.local_ts;
    c :> info.ptp_ts;
    c :> info.ptp_adjust;
    c :> info.inv_ptp_adjust;
  }
  info.local_ts = info.local_ts - (t2-t1);
}


void ptp_get_time_info(chanend c, 
                       ptp_time_info  &info)
{
  ptp_request_time_info(c);
  ptp_get_requested_time_info(c, info);   
}


void ptp_request_time_info_mod64(chanend c)
{
  send_cmd(c, PTP_GET_TIME_INFO_MOD64);
}


void ptp_get_requested_time_info_mod64(chanend c, 
                                       ptp_time_info_mod64 &info)
{
  timer tmr;
  signed t1,t2;
  slave {
    c <: 0;
    c :> t2;
    tmr :> t1;
    c :> info.local_ts;
    c :> info.ptp_ts_hi;
    c :> info.ptp_ts_lo;
    c :> info.ptp_adjust;
    c :> info.inv_ptp_adjust;
  }
  info.local_ts = info.local_ts - (t2-t1);
}


void ptp_get_time_info_mod64(chanend c, 
                             ptp_time_info_mod64  &info)
{
  ptp_request_time_info_mod64(c);
  ptp_get_requested_time_info_mod64(c, info);   
}



void ptp_set_legacy_mode(chanend c, int mode)
{
  send_cmd(c, PTP_SET_LEGACY_MODE);
  c <: mode;
}
