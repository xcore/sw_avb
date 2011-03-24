#include "string.h"
#include "gptp.h"
#include "avnu_observability.h"

void ptp_give_requested_time_info(chanend c);

void avnu_update_ptp_timeinfo_xc(chanend c) {
  c <: 0;
  c <: 1;
  ptp_give_requested_time_info(c);
}

void avnu_log_xc(chanend c,
                 enum avnu_testpoint_type_t testpoint,
                 ptp_timestamp &?ts,
                 char msg[])
{
  int len = strlen(msg)+1;
  c <: 0;
  c <: 0;
  master {
    c <: testpoint;
    c <: !isnull(ts);
    if (!isnull(ts)) 
      c <: ts;
  }
  master {
    c <: len;
    for (int i=0;i<len;i++)  
      c <: msg[i];
  }
}

void avnu_get_log_hdr(chanend c,
                      enum avnu_testpoint_type_t &testpoint,
                      ptp_timestamp &ts,
                      int &valid_ts)
{
  slave {
  c :> testpoint;
  c :> valid_ts;
    if (valid_ts)
    c :> ts;
  }
}

void avnu_get_log_msg(chanend c,
                      char msg[])
{
  int len;
  slave {
    c :> len;
    for (int i=0;i<len;i++)
    c :> msg[i];
  }
}

int avnu_msg_type(chanend c) {
  int x;
  c :> x;
  return x; 
}

void avnu_receive_timeinfo_update(chanend c, ptp_time_info &timeinfo)
{
  ptp_get_requested_time_info(c, 
                              timeinfo);  
}
