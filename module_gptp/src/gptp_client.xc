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
  signed thiscore_now,othercore_now;
  unsigned server_tile_id;
  slave {
    tmr :> thiscore_now;
    c :> othercore_now;
    c :> info.local_ts;
    c :> info.ptp_ts;
    c :> info.ptp_adjust;
    c :> info.inv_ptp_adjust;
    c :> server_tile_id;
  }
  if (server_tile_id != get_local_tile_id())
  {
    info.local_ts = info.local_ts - (othercore_now-thiscore_now);
  }
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


void ptp_get_requested_time_info_mod64_use_timer(chanend c,
                                                 ptp_time_info_mod64 &info,
                                                 timer tmr)
{
  signed thiscore_now,othercore_now;
  unsigned server_tile_id;
  slave {
    c <: 0;
    tmr :> thiscore_now;
    c :> othercore_now;
    c :> info.local_ts;
    c :> info.ptp_ts_hi;
    c :> info.ptp_ts_lo;
    c :> info.ptp_adjust;
    c :> info.inv_ptp_adjust;
    c :> server_tile_id;
  }
  if (server_tile_id != get_local_tile_id())
  {
    // 3 = protocol instruction cycle difference
    info.local_ts = info.local_ts - (othercore_now-thiscore_now-3);
  }
}


void ptp_get_requested_time_info_mod64(chanend c,
                                       ptp_time_info_mod64 &info)
{
  timer tmr;
  ptp_get_requested_time_info_mod64_use_timer(c, info, tmr);
}


void ptp_get_local_time_info_mod64(ptp_time_info_mod64 &info);

void ptp_get_time_info_mod64(chanend ?c,
                             ptp_time_info_mod64  &info)
{
  ptp_request_time_info_mod64(c);
  ptp_get_requested_time_info_mod64(c, info);
}

void ptp_get_current_grandmaster(chanend ptp_server, unsigned char grandmaster[8])
{
  send_cmd(ptp_server, PTP_GET_GRANDMASTER);
  slave
  {
    for(int i = 0; i < 8; i++)
    {
      ptp_server :> grandmaster[i];
    }
  }
}

ptp_port_role_t ptp_get_state(chanend ptp_server)
{
  ptp_port_role_t state;
  send_cmd(ptp_server, PTP_GET_STATE);
  slave
  {
    ptp_server :> state;
  }

  return state;
}


void ptp_get_propagation_delay(chanend ptp_server, unsigned *pdelay)
{
  send_cmd(ptp_server, PTP_GET_PDELAY);
  slave
  {
    ptp_server :> *pdelay;
  }
}
