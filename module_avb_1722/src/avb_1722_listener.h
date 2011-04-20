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
#include "media_output_fifo.h"

#ifndef MAX_INCOMING_AVB_STREAMS 
#define MAX_INCOMING_AVB_STREAMS (AVB_NUM_SINKS)
#endif

#ifndef AVB_MAX_CHANNELS_PER_STREAM 
#define AVB_MAX_CHANNELS_PER_STREAM 16
#endif

#ifndef MAX_AVB_STREAMS_PER_LISTENER
#define MAX_AVB_STREAMS_PER_LISTENER 12
#endif


typedef struct avb_1722_stream_info_t {
  int active;
  int num_channels;
  int dbc;
  int count;
  int chan_lock;
  int prev_num_samples;
  int num_channels_in_payload;
  media_output_fifo_t map[AVB_MAX_CHANNELS_PER_STREAM];
} avb_1722_stream_info_t;



int 
avb_1722_listener_process_packet(chanend buf_ctl,
                                 unsigned char Buf[], 
                                 int numBytes,
                                 REFERENCE_PARAM(avb_1722_stream_info_t, 
                                                 stream_info),
                                 int index,
                                 REFERENCE_PARAM(int, notified_buf_ctl));

#endif
