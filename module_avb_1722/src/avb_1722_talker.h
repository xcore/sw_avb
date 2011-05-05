/**
 * \file avb_1722_talker.h
 * \brief IEC 61883-6/AVB1722 Talker definitions
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

//! Data structure to identify Ethernet/AVB stream configuration.
typedef struct avb1722_Talker_StreamConfig_t
{
  //! 0=disabled, 1=enabled, 2=streaming
  int active;
  //! the destination mac address - typically a multicast address
  unsigned char destMACAdrs[MAC_ADRS_BYTE_COUNT];
  //! the source mac address - typically my own address
  unsigned char srcMACAdrs[MAC_ADRS_BYTE_COUNT];
  //! the stream ID
  unsigned int  streamId[2];
  //! number of channels in the stream
  unsigned int  num_channels;
  //! map from media fifos to channels in the stream
  media_input_fifo_t map[AVB_MAX_CHANNELS_PER_STREAM];  
  //! the type of samples in the stream
  unsigned int sampleType;
  //! Data Block Count (count of samples transmitted in the stream)
  unsigned int dbc;
  //! Number of samples per packet in the audio fifo
  unsigned int samples_per_fifo_packet;
  //! Number of samples per 1722 packet (integer part)
  unsigned int samples_per_packet_base;
  //! Number of samples per 1722 packet (16.16)
  unsigned int samples_per_packet_fractional;
  //! An accumulator for the fractional part
  unsigned int rem;
  //! The number of samples remaining in the fifo packet
  unsigned int samples_left;
  //! a flag, true when the stream has just been initialised
  unsigned int initial;
  //! the delay in ms that is added to the current PTP time
  unsigned presentation_delay;
  //! 1 when the packet should be transmitted without checking the transmission timer
  unsigned transmit_ok;
  //! the internal clock count when the last 1722 packet was transmitted
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
