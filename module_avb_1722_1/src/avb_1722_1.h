
#ifndef _avb_1722_1_h_
#define _avb_1722_1_h_

#include "avb_control_types.h"
#include "avb_1722_1_protocol.h"

#ifdef __avb_1722_1_conf_h_exists__
#include "avb_1722_1_conf.h"
#endif

#ifndef AVB_1722_1_SDP_VALID_TIME
#define AVB_1722_1_SDP_VALID_TIME (30)  // 30*2 = 60 seconds validity time
#endif

#ifndef AVB_1722_1_SDP_ENTITY_GUID_LO
#define AVB_1722_1_SDP_ENTITY_GUID_LO 0
#endif

#ifndef AVB_1722_1_SDP_ENTITY_GUID_HI
#define AVB_1722_1_SDP_ENTITY_GUID_HI 0
#endif

#ifndef AVB_1722_1_SDP_VENDOR_ID
#define AVB_1722_1_SDP_VENDOR_ID 0
#endif

#ifndef AVB_1722_1_SDP_MODEL_ID
#define AVB_1722_1_SDP_MODEL_ID 0
#endif

#ifndef AVB_1722_1_SDP_ENTITY_CAPIBILITIES
#define AVB_1722_1_SDP_ENTITY_CAPIBILITIES 0
#endif

#ifndef AVB_1722_1_SDP_TALKER_STREAM_SOURCES
#define AVB_1722_1_SDP_TALKER_STREAM_SOURCES 0
#endif

#ifndef AVB_1722_1_SDP_TALKER_CAPABILITIES
#define AVB_1722_1_SDP_TALKER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_SDP_LISTENER_STREAM_SINKS
#define AVB_1722_1_SDP_LISTENER_STREAM_SINKS 0
#endif

#ifndef AVB_1722_1_SDP_LISTENER_CAPIBILITIES
#define AVB_1722_1_SDP_LISTENER_CAPIBILITIES 0
#endif

#ifndef AVB_1722_1_SDP_CONTROLLER_CAPABILITIES
#define AVB_1722_1_SDP_CONTROLLER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_SDP_BOOT_ID
#define AVB_1722_1_SDP_BOOT_ID 0
#endif


/** \file avb_1722_1.h
 *
 *  1722.1 is a discovery and control protocol for 1722.  It is a layer 2 protocol
 *  similar to Open Sound Control (OSC), and allows for 1722 endpoints to be discovered
 *  by a central controller, and connections between talkers and listeners to be
 *  established
 */


/** \fn avb_1722_1_init
 *
 *  Initialization
 */
void avb_1722_1_init(unsigned char macaddr[6]);

/** \fn avb_1722_1_periodic
 *
 *  This function performs periodic processing. It must be called
 *  approximately 4 times a second.
 */
void avb_1722_1_periodic(chanend c_tx);

/** \fn avb_1722_1_process_packet
 *
 *  Process a received 1722_1 packet
 */
avb_status_t avb_1722_1_process_packet(unsigned int buf[], int len, chanend c_tx);

/** \fn avb_1722_1_sdp_announce
 *
 *  Start advertising that this is an endpoint
 */
void avb_1722_1_sdp_announce();

/** \fn avb_1722_1_sdp_depart
 *
 *  Stop advertising that this is an endpoint
 */
void avb_1722_1_sdp_depart();

/** \fn avb_1722_1_sdp_discover
 *
 *  Ask to discover the information for a specific entity guid
 */
void avb_1722_1_sdp_discover(unsigned guid[]);

/** \fn avb_1722_1_sdp_discover_all
 *
 *  Ask to discover all available entities
 */
void avb_1722_1_sdp_discover_all();

#endif





