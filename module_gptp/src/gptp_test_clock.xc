#include <xs1.h>
#include "gptp.h"

void ptp_output_test_clock(chanend ptp_link,
                           port test_clock_port,
                           int period)
{ int x = 0;
  timer tmr;
  int t;
  ptp_timestamp ptp_ts;
  ptp_time_info ptp_info;
  int t0;
#if 0
 tmr :> t;
  while(1) {
    tmr when timerafter(t) :> void;
    test_clock_port <: x;
    x = ~x;
    t += 1000000;
  }
#else

  ptp_get_time_info(ptp_link, ptp_info);

  while(1) {
    int discontinuity = 0;
    //    tmr :> t;
    //    t += 200000000;
    //    tmr when timerafter(t) :> void;

    tmr :> t;
    local_timestamp_to_ptp(ptp_ts, t, ptp_info);

    ptp_ts.seconds[0] += 2;
    ptp_ts.nanoseconds = 0;

    t = ptp_timestamp_to_local(ptp_ts, ptp_info);

    x = ptp_ts.seconds[0] & 1;

    while (!discontinuity) {
      tmr when timerafter(t) :> void;
      test_clock_port <: x;
      t0 = t + period/2/10;
      x = ~x;
      ptp_get_time_info(ptp_link, ptp_info);
      ptp_timestamp_offset(ptp_ts, period/2);
      t = ptp_timestamp_to_local(ptp_ts, ptp_info);
      t0 = t - t0;
      if (t0<0) t0 = -t0;
      if (t0 > 2000)
        discontinuity = 1;
    }
  }

#endif

}
