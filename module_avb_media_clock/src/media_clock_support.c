#include <xccompat.h>
#include "gptp.h"
#include "avb_1722_def.h"
#include "print.h"
#include "media_clock_internal.h"
#include "misc_timer.h"
#include <print.h>
#include "simple_printf.h"
#define NANO_SECOND 1000000000


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
  unsigned wordlen_ptp, wordlen;
  unsigned int rate;
  int err;
  long long ierror;
  int first;
  stream_info_t stream_info1;
  stream_info_t stream_info2;
} clock_info_t;


static clock_info_t clock_states[MAX_NUM_MEDIA_CLOCKS];

unsigned init_media_clock_recovery(chanend ptp_svr, 
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
    long long wordlen;
    wordlen = (int) ((100000000LL << WC_FRACTIONAL_BITS) / clock_info->rate);
    clock_info->wordlen = wordlen;
    clock_info->wordlen_ptp = wordlen*10;
  }
  else {
    clock_info->wordlen = 0;
    clock_info->wordlen_ptp = 0; 
  }


  ptp_get_time_info_mod64(ptp_svr, &timeInfo);  	
  
  clock_info->t1 = clk_time;
  clock_info->ptp1 = local_timestamp_to_ptp_mod32(clk_time, &timeInfo);
	  
  clock_info->stream_info1.valid = 0;
  clock_info->stream_info2.valid = 0;
  return clock_info->wordlen;
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
  return;
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
  unsigned ptp2;
  unsigned diff_local, diff_ptp;
  int clock_type = mclock->clock_type;
  static int count=1;
  count++;      
  switch (clock_type) 
    {
    case LOCAL_CLOCK:
      return (clock_info->wordlen);
      break;
    case PTP_DERIVED: 
      {
        long long err;
             
        ptp_get_time_info_mod64(ptp_svr, &timeInfo);
        
        ptp2 = local_timestamp_to_ptp_mod32(t2, &timeInfo);
      
        diff_local = (signed) t2 - (signed) clock_info->t1;
        diff_ptp = (signed) ptp2 - (signed) clock_info->ptp1;
              
        //      error in ns = diff_ptp - diff_local * wlptp / wl
        //      error in ns * wl = dptp * wl - dlocal * wlptp
        //      err = actual - expected

        err = (long long) diff_ptp * (long long) clock_info->wordlen -
          (long long) diff_local * (long long) clock_info->wordlen_ptp;
        
        err = (err << WC_FRACTIONAL_BITS) / (int) clock_info->wordlen;
        
        if ((err >> WC_FRACTIONAL_BITS) > MAX_ERROR_TOLERANCE || 
            (err >> WC_FRACTIONAL_BITS) < -MAX_ERROR_TOLERANCE) 
          {
            long long wordlen;
            wordlen = (int) ((100000000LL << WC_FRACTIONAL_BITS) / clock_info->rate);
            clock_info->wordlen = wordlen;
            clock_info->wordlen_ptp = wordlen*10;
            clock_info->err = 0;

          }
        else {
          clock_info->err += err;      
          // adjust for error
          clock_info->wordlen =
            clock_info->wordlen  - ((int) (err/(int) diff_local)*8) - (clock_info->err / (int) diff_local) / 4;
        }
        
        clock_info->t1 = t2;
        clock_info->ptp1 = ptp2;                 
      break;
      }
    case MEDIA_FIFO_DERIVED:
      {long long ierror, perror;

        if (!clock_info->stream_info2.valid)
          return clock_info->wordlen;
        
        if (!clock_info->stream_info1.valid) {
          clock_info->stream_info1 = clock_info->stream_info2;
          clock_info->stream_info2.valid = 0;
          return clock_info->wordlen;
        }

        if (!clock_info->stream_info2.locked) {
            long long wordlen;
            wordlen = (int) ((100000000LL << WC_FRACTIONAL_BITS) / clock_info->rate);
            clock_info->wordlen = wordlen;
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
                 
          ierror = ierror << WC_FRACTIONAL_BITS;
          
          if (clock_info->first) {
            perror = 0;
            clock_info->first = 0;
          }
          else
            perror = ierror - clock_info->ierror;
          
          clock_info->ierror = ierror;
                    
          clock_info->wordlen =
            clock_info->wordlen  - ((int) (perror/(int) diff_local)*32) - (ierror / (int) diff_local);


          clock_info->stream_info1 = clock_info->stream_info2;
          clock_info->stream_info2.valid = 0;          
        }
      }
      
      break;
    }
  
  return (clock_info->wordlen);
}




