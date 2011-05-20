#ifndef MISC_TIMER_H_
#define MISC_TIMER_H_

#include <xccompat.h>

unsigned get_local_time(void);

void waitfor(unsigned t);


/*!
 * Utility for keeping track of timeout periods
 */
typedef struct {
  unsigned int timeout;
  unsigned int period;
  int active;
  int timeout_multiplier;
} avb_timer;

void init_avb_timer(REFERENCE_PARAM(avb_timer,tmr), int mult);
void start_avb_timer(REFERENCE_PARAM(avb_timer,tmr), unsigned int period_cs);
int avb_timer_expired(REFERENCE_PARAM(avb_timer,tmr));
void stop_avb_timer(REFERENCE_PARAM(avb_timer,tmr));



#endif /*MISC_TIMER_H_*/
