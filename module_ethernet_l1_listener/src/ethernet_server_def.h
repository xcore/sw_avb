// Copyright (c) 2011, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

/*************************************************************************
 *
 * Ethernet MAC Layer Implementation
 * IEEE 802.3 Ethernet Common Definitions
 *
 *
 *
 * Generic definations for Ethernet server/client.
 *
 *************************************************************************/
 
#ifndef _ETHERNET_SERVER_DEF_H_
#define _ETHERNET_SERVER_DEF_H_ 1

#ifdef __ethernet_conf_h_exists__
#include "ethernet_conf.h"
#endif

#ifndef MAX_ETHERNET_CLIENTS
#define MAX_ETHERNET_CLIENTS   (4)      // Number of link layers to support
#endif

/*****************************************************************************
 *
 *  DO NOT CHANGE THESE.
 *
 *****************************************************************************/

// Protocol definitions.
#define ETHERNET_TX_REQ                   (0x80000000)
#define ETHERNET_TX_REQ_TIMED             (0x80000001)
#define ETHERNET_GET_MAC_ADRS             (0x80000002)
#define ETHERNET_TX_SET_SPACING           (0x80000003)
#define ETHERNET_TX_REQ_OFFSET2           (0x80000004)
#define ETHERNET_TX_UPDATE_AVB_ROUTER     (0x80000005)
#define ETHERNET_TX_INIT_AVB_ROUTER       (0x80000006)
#define ETHERNET_TX_REQ_HP                (0x80000007)
#define ETHERNET_TX_REQ_TIMED_HP          (0x80000008)
#define ETHERNET_TX_REQ_OFFSET2_HP        (0x80000009)

#if defined(ETHERNET_TX_HP_QUEUE) && defined(ETHERNET_TRAFFIC_SHAPER)
#define ETHERNET_TX_SET_QAV_IDLE_SLOPE   (0x80000010)
#endif

#define ETHERNET_START_DATA              (0xA5DA1A5A) // Marker for start of data.

#define ETHERNET_RX_FRAME_REQ	         (0x80000010) // Request for ethernet
                                                      // complete frame,
                                                      // including src/dest
#define ETHERNET_RX_TYPE_PAYLOAD_REQ     (0x80000011) // Request for ethernet
                                                      // type and payload only
                                                      // (i.e. strip MAC
                                                      //  address(s))
#define ETHERNET_RX_OVERFLOW_CNT_REQ     (0x80000012)
#define ETHERNET_RX_OVERFLOW_MII_CNT_REQ (0x80000013)
#define ETHERNET_RX_FILTER_SET           (0x80000014)
#define ETHERNET_RX_DROP_PACKETS_SET     (0x80000015)
#define ETHERNET_RX_KILL_LINK            (0x80000016)
#define ETHERNET_RX_CUSTOM_FILTER_SET    (0x80000017)
#define ETHERNET_RX_QUEUE_SIZE_SET       (0x80000018)

#define ETHERNET_RX_FRAME_REQ_OFFSET2    (0x80000019)


#define ETHERNET_REQ_ACK	             (0x80000020) // Acknowledged
#define ETHERNET_REQ_NACK	             (0x80000021) // Negative ack.


#define ETH_BROADCAST (-1)


#define MII_CREDIT_FRACTIONAL_BITS 16
#endif
