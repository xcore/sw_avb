/**
 * Module:  module_avb_media_clock
 * Version: 1v0beta1
 * Build:   8bfffb3de662c21487d7e2a3a868dbbd51e5df8a
 * File:    media_clock_support.c
 *
 * The copyrights, all other intellectual and industrial
 * property rights are retained by XMOS and/or its licensors.
 * Terms and conditions covering the use of this code can
 * be found in the Xmos End User License Agreement.
 *
 * Copyright XMOS Ltd 2010
 *
 * In the case where this code is a modification of existing code
 * under a separate license, the separate license terms are shown
 * below. The modifications to the code are still covered by the
 * copyright notice above.
 *
 **/
#include <xccompat.h>
#include "gptp.h"
#include "avb_1722_def.h"
#include "print.h"
#include "media_clock_internal.h"
#include "media_clock_client.h"
#include "misc_timer.h"
#include <print.h>
#include "simple_printf.h"

#define NANO_SECOND 1000000000

// The clock recovery internal representation of the worldlen.  More precision and range than the external
// worldlen representation
#define WORDLEN_FRACTIONAL_BITS 32


typedef struct stream_info_t {
  int valid;
  unsigned int sample_count;
  unsigned int local_ts;
  unsigned int outgoing_ptp_ts;
  unsigned int presentation_ts;
  int locked;
  int fill;
} stream_info_t;

typedef struct clock_info_t {
  unsigned t1, ptp1;
  unsigned long long wordlen_ptp, wordlen;
  unsigned int rate;
  long long err;
  long long ierror;
  int first;
  stream_info_t stream_info1;
  stream_info_t stream_info2;
} clock_info_t;


static clock_info_t clock_states[MAX_NUM_MEDIA_CLOCKS];

static unsigned int local_wordlen_to_external_wordlen(unsigned long long w)
{
	return (w >> (WORDLEN_FRACTIONAL_BITS-WC_FRACTIONAL_BITS));
}

void init_media_clock_recovery(chanend ptp_svr,
                               int clock_num,
                               unsigned int clk_time,
                               unsigned int rate)
{
  clock_info_t *clock_info = &clock_states[clock_num];

  ptp_time_info_mod64 timeInfo;

  clock_info->first = 1;
  clock_info->rate = rate;
  clock_info->err = 0;
  clock_info->ierror = 0;
  if (rate != 0) {
    clock_info->wordlen_ptp = ((1000000000LL << WORDLEN_FRACTIONAL_BITS) / clock_info->rate);
    clock_info->wordlen = clock_info->wordlen_ptp / 10;
  }
  else {
    clock_info->wordlen = 0;
    clock_info->wordlen_ptp = 0; 
  }

  clock_info->t1 = clk_time;

  ptp_get_time_info_mod64(ptp_svr, &timeInfo);  	
  clock_info->ptp1 = local_timestamp_to_ptp_mod32(clk_time, &timeInfo);
	  
  clock_info->stream_info1.valid = 0;
  clock_info->stream_info2.valid = 0;
}




void update_media_clock_stream_info(int clock_index, 
                                    unsigned int sample_count,
                                    unsigned int local_ts,
                                    unsigned int outgoing_ptp_ts,
                                    unsigned int presentation_ts,
                                    int locked,
                                    int fill)
{
  clock_info_t *clock_info = &clock_states[clock_index];
  
  clock_info->stream_info2.sample_count = sample_count;
  clock_info->stream_info2.local_ts = local_ts;
  clock_info->stream_info2.outgoing_ptp_ts = outgoing_ptp_ts;
  clock_info->stream_info2.presentation_ts = presentation_ts;
  clock_info->stream_info2.valid = 1;
  clock_info->stream_info2.locked = locked;
  clock_info->stream_info2.fill = fill;
}

void inform_media_clock_of_lock(int clock_index) 
{
 clock_info_t *clock_info = &clock_states[clock_index];
 clock_info->stream_info2.valid = 0;
}

#define MAX_ERROR_TOLERANCE 1000

unsigned int update_media_clock(chanend ptp_svr, 
                                int clock_index,
                                media_clock_t *mclock,
                                unsigned int t2,
                                int period0)
{
  clock_info_t *clock_info = &clock_states[clock_index];
  ptp_time_info_mod64 timeInfo;
  long long diff_local;
  int clock_type = mclock->clock_type;

  switch (clock_type) 
    {
    case LOCAL_CLOCK:
        return local_wordlen_to_external_wordlen(clock_info->wordlen);
      break;
    case PTP_DERIVED: 
      {
        long long err, diff_ptp;
        unsigned ptp2;

        ptp_get_time_info_mod64(ptp_svr, &timeInfo);
        
        ptp2 = local_timestamp_to_ptp_mod32(t2, &timeInfo);

        {
            static int counts[6] = { 0,0,0,0,0,0 };
            static int i=0;
            int d = ((int)((signed) ptp2 - (signed) clock_info->ptp1) - 20971520)/10 + 2;
            if (d >= 0 && d <= 4)
            	counts[d]++;
            else
            	counts[5]++;

            if (i % 0x40 == 0)
            {
            	simple_printf("%d %d (%d) %d %d. outside=%d\n", counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
            }
            i++;
        }

        diff_local = (signed) t2 - (signed) clock_info->t1;
        diff_ptp = (signed) ptp2 - (signed) clock_info->ptp1;
              
        //      error in ns = diff_ptp - diff_local * wlptp / wl
        //      error in ns * wl = dptp * wl - dlocal * wlptp
        //      err = actual - expected

        err = (long long) diff_ptp * clock_info->wordlen -
          (long long) diff_local * clock_info->wordlen_ptp;
        
        err = (err << WORDLEN_FRACTIONAL_BITS) / clock_info->wordlen;

        if ((err >> WORDLEN_FRACTIONAL_BITS) > MAX_ERROR_TOLERANCE ||
            (err >> WORDLEN_FRACTIONAL_BITS) < -MAX_ERROR_TOLERANCE)
          {
            clock_info->wordlen_ptp = ((1000000000LL << WORDLEN_FRACTIONAL_BITS) / clock_info->rate);
            clock_info->wordlen = clock_info->wordlen_ptp / 10;
            clock_info->err = 0;
          }
        else {
          clock_info->err += err;

          long long diff = ((err / diff_local)*8) + (clock_info->err / diff_local) / 4;
          // adjust for error
          //clock_info->wordlen =
            //clock_info->wordlen  - diff;

          {
  			static int i=0;
  			if (i % 0x40 == 0) simple_printf("%x %x\n", (int)(clock_info->wordlen_ptp), (int)(clock_info->wordlen));
  			i++;
          }

        }
        
        clock_info->t1 = t2;
        clock_info->ptp1 = ptp2;                 
      break;
      }
    case MEDIA_FIFO_DERIVED:
      {long long ierror, perror;
        if (!clock_info->stream_info2.valid)
          return local_wordlen_to_external_wordlen(clock_info->wordlen);
        
        if (!clock_info->stream_info1.valid) {
          clock_info->stream_info1 = clock_info->stream_info2;
          clock_info->stream_info2.valid = 0;
          return local_wordlen_to_external_wordlen(clock_info->wordlen);
        }

        if (!clock_info->stream_info2.locked) {
            clock_info->wordlen = ((100000000LL << WORDLEN_FRACTIONAL_BITS) / clock_info->rate);
            clock_info->stream_info1 = clock_info->stream_info2;
            clock_info->stream_info2.valid = 0;
            clock_info->first = 1;
            clock_info->ierror = 0;
        }
        else {
          diff_local = 
            clock_info->stream_info2.local_ts -
            clock_info->stream_info1.local_ts;

          ierror = 
            (signed) clock_info->stream_info2.outgoing_ptp_ts -
            (signed) clock_info->stream_info2.presentation_ts;

          ierror = ierror << WORDLEN_FRACTIONAL_BITS;
          
          if (clock_info->first) {
            perror = 0;
            clock_info->first = 0;
          }
          else
            perror = ierror - clock_info->ierror;

          clock_info->ierror = ierror;

          // Kp = 32, Ki = 1 (originally)
          clock_info->wordlen =
            clock_info->wordlen  - (perror/diff_local) * 64 - (ierror/diff_local) * 4;

#if 0
          unsigned long long f = (100000000LL << WC_FRACTIONAL_BITS)*1000 / local_wordlen_to_external_wordlen(clock_info->wordlen);
          xscope_probe_data(2, (unsigned)f);
#endif

          clock_info->stream_info1 = clock_info->stream_info2;
          clock_info->stream_info2.valid = 0;          
        }
      }
      
      break;
    }
  
  return local_wordlen_to_external_wordlen(clock_info->wordlen);
}




