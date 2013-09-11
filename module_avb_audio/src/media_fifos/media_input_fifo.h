 /*
 * @ModuleName Audio DAC/LOCAL_TALKER_STREAM FIFO Defines.
 *
 *
 */

#ifndef _LOCAL_TALKER_STREAM_H_
#define _LOCAL_TALKER_STREAM_H_ 1
#include "avb_conf.h"

#ifndef AVB_MAX_AUDIO_SAMPLE_RATE
#define AVB_MAX_AUDIO_SAMPLE_RATE (48000)
#endif

#ifndef MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE
#define GET_SIZE(x) (x == 44100 || x == 48000) ? 64 : 128
#define MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE (GET_SIZE(AVB_MAX_AUDIO_SAMPLE_RATE))
#endif

typedef struct ififo_t {
  int wrIndex;				//!< Current write pointer
  int rdIndex;				//!< Current read pointer
  int startIndex;			//!< The pointer for the start of the packet being written
  int packetSize;			//!< Size of the packet, exclusing the Timestamp and DBC value.
  int sampleCountInPacket;	//!< The current number of words in the FIFO (samples only)
  unsigned int dbc;			//!< The Data Block Count from IEC.61883
  int fifoEnd;				//!< The index of the end of the FIFO
  int ptr;					//!< The read pointer used by the packet reader in the talker
  unsigned int buf[MEDIA_INPUT_FIFO_SAMPLE_FIFO_SIZE];
} ififo_t;

/** This type provides the data structure used by a media input fifo.
 */
typedef struct ififo_t media_input_fifo_data_t;

/**
 * This type provides a handle to a media input fifo.
 **/
typedef int media_input_fifo_t;

/**
 *  \brief Push a sample into the FIFO
 *
 *  If the packet of samples has just begun then a timestamp and data
 *  block count are added along with the first sample.  If the last sample
 *  of a packet has been written then the pointer to the start of the
 *  packet is updated.
 */
void
media_input_fifo_push_sample(media_input_fifo_t media_input_fifo,
                             unsigned int sample,
                             unsigned int ts);

#ifndef __XC__
/**
 * \brief Perform a blocking read of the next audio packet
 *
 * This is called by the talker thread when it needs the next audio
 * packet.  Later it will copy samples out of the packet, and ultimately
 * release the packet.
 *
 * \param media_input_fifo the media fifo
 */
unsigned int *
media_input_fifo_get_packet(media_input_fifo_t media_input_fifo,
                               unsigned int *ts,
                               unsigned int *dbc);
#endif

/**
 * \brief Release the packet after consuming the samples within
 *
 * This is called by the talker thread when it has written all of the
 * samples within a particular audio packet into 1722 AVBTP packets
 *
 * \param media_input_fifo  The fifo on which to release the packet
 */
void
media_input_fifo_release_packet(media_input_fifo_t media_input_fifo);

/** \brief Initialise the fifo
 *
 *  \param media_input_fifo the media fifo
 *  \param stream_num  the numberical index of the stream
 */
void
media_input_fifo_init(media_input_fifo_t media_input_fifo, int stream_num);

/**
 *  \brief Start storing samples/packets in the FIFO
 *
 *  \param media_input_fifo the media fifo
 *  \param rate the rate in Hz
 *  \return the number of samples in each audio packet
 */
int
media_input_fifo_enable(media_input_fifo_t media_input_fifo, int rate);

/**
 *  \brief Stop storing samples/packets in the FIFO
 *
 *  \param media_input_fifo the media fifo
 */
void
media_input_fifo_disable(media_input_fifo_t media_input_fifo);

/** Initialize media input fifos.
 *
 *  This function intializes media input FIFOs and ties the handles
 *  to their associated data structures. It should be called before the main
 *  component function on a thread to setup the FIFOs.
 *
 *  \param ififos      an array of media input FIFO handles to initialize
 *  \param ififo_data  an array of associated data structures
 *  \param n           the number of FIFOs to initialize
 **/
void
init_media_input_fifos(media_input_fifo_t ififos[],
                       media_input_fifo_data_t ififo_data[],
                       int n);


/**
 *  \brief Get the FIFO for the given stream number
 */
media_input_fifo_t get_media_input_fifo(int stream_num);


int media_input_fifo_fill_level(media_input_fifo_t media_input_fifo0);

/**
 *  \brief Check if the FIFO is empty
 */
int media_input_fifo_empty(media_input_fifo_t media_input_fifo0);

/**
 *   \brief Flush any complete packets which are sitting in the fifo
 *
 *   \param  media_input_fifo  The fifo to flush
 */
void media_input_fifo_flush(media_input_fifo_t media_input_fifo);

/**
 * \brief Enable a set of input fifos
 *
 * The talker threads use this to start the set of fifos which make up
 * a stream.  Before doing this, they should make sure that the FIFOs
 * are not enabled already by checking the indicated fifo state using
 * media_input_fifo_enable_ind_state.
 */
void media_input_fifo_enable_fifos(unsigned int enable);

/**
 * \brief Disable a set of input fifos
 *
 * The talker threads use this to stop a set of fifos from being filled.
 *
 */
void media_input_fifo_disable_fifos(unsigned int enable);

/**
 * \brief Get the enable state for a set of Fifos.
 *
 * The talker threads use this to determine the current enable state
 * of the fifos. After asking for the enable state of a set of fifos
 * to change, the talkers should not take any further action until
 * the enable state is reflected in this word, indicating that the
 * FIFO filling threads (I2S threads typically) has responded to the
 * request to start or stop filling the FIFO
 */
unsigned int media_input_fifo_enable_ind_state();

/**
 * \brief Check the requested enable state for the FIFOs
 *
 * The FIFO filling threads (I2S, TDM etc) threads can read the requested
 * enable state of the FIFOs and respond appropriately.  After they have
 * completed a sample input for each of the enabled channels, and emptied
 * the fifos for each of the non-active channels, then it should update
 * the FIFO enabled indication word by calling the function
 * media_input_fifo_update_enable_ind_state
 */
unsigned int media_input_fifo_enable_req_state();

/**
 * \brief Update the FIFO enabled indication state
 *
 * The FIFO filling threads should indicate that they have acted upon the
 * fifo enabled request state by calling this function once they have
 * completed a pass of sample input.
 */
void media_input_fifo_update_enable_ind_state(unsigned int enable, unsigned int mask);

#ifndef __XC__

/**
 *  \brief Update the current sample read pointer
 *
 *  This is used by the talker to store a pointer to the 'about to be
 *  written' sample in the audio packet.  1722 AVBTP packets do not
 *  necessarily line up with the packets of samples in the audio FIFOs.
 *
 *  This pointer is not the same as the packet read pointer in the
 *  FIFO structure, which points at the packet currently being read.
 *
 *  \param media_input_fifo0 The FIFO to update
 *  \param p The new value of the sample pointer
 */
inline void media_input_fifo_set_ptr(media_input_fifo_t media_input_fifo0,
                                     int *p)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
  media_input_fifo->ptr = (int) p;
  return;
}

/**
 * \brief Get the current sample pointer from the FIFO
 *
 * The talker thread stores a pointer to the current sample as discussed in
 * the media_input_fifo_set_ptr function description.
 *
 * \param media_input_fifo0 the fifo to read the pointer from
 * \return the value of the sample read pointer
 */
inline int *media_input_fifo_get_ptr(media_input_fifo_t media_input_fifo0)
{
  volatile ififo_t *media_input_fifo =  (ififo_t *) media_input_fifo0;
  return (int *) (media_input_fifo->ptr);
}
#endif

#endif
