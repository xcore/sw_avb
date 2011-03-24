#ifndef _calc_word_length_h
#define _calc_word_length_h

#include "ptp_data_types.h"


unsigned int fixedp_div(int a, int b, int bits);

/** This function calculates the word length of 
 *  a sample given two sample counts and two nanosecond timestamps taken
 *  at different times.
 *   
 *   Note that the timers need to be within ~2secs of each other.
 */
unsigned int calc_init_word_length(unsigned int c1,
                                   unsigned int c2,
                                   unsigned int t1,
                                   unsigned int t2,
                                   int adjustNSPerSecond);



/** Calculates the difference between an AVB timestamp and a local timestamp
 *  in terms of number of samples.
 *
 *  Return value is an signed 16.16 fixed point value.
 */
#ifdef __XC__
int calc_avb_sample_diff(SystemTimeCtl &timeinfo,
                         unsigned int avb_timestamp,
                         unsigned int local_timestamp,
                         int adjustNSPerSecond,
                         unsigned int wordLength);
#else
int calc_avb_sample_diff(SystemTimeCtl *timeinfo,
                         unsigned int avb_timestamp,
                         unsigned int local_timestamp,
                         int adjustNSPerSecond,
                         unsigned int wordLength);
#endif

#ifdef __XC__
int calc_avb_correction(SystemTimeCtl &timeinfo,
                        unsigned int avb_timestamp,
                        unsigned int local_timestamp,
                        int adjustNSPerSecond,
                        unsigned int wordLength,
                        unsigned int time_period_ticks);
#else
int calc_avb_correction(SystemTimeCtl *timeinfo,
                        unsigned int avb_timestamp,
                        unsigned int local_timestamp,
                        int adjustNSPerSecond,
                        unsigned int wordLength,
                        unsigned int time_period_ticks);

#endif

int calc_adjusted_wrdclk(unsigned int wordLength, int adjustNSPerSecond);

#endif
