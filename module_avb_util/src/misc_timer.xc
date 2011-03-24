#include <xs1.h>
#include "swlock.h"


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
   return;
}
