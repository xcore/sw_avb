#ifndef __local_listener_stream_h__
#define __local_listener_stream_h__
#ifndef __XC__
#ifndef streaming
#define streaming
#endif
#ifndef NULLABLE
#define NULLABLE 
#endif
#else
#define NULLABLE ?
#endif
#include <xccompat.h>
#include "avb_conf.h"

#ifndef AVB_MAX_AUDIO_SAMPLE_RATE
#define AVB_MAX_AUDIO_SAMPLE_RATE (48000)
#endif

#ifndef MEDIA_OUTPUT_FIFO_SAMPLE_FIFO_SIZE
#define MEDIA_OUTPUT_FIFO_SAMPLE_FIFO_SIZE (AVB_MAX_AUDIO_SAMPLE_RATE/450)
#endif

typedef enum ofifo_state_t {
  DISABLED,
  ZEROING,
  LOCKING,
  LOCKED
} ofifo_state_t;


struct media_output_fifo_data_t {
  int zero_flag;							//!< When set, the FIFO will output zero samples instead of its contents
  unsigned int dptr;						//!< The read pointer
  unsigned int wrptr;						//!< The write pointer
  unsigned int marker;						//!<
  int local_ts;								//!<
  int ptp_ts;								//!<
  unsigned int sample_count;				//!<
  unsigned int zero_marker;					//!<
  ofifo_state_t state;						//!<
  int last_notification_time;				//!<
  int stream_num;							//!<
  int media_clock;							//!<
  int pending_init_notification;			//!<
  unsigned int sample_count_at_timestamp;	//!<
  unsigned int fifo[MEDIA_OUTPUT_FIFO_SAMPLE_FIFO_SIZE];
};

/**
 * \brief This type provides the data structure used by a media output FIFO.
 */
typedef struct media_output_fifo_data_t media_output_fifo_data_t;

/**
 * \brief This type provides a handle to a media output FIFO.
 **/
typedef int media_output_fifo_t;

/**
 *  \brief Output audio streams from media fifos to an XC channel.
 *
 * This function outputs samples from several media output FIFOs
 * over an XC channel over the streaming chanend ``samples_out``.
 * 
 * The protocol over the channel is that the thread expects a timestamp to 
 * be sent to it and then it will output ``num_channels`` samples, pulling 
 * from the ``ofifos`` array. It will then expect another timestamp before
 * the next set of samples. 
 *
 *  \param media_ctl            chanend connected to the main control thread
 *  \param samples_out          the chanend on which samples are output
 *  \param clk_ctl_index        the index in the clk_ctl array passed to 
 *                              media_clock_server() that controls the rate
 *                              of the FIFOs (i.e. the rate at which samples 
 *                              are pulled from the other end of the channel).
 *                              This should be -1 if the pull rate is not
 *                              controlled by the media clock server.
 *  \param ofifos               array of media output FIFOs to pull from
 *  \param num_channels         the number of channels (or FIFOs)
 **/
void media_output_fifo_to_xc_channel(chanend media_ctl,
                                     streaming chanend samples_out,
                                     int clk_ctl_index,
                                     media_output_fifo_t ofifos[],
                                     int num_channels);


/**
 *  \brief Output audio streams from media FIFOs to an XC channel, splitting left
 *  and right pairs.
 *
 * This function outputs samples from several media output FIFOs
 * over an XC channel over the streaming chanend ``samples_out``. The 
 * media FIFOs are assumed to be grouped in left/right stereo pairs which are
 * then split.
 * 
 * The protocol over the channel is that the thread expects a timestamp to 
 * be sent to it and then it will first output ``num_channels/2`` samples, 
 * pulling 
 * from all the even indexed elements of the ``ofifos`` array and then output 
 * all the odd elements.
 * It will then expect another timestamp before
 * the next set of samples.
 *
 *  \param media_ctl            chanend connected to the main control thread
 *  \param samples_out          the chanend on which samples are output
 *  \param clk_ctl_index        the index in the clk_ctl array passed to 
 *                              media_clock_server() that controls the rate
 *                              of the FIFOs (i.e. the rate at which samples 
 *                              are pulled from the other end of the channel).
 *                              This should be -1 if the pull rate is not
 *                              controlled by the media clock server.
 *  \param ofifos               array of media output fifos to pull from
 *  \param num_channels         the number of channels (or FIFOs)
 **/
void 
media_output_fifo_to_xc_channel_split_lr(chanend media_ctl,
                                         streaming chanend samples_out,    
                                         int clk_ctl_index,
                                         media_output_fifo_t output_fifos[],
                                         int num_channels);


void media_output_fifo_ctl(streaming chanend samples_out);

/**
 * \brief Get the number of the FIFO from a pointer to the structure
 */
int get_media_output_fifo_num(media_output_fifo_t s);

/**
 * \brief Intiialise a FIFO
 */
void media_output_fifo_init(media_output_fifo_t s, int stream_num);

/**
 * \brief Disable a FIFO
 *
 * This prevents samples from flowing through the FIFO
 */
void disable_media_output_fifo(media_output_fifo_t s);

/**
 * \brief Enable a FIFO
 *
 * This starts samples flowing through the FIFO
 */
void enable_media_output_fifo(media_output_fifo_t s,
                              int media_clock);

void 
media_output_fifo_maintain(media_output_fifo_t s,
                           chanend buf_ctl,
                           REFERENCE_PARAM(int, notified_buf_ctl));

void 
media_output_fifo_push_sample(media_output_fifo_t s,
                                  unsigned int sample);


#ifndef __XC__
void 
media_output_fifo_strided_push(media_output_fifo_t s0,
                               unsigned int *sample_ptr,
                               int stride,
                               int n);
#endif



void media_output_fifo_set_marker(media_output_fifo_t s,
                                      unsigned int timestamp);

int 
media_output_fifo_timestamp_info(media_output_fifo_t s,
                                 REFERENCE_PARAM(unsigned int, local_ts),
                                 REFERENCE_PARAM(unsigned int, ptp_ts));

unsigned int
media_output_fifo_pull_sample(media_output_fifo_t s,
                                  unsigned int timestamp);


void media_output_fifo_set_ptp_timestamp(media_output_fifo_t s0,
                                         unsigned int timestamp);


int 
media_output_fifo_get_timestamps(media_output_fifo_t s0,
                                 REFERENCE_PARAM(unsigned int, local_ts),
                                 REFERENCE_PARAM(unsigned int, ptp_ts));


void
media_output_fifo_handle_buf_ctl(chanend buf_ctl, 
                                 int stream_num,
                                 REFERENCE_PARAM(int, buf_ctl_notified));


/** Initialize media output FIFOs.
 *
 *  This function initializes media output FIFOs and ties the handles
 *  to their associated data structures. It should be called before the main 
 *  component function on a thread to setup the FIFOs.
 * 
 *  \param ofifos      an array of media output FIFO handles to initialize
 *  \param ofifo_data  an array of associated data structures
 *  \param n           the number of FIFOs to initialize
 **/
void
init_media_output_fifos(media_output_fifo_t ofifos[],
                       media_output_fifo_data_t ofifo_data[],
                       int n);


#endif


