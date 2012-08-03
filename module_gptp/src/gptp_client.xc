#include <xs1.h>
#include "gptp.h"
#include "gptp_cmd.h"
#include "get_core_id_from_chanend.h"

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
  unsigned server_core_id;
  slave {
    tmr :> thiscore_now;
    c :> othercore_now;
    c :> info.local_ts;
    c :> info.ptp_ts;
    c :> info.ptp_adjust;
    c :> info.inv_ptp_adjust;
    c :> server_core_id;
  }
  if (server_core_id != get_core_id_from_chanend(c))
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


void ptp_get_requested_time_info_mod64(chanend c, 
                                       ptp_time_info_mod64 &info)
{
  timer tmr;
  signed thiscore_now,othercore_now;
  unsigned server_core_id;
  slave {
    c <: 0;
    tmr :> thiscore_now;
    c :> othercore_now;
    c :> info.local_ts;
    c :> info.ptp_ts_hi;
    c :> info.ptp_ts_lo;
    c :> info.ptp_adjust;
    c :> info.inv_ptp_adjust;
    c :> server_core_id;
  }
  if (server_core_id != get_core_id_from_chanend(c))
  {
	  // 3 = protocol instruction cycle difference
	  info.local_ts = info.local_ts - (othercore_now-thiscore_now-3);
  }
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
