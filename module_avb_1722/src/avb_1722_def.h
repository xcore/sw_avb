/**
 * \file avb_1722_def.h
 * \brief IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 */

#ifndef _AVB1722_DEF_H_
#define _AVB1722_DEF_H_ 1

#include "avb_1722_common.h"


// common definations
#define SW_REF_CLK_PERIOD_NS           (10)
#define IDEAL_NS_IN_SEC                (1000000000)
#define IDEAL_TIMER_TICKS_IN_SECOND    (IDEAL_NS_IN_SEC / SW_REF_CLK_PERIOD_NS)


#define INVALID_ADJUST_NS_PER_SECOND   (0xFFFFFFFF)


// AVB1722 default values
#define AVB1722_DEFAULT_AVB_PKT_DATA_LENGTH    (0x38)

// Default 61883-6 values
#define AVB1722_DEFAULT_TAG                    (1)
#define AVB1722_DEFAULT_CHANNEL                (31)
#define AVB1722_DEFAULT_TCODE                  (0xA)
#define AVB1722_DEFAULT_SY                     (0)

#define AVB1722_DEFAULT_EOH1                   (0)
#define AVB1722_DEFAULT_SID                    (63)
#define AVB1722_DEFAULT_DBS                    (2)
#define AVB1722_DEFAULT_FN                     (0)
#define AVB1722_DEFAULT_QPC                    (0)
#define AVB1722_DEFAULT_SPH                    (0)
#define AVB1722_DEFAULT_DBC                    (0)

#define AVB1722_DEFAULT_EOH2                   (2)
#define AVB1722_DEFAULT_FMT                    (0x10)
#define AVB1722_DEFAULT_FDF                    (2)
#define AVB1722_DEFAULT_SYT                    (0xFFFF)

#define AVB1722_PORT_UNINITIALIZED             (0xDEAD)

#define AVB_DEFAULT_VLAN                   (2)

// 61883-6 CIP header
#define AVB_CIP_HDR_SIZE    				(8)

// 61883-6/AVB1722 CIP Header definitions.
typedef struct
{
  unsigned char SID;             // bit 0,1 are fixed 00;
  unsigned char DBS;             // Data block size. In 61883 a data block is all information arriving
								 // at the transmitter in one sample period. Thus is our case it is the
                                 // size of one sample multiplied by the number of channels (in quadlets)
  unsigned char FN_QPC_SPH;      // bit 0-1 : Fraction Number
                                 // bit 2-4 : quadlet padding count
                                 // bit 5   : source packet header
  unsigned char DBC;             // data block count
  unsigned char FMT;             // stream format
  unsigned char FDF;             // format dependent field
  unsigned char SYT[2];          // synchronisation timing

} AVB_AVB1722_CIP_Header_t;

// NOTE: It is worth pointing out that the 'data block' in 61886-3 means a sample (or
// a collection of samples one for each channel)

//
// Macros for 61883 header
//
#define SET_AVB1722_CIP_TAG(x, a)              (x->protocol_specific[0] |= (a & 0x3) << 6)

#define SET_AVB1722_CIP_CHANNEL(x, a)          (x->protocol_specific[0] |= (a & 0x3F))
#define SET_AVB1722_CIP_TCODE(x, a)            (x->protocol_specific[1] |= (a & 0xF) << 4)
#define SET_AVB1722_CIP_SY(x, a)               (x->protocol_specific[1] |= (a & 0xF))

#define SET_AVB1722_CIP_SID(x, a)              (x->SID |= (a & 0x3F))
#define SET_AVB1722_CIP_EOH1(x, a)             (x->SID |= (a & 0x3) << 6)
#define SET_AVB1722_CIP_DBS(x, a)              (x->DBS = a)
#define SET_AVB1722_CIP_FN(x, a)               (x->FN_QPC_SPH |= (a & 0x3) << 6)
#define SET_AVB1722_CIP_QPC(x, a)              (x->FN_QPC_SPH |= (a & 0x7) << 3)
#define SET_AVB1722_CIP_SPH(x, a)              (x->FN_QPC_SPH |= (a & 0x1) << 2)
#define SET_AVB1722_CIP_DBC(x, a)              (x->DBC = a)
#define SET_AVB1722_CIP_EOH2(x, a)             (x->FMT |= (a & 0x3) << 6)
#define SET_AVB1722_CIP_FMT(x, a)              (x->FMT |= (a & 0x3F))
#define SET_AVB1722_CIP_FDF(x, a)              (x->FDF = a)
#define SET_AVB1722_CIP_SYT(x, a)              do {x->SYT[0] = a >> 8; \
                                                 x->SYT[1] = a & 0xFF; } while (0)

// Audio MBLA definitions (top 8 bits for easy combination with sample)
//   From 61883:
//   0100---- = multibit linear audio
//   ----00-- = raw audio sample
//   ------00 = 24 bit
//   ------01 = 20 bit
//   ------10 = 16 bit
//   ------11 = undefined
#define MBLA_24BIT                           (0x40000000)
#define MBLA_20BIT                           (0x41000000)
#define MBLA_16BIT                           (0x42000000)

// Generic configuration
#define ENABLE_AVB1722_TALKER                 (0x800000A5)
#define DISABLE_AVB1722_TALKER                (0x8000005A)
#define ENABLE_AVB1722_LISTENER               (0x800001A5)
#define DISABLE_AVB1722_LISTENER              (0x8000015A)
#define CHECK_AVB1722_STATUS                  (0x800015A5)
#define GET_AVB1722_STREAMS                   (0x80015A51)
#define SET_AVB1722_STREAM                    (0x8015A51A)
#define AVB1722_CONFIGURE_TALKER_STREAM       (0x800000B5)
#define AVB1722_CONFIGURE_LISTENER_STREAM     (0x8000B51A)
#define AVB1722_ADJUST_TALKER_STREAM          (0x800B5AB5)
#define AVB1722_ADJUST_LISTENER_STREAM        (0x800B5A1A)
#define AVB1722_DISABLE_TALKER_STREAM         (0x802233B5)
#define AVB1722_DISABLE_LISTENER_STREAM       (0x8022441A)
#define AVB1722_GET_ROUTER_LINK               (0x8522441A)
#define AVB1722_SET_VLAN                      (0x8522441B)
#define AVB1722_TALKER_GO                     (0x8AAA0001)
#define AVB1722_TALKER_STOP                   (0x8AAA0002)
#define AVB1722_SET_PORT                      (0x8AAA0101)

#define AVB1722_ACK                           (0x80000200)
#define AVB1722_NACK                          (0x80000300)

// Stream adjust specific commands
#define AVB1722_ADJUST_LISTENER_VOLUME        (0x00000001)

// The rate of 1722 packets (8kHz)
#define AVB1722_PACKET_RATE (8000)

// The number of samples per stream in each 1722 packet
#define TALKER_NUM_AUDIO_SAMPLES_PER_CHANNEL_PER_AVB1722_PKT (AVB_MAX_AUDIO_SAMPLE_RATE / AVB1722_PACKET_RATE)

// We add a 2% fudge factor to handle clock difference in the stream transmission shaping
#define AVB1722_PACKET_PERIOD_TIMER_TICKS (((100000000 / AVB1722_PACKET_RATE)*98)/100)

#define AVB1722_PLUS_SIP_HEADER_SIZE (32)

#endif
