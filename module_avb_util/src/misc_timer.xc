#include <xs1.h>
#include "misc_timer.h"

#define TICKS_PER_CENTISECOND (XS1_TIMER_KHZ * 10)
#define timeafter(A, B) ((int)((B) - (A)) < 0)

unsigned get_local_time(void)
{
   unsigned t;
  timer tmr;
   tmr :> t;
   return t;
}

void waitfor(unsigned t)
{
  timer tmr;
  tmr when timerafter(t) :> void;
}


void init_avb_timer(avb_timer& tmr, int mult)
{
  tmr.active = 0;
  tmr.timeout_multiplier = mult;
}

void start_avb_timer(avb_timer& tmr, unsigned int period_cs)
{
  tmr.period = (period_cs * TICKS_PER_CENTISECOND);
  tmr.timeout = get_local_time() + (period_cs * TICKS_PER_CENTISECOND);
  tmr.active = tmr.timeout_multiplier;
}

int avb_timer_expired(avb_timer& tmr)
{
  unsigned int now = get_local_time();
  if (!tmr.active)
    return 0;

  if (timeafter(now, tmr.timeout)) {
    tmr.active--;
    tmr.timeout = now + tmr.period;
  }

  return (tmr.active == 0);
}

void stop_avb_timer(avb_timer& tmr)
{
  tmr.active = 0;
}

