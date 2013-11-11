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

#ifndef MEDIA_OUTPUT_FIFO_WORD_SIZE
#define MEDIA_OUTPUT_FIFO_WORD_SIZE (AVB_MAX_AUDIO_SAMPLE_RATE/450)
#endif

typedef enum ofifo_state_t {
  DISABLED, //!< Not active
  ZEROING,  //!< pushing zeros through to fill
  LOCKING,  //!< Clock recovery trying to lock to the sample stream
  LOCKED    //!< Clock recovery is locked and working
} ofifo_state_t;


struct media_output_fifo_data_t {
  int zero_flag;							//!< When set, the FIFO will output zero samples instead of its contents
  unsigned int dptr;						//!< The read pointer
  unsigned int wrptr;						//!< The write pointer
  unsigned int marker;						//!< This indicates which sample is the one which the timestamps apply to
  int local_ts;								//!< When a marked sample has played out, this contains the ref clock when it happened.
  int ptp_ts;								//!< Contains the PTP timestamp of the marked sample.
  unsigned int sample_count;				//!< The count of samples that have passed through the buffer.
  unsigned int zero_marker;					//!<
  ofifo_state_t state;						//!< State of the FIFO
  int last_notification_time;				//!< Last time that the clock recovery thread was informed of the timestamp info
  int media_clock;							//!<
  int pending_init_notification;			//!<
  int volume;                               //!< The linear volume multipler in 2.30 signed fixed point format
  unsigned int fifo[MEDIA_OUTPUT_FIFO_WORD_SIZE];
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
 *  \param samples_out          the chanend on which samples are output
 *  \param ofifos               array of media output FIFOs to pull from
 *  \param num_channels         the number of channels (or FIFOs)
 **/
void media_output_fifo_to_xc_channel(streaming chanend samples_out,
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
 *  \param samples_out          the chanend on which samples are output
 *  \param output_fifos         array of media output fifos to pull from
 *  \param num_channels         the number of channels (or FIFOs)
 **/
void
media_output_fifo_to_xc_channel_split_lr(streaming chanend samples_out,
                                         media_output_fifo_t output_fifos[],
                                         int num_channels);


/**
 * \brief Intiialise a FIFO
 */
void media_output_fifo_init(media_output_fifo_t s, unsigned stream_num);

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

/**
 *  \brief Perform maintanance on the FIFO, called periodically
 *
 *  This should be called periodically to allow the FIFO to
 *  perform tasks such as informing the clock recovery thread
 *  of some new timing information.
 *
 *  \param s the fifo to maintain
 *  \param buf_ctl a channel end that links the FIFO to the media clock service
 *  \param notified_buf_ctl pointer to a flag which is set when the media clock has been notified of a timing event in the FIFO
 */
void
media_output_fifo_maintain(media_output_fifo_t s,
                           chanend buf_ctl,
                           REFERENCE_PARAM(int, notified_buf_ctl));


#ifndef __XC__

/**
 *  \brief Push a set of samples into the FIFO
 *
 *  The samples are taken from the buffer pointed to by the sample_ptr,
 *  but are read from that buffer with a stride between each sample.
 *
 *  The 1722 listener thread uses this to put samples from the decoded
 *  packet into the audio FIFOs.
 *
 *  \param s0 the FIFO to push samples into
 *  \param sample_ptr a pointer to a block of samples in the 1722 packet
 *  \param stride the number of words between successive samples for this FIFO
 *  \param n the number of samples to push into the buffer
 */
void
media_output_fifo_strided_push(media_output_fifo_t s0,
                               unsigned int *sample_ptr,
                               int stride,
                               int n);
#endif


/**
 *  \brief Used by the audio output system to pull the next sample from the FIFO
 *
 *  If there are no samples in the buffer, a zero will be returned. The current
 *  ref clock time is passed into the function, and the FIFO will record this
 *  time if the sample which has been removed was the marked sample
 *
 *  \param s the FIFO to remove a sample from
 *  \param timestamp the ref clock time of the sample playout
 */
unsigned int
media_output_fifo_pull_sample(media_output_fifo_t s,
                                  unsigned int timestamp);


/**
 *  \brief Set the PTP timestamp on a specific sample in the buffer
 *
 *  When the 1722 thread unpacks a PDU, one of the samples in that
 *  PDU will have a PTP timestamp associated with it.  The 1722
 *  listener thread calls this to cause the FIFO to update control
 *  structures to record which sample is marked and the timestamp
 *  of that sample.
 *
 *  If the FIFO already has a marked timestamped sample within the
 *  buffer then it does not record the new timestamp.
 *
 *  \param s0 the media fifo which is being updated
 *  \param timestamp the 32 bit PTP timestamp
 *  \param sample_number the sample, counted from the end of the FIFO, which the timestamp applies to
 *
 */
void media_output_fifo_set_ptp_timestamp(media_output_fifo_t s0,
                                         unsigned int timestamp,
                                         unsigned sample_number);


/**
 *  \brief Handle notification events on the buffer control channel
 *
 *  The 1722 listener thread notifies the clock server about timing
 *  events in the audio FIFOs using a channel. This function provides
 *  the processing of the messages which can be sent by the clock
 *  recovery thread over that channel.
 *
 *  \param buf_ctl  the communication channel with the clock recovery service
 *  \param stream_num  the number of the stream which is being handled
 *  \param buf_ctl_notified pointer to the flag which indicates whether the clock recovery thread has been notified of a timing event
 */
void
media_output_fifo_handle_buf_ctl(chanend buf_ctl,
                                 int stream_num,
                                 REFERENCE_PARAM(int, buf_ctl_notified),
                                 timer tmr);

/**
 *  \brief Set the volume control multiplier for the media FIFO
 *
 *  \param s0 the media fifo to set the volume for
 *  \param volume the 2.30 signed fixed point linear volume multiplier
 */
void
media_output_fifo_set_volume(media_output_fifo_t s0,
                             unsigned int volume);


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


