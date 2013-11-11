/**
 * \file avb_1722_listener.h
 * \brief IEC 61883-6/AVB1722 Listener definitions
 */

#ifndef _AVB1722_LISTENER_H_
#define _AVB1722_LISTENER_H_ 1
#ifndef __XC__
#define streaming
#endif
#include <xccompat.h>
#include "avb_conf.h"
#include "avb_1722_def.h"
#include "gptp.h"
#include "media_output_fifo.h"

#ifndef MAX_INCOMING_AVB_STREAMS
#define MAX_INCOMING_AVB_STREAMS (AVB_NUM_SINKS)
#endif

#ifndef AVB_MAX_CHANNELS_PER_LISTENER_STREAM
#define AVB_MAX_CHANNELS_PER_LISTENER_STREAM 8
#endif

#ifndef MAX_AVB_STREAMS_PER_LISTENER
#define MAX_AVB_STREAMS_PER_LISTENER 4
#endif


typedef struct avb_1722_stream_info_t {
  short active;                    //!< 1-bit flag to say if the stream is active
  short state;                     //!< Generic state info
  int chan_lock;                   //!< Counter for locking onto a data stream
  int rate;                        //!< The estimated rate of the audio traffic
  int prev_num_samples;            //!< Number of samples in last received 1722 packet
  int num_channels_in_payload;     //!< The number of channels in the 1722 payloads
  int num_channels;
  int dbc;                         //!< The DBC of the last seen packet
  int last_sequence;               //!< The sequence number from the last 1722 packet
  media_output_fifo_t map[AVB_MAX_CHANNELS_PER_LISTENER_STREAM];
} avb_1722_stream_info_t;


#ifdef __XC__
int avb_1722_listener_process_packet(chanend? buf_ctl,
                                     unsigned char Buf[],
                                     int numBytes,
                                     REFERENCE_PARAM(avb_1722_stream_info_t, stream_info),
                                     NULLABLE_REFERENCE_PARAM(ptp_time_info_mod64, timeInfo),
                                     int index,
                                     REFERENCE_PARAM(int, notified_buf_ctl));
#else
int avb_1722_listener_process_packet(chanend buf_ctl,
                                     unsigned char Buf[],
                                     int numBytes,
                                     REFERENCE_PARAM(avb_1722_stream_info_t, stream_info),
				                             REFERENCE_PARAM(ptp_time_info_mod64, timeInfo),
                                     int index,
                                     REFERENCE_PARAM(int, notified_buf_ctl));
#endif

typedef struct avb_1722_listener_state_s {
  avb_1722_stream_info_t listener_streams[MAX_AVB_STREAMS_PER_LISTENER];
  int notified_buf_ctl;
  int router_link;
} avb_1722_listener_state_t;


#endif
