/** \file avb_1722_1.h
 *
 *  1722.1 is a discovery and control protocol for 1722.  It is a layer 2 protocol
 *  similar to Open Sound Control (OSC), and allows for 1722 endpoints to be discovered
 *  by a central controller, and connections between talkers and listeners to be
 *  established
 */


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

#ifndef AVB_1722_1_SDP_VENDOR_ID
#define AVB_1722_1_SDP_VENDOR_ID 0
#endif

#ifndef AVB_1722_1_SDP_MODEL_ID
#define AVB_1722_1_SDP_MODEL_ID 0
#endif

#ifndef AVB_1722_1_SDP_ENTITY_CAPABILITIES
#define AVB_1722_1_SDP_ENTITY_CAPABILITIES 0
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

#ifndef AVB_1722_1_SDP_LISTENER_CAPABILITIES
#define AVB_1722_1_SDP_LISTENER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_SDP_CONTROLLER_CAPABILITIES
#define AVB_1722_1_SDP_CONTROLLER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_SDP_BOOT_ID
#define AVB_1722_1_SDP_BOOT_ID 0
#endif

#ifndef AVB_1722_1_MAX_ENTITIES
#define AVB_1722_1_MAX_ENTITIES 4
#endif

#ifndef AVB_1722_1_MAX_LISTENERS
#define AVB_1722_1_MAX_LISTENERS 1
#endif

#ifndef AVB_1722_1_MAX_TALKERS
#define AVB_1722_1_MAX_TALKERS 1
#endif

#ifndef AVB_1722_1_MAX_LISTENERS_PER_TALKER
#define AVB_1722_1_MAX_LISTENERS_PER_TALKER 4
#endif

#ifndef AVB_1722_1_MAX_INFLIGHT_COMMANDS
#define AVB_1722_1_MAX_INFLIGHT_COMMANDS 2
#endif


/** \fn avb_1722_1_init
 *
 *  Initialization
 */
void avb_1722_1_init(unsigned char macaddr[6], unsigned int serial_number);

/** \fn avb_1722_1_periodic
 *
 *  This function performs periodic processing. It must be called frequently
 */
avb_status_t avb_1722_1_periodic(chanend c_tx);

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

/** \fn avb_1722_1_scm_get_talker_connection_info
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_TALKER and
 * AVB_1722_1_DISCONNECT_TALKER notifications.
 */
unsigned avb_1722_1_scm_get_talker_connection_info();

/** \fn avb_1722_1_scm_get_listener_connection_info
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_LISTENER and
 * AVB_1722_1_DISCONNECT_LISTENER notifications.
 */
unsigned avb_1722_1_scm_get_listener_connection_info();

/** \fn avb_1722_1_scm_talker_connection_complete
 *
 * Inform the 1722.1 state machine that the AVB_1722_1_CONNECT_TALKER or
 * AVB_1722_1_DISCONNECT_TALKER has completed, and passing in an error
 * code.
 *
 * \param code the 1722.1 status field code
 */
void avb_1722_1_scm_talker_connection_complete(short code);

/** \fn avb_1722_1_scm_listener_connection_complete
 *
 * Inform the 1722.1 state machine that the AVB_1722_1_CONNECT_LISTENER or
 * AVB_1722_1_DISCONNECT_LISTENER has completed, and passing in an error
 * code.
 *
 * \param code the 1722.1 status field code
 */
void avb_1722_1_scm_listener_connection_complete(short code);

#endif





