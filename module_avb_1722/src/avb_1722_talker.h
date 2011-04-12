/*
 * @ModuleName IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 * @Description: Implements Talker functionality.
 *
 *
 *
 */


#ifndef __AVB_AVB1722_TALKER_H__
#define __AVB_AVB1722_TALKER_H__ 1

#include "avb_1722_def.h"
#include <xccompat.h>
#include "avb_conf.h" 
#include "gptp.h"
#include "media_input_fifo.h"


#ifndef AVB_MAX_CHANNELS_PER_STREAM 
#define AVB_MAX_CHANNELS_PER_STREAM 16
#endif

#ifndef AVB_MAX_STREAMS_PER_TALKER_UNIT
#define AVB_MAX_STREAMS_PER_TALKER_UNIT (AVB_NUM_SOURCES)
#endif

// Data structure to identify Ethernet/AVB stream configuration.
typedef struct avb1722_Talker_StreamConfig_t
{
  int active;
  unsigned char destMACAdrs[MAC_ADRS_BYTE_COUNT];
  unsigned char srcMACAdrs[MAC_ADRS_BYTE_COUNT];   
  unsigned int  streamId[2];
  unsigned int  num_channels;
  media_input_fifo_t map[AVB_MAX_CHANNELS_PER_STREAM];  
  unsigned int sampleType;
  unsigned int dbc;
  unsigned int prev_dbc;
  unsigned int latency;
  unsigned int samples_per_fifo_packet;
  unsigned int samples_per_packet_base;
  unsigned int samples_per_packet_fractional;
  unsigned int rem;
  unsigned int samples_left;
  unsigned int initial;
  unsigned presentation_delay;
  unsigned transmit_ok;
  int last_transmit_time;
} avb1722_Talker_StreamConfig_t;


/** Sets the vlan id in outgoing packet buffer for outgoing 1722 packets.
 */
void avb1722_set_buffer_vlan(int vlan,
                             unsigned char Buf[]);

/** This configure AVB Talker buffer for a given stream configuration.
 *  It updates the static portion of Ehternet/AVB transport layer headers.
 */
void AVB1722_Talker_bufInit(unsigned char Buf[],
                            REFERENCE_PARAM(avb1722_Talker_StreamConfig_t,
                                            pStreamConfig),
                            int vlan_id);

/** This receives user defined audio samples from local out stream and packetize
 *  them into specified AVB1722 transport packet.
 */
int avb1722_create_packet(unsigned char Buf[],
                          REFERENCE_PARAM(avb1722_Talker_StreamConfig_t,
                                          stream_info),
                          REFERENCE_PARAM(ptp_time_info_mod64,
                                          timeInfo),
                          int time);

#endif
