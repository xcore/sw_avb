#ifndef __media_clock_internal_h__
#define __media_clock_internal_h__
#include <xccompat.h>
#include "media_clock_server.h"

/** A description of a media clock */
typedef struct media_clock_t {
  int active;
  media_clock_type_t clock_type;  ///< The type of the clock
  int source;               ///< If the clock is derived from a fifo
                            ///  this is the id of the output
                            ///  fifo it should be derived from.
  int rate;                 ///<  The rate of the media clock in Hz
  unsigned int wordLength;
} media_clock_t;


#define WC_FRACTIONAL_BITS 16

// The number of ticks to wait before trying to get initial lock
#define INITIAL_CLOCK_RECOVERY_DELAY (1<<26)

// The number of ticks between period clock recovery checks
#define CLOCK_RECOVERY_SLOW_PERIOD  300000000
#define CLOCK_RECOVERY_PERIOD  (1<<23)

// The number of samples the buffer can deviate from the fill point 
// over the recovery period to allow a lock
#define ACCEPTABLE_LOCKED_FILL_DEVIATION 20

void init_media_clock_recovery(chanend ptp_svr,
                                       int clock_info,
                                       unsigned int clk_time,
                                       unsigned int rate);

unsigned int update_media_clock(chanend ptp_svr, 
                                int clock_index,
                                REFERENCE_PARAM(media_clock_t, mclock),
                                unsigned int t2,
                                int period);


void update_media_clock_stream_info(int clock_index, 
                                    unsigned int local_ts,
                                    unsigned int outgoing_ptp_ts,
                                    unsigned int presentation_ts,
                                    int locked,
                                    int fill);

void inform_media_clock_of_lock(int clock_index);
#ifdef __XC__
void clock_recovery_maintain_buffer(chanend buf_info,
		chanend ptp_server,
			         int &locked,
			         int &balance,
			         int &fill_level,
			        unsigned int &t);
#else
void clock_recovery_maintain_buffer(chanend buf_info,
		chanend ptp_server,
			         int *locked,
			         int *balance,
			         int *fill_level,
			         unsigned int *t);
#endif



#endif
