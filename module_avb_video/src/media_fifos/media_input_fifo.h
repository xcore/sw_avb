 /*
 * @ModuleName Audio DAC/LOCAL_TALKER_STREAM FIFO Defines.
 *
 *
 */
 
#ifndef _LOCAL_TALKER_STREAM_H_ 
#define _LOCAL_TALKER_STREAM_H_ 1
#include "avb_conf.h"

/*
 *   The TS FIFO is a word buffer. Each packet within the buffer is layed out as:
 *
 *   1 word of dummy data.  during FIFO overflow, these dummy words are used for dumping an incoming packet
 *   1 word of timestamp
 *   188 bytes (47 words) of TS packet
 *   1 word of inuse marker
 *
 *   The inuse marker is set to a non-zero value when the buffer preceeding it contains valid data, and
 *   zero when it is available for reception.
 */

// A TS packet is a one word dummy space, 1 word timestamp, 188 bytes payload, plus 1 word for an 'inuse' marker
#define TS_INPUT_PACKET_SIZE (4+4+188+4)

// The 61883-4, section 7, recommends this number of packets be stored
#define TS_INPUT_FIFO_SIZE 16

// Size of the FIFO in words
#define MEDIA_INPUT_FIFO_WORD_SIZE ((TS_INPUT_PACKET_SIZE*TS_INPUT_FIFO_SIZE)/4)

// Word index of inuse byte
#define MEDIA_INPUT_FIFO_INUSE_OFFSET 49

typedef struct ififo_t {
	unsigned packet_rd;
	unsigned int fifo[MEDIA_INPUT_FIFO_WORD_SIZE];
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
media_input_fifo_get_packet(media_input_fifo_t media_input_fifo);
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

#endif
