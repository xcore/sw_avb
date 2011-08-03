/** \file avb_1722_1.h
 *
 *  1722.1 is a discovery and control protocol for 1722.  It is a layer 2 protocol
 *  similar to Open Sound Control (OSC), and allows for 1722 endpoints to be discovered
 *  by a central controller, and connections between talkers and listeners to be
 *  established
 */


#ifndef _avb_1722_1_h_
#define _avb_1722_1_h_

#include <xccompat.h>

#include "avb_control_types.h"
#include "avb_1722_1_protocol.h"

#ifdef __avb_1722_1_conf_h_exists__
#include "avb_1722_1_conf.h"
#endif

#ifndef AVB_1722_1_ADP_VALID_TIME
#define AVB_1722_1_ADP_VALID_TIME (31)  // 31*2 = 62 seconds validity time
#define AVB_1722_1_ADP_REPEAT_TIME (AVB_1722_1_ADP_VALID_TIME/2)
#endif

#ifndef AVB_1722_1_ADP_VENDOR_ID
#define AVB_1722_1_ADP_VENDOR_ID 0x00229700u
#endif

#ifndef AVB_1722_1_ADP_MODEL_ID
#define AVB_1722_1_ADP_MODEL_ID 0
#endif

#ifndef AVB_1722_1_ADP_ENTITY_CAPABILITIES
#define AVB_1722_1_ADP_ENTITY_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_ADP_TALKER_STREAM_SOURCES
#define AVB_1722_1_ADP_TALKER_STREAM_SOURCES 0
#endif

#ifndef AVB_1722_1_ADP_TALKER_CAPABILITIES
#define AVB_1722_1_ADP_TALKER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_ADP_LISTENER_STREAM_SINKS
#define AVB_1722_1_ADP_LISTENER_STREAM_SINKS 0
#endif

#ifndef AVB_1722_1_ADP_LISTENER_CAPABILITIES
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_ADP_CONTROLLER_CAPABILITIES
#define AVB_1722_1_ADP_CONTROLLER_CAPABILITIES 0
#endif

#ifndef AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT
#define AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT (AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_2_CH|\
											 AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_48K|\
											 AVB_1722_1_ADP_DEFAULT_AUDIO_FORMAT_MAX_STREAMS_8)
#endif

#ifndef AVB_1722_1_ADP_DEFAULT_VIDEO_FORMAT
#define AVB_1722_1_ADP_DEFAULT_VIDEO_FORMAT 0
#endif

#ifndef AVB_1722_1_ADP_ASSOCIATION_ID
#define AVB_1722_1_ADP_ASSOCIATION_ID 0
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

/* Debug defines */
/*
#ifndef AVB_1722_1_ADP_DEBUG_ENTITY_REMOVAL
#define AVB_1722_1_ADP_DEBUG_ENTITY_REMOVAL
#endif
*/

/** \fn avb_1722_1_init
 *
 *  Initialization
 */
void avb_1722_1_init(unsigned char macaddr[6], unsigned char serial_number[2]);

/** \fn avb_1722_1_periodic
 *
 *  This function performs periodic processing. It must be called frequently
 */
avb_status_t avb_1722_1_periodic(chanend tx, chanend c_ptp);

/** \fn avb_1722_1_process_packet
 *
 *  Process a received 1722_1 packet
 */
avb_status_t avb_1722_1_process_packet(unsigned int buf[], int len, chanend c_tx);

/** \fn avb_1722_1_adp_announce
 *
 *  Start advertising that this is an endpoint
 */
void avb_1722_1_adp_announce();

/** \fn avb_1722_1_adp_depart
 *
 *  Stop advertising that this is an endpoint
 */
void avb_1722_1_adp_depart();

/** \fn avb_1722_1_adp_discover
 *
 *  Ask to discover the information for a specific entity guid
 */
void avb_1722_1_adp_discover(unsigned guid[]);

/** \fn avb_1722_1_adp_discover_all
 *
 *  Ask to discover all available entities
 */
void avb_1722_1_adp_discover_all();

/** \fn avb_1722_1_adp_change_ptp_grandmaster
 *
 *  Set the current PTP grandmaster used by ADP when it changes
 */
void avb_1722_1_adp_change_ptp_grandmaster(char grandmaster[8]);

/** \fn avb_1722_1_acmp_get_talker_connection_info
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_TALKER and
 * AVB_1722_1_DISCONNECT_TALKER notifications.
 */
unsigned avb_1722_1_acmp_get_talker_connection_info(REFERENCE_PARAM(short,talker));

/** \fn avb_1722_1_acmp_get_listener_connection_info
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_LISTENER and
 * AVB_1722_1_DISCONNECT_LISTENER notifications.
 */
unsigned avb_1722_1_acmp_get_listener_connection_info(REFERENCE_PARAM(short,listener), char address[6], unsigned streamId[2], REFERENCE_PARAM(unsigned,vlan));

/** \fn avb_1722_1_acmp_talker_connection_complete
 *
 * Inform the 1722.1 state machine that the AVB_1722_1_CONNECT_TALKER or
 * AVB_1722_1_DISCONNECT_TALKER has completed, and passing in an error
 * code.
 *
 * \param code the 1722.1 status field code
 */
void avb_1722_1_acmp_talker_connection_complete(short code, chanend c_tx);

/** \fn avb_1722_1_acmp_listener_connection_complete
 *
 * Inform the 1722.1 state machine that the AVB_1722_1_CONNECT_LISTENER or
 * AVB_1722_1_DISCONNECT_LISTENER has completed, and passing in an error
 * code.
 *
 * \param code the 1722.1 status field code
 */
void avb_1722_1_acmp_listener_connection_complete(short code, chanend c_tx);

/** \fn avb_1722_1_talker_set_mac_address
 *
 *  Called by the application to inform 1722.1 of the source MAC
 *  address for the particular talker.
 */
void avb_1722_1_talker_set_mac_address(unsigned talker_unique_id, char macaddr[]);

/** \fn avb_1722_1_talker_set_stream_id
 *
 * Called by the application to inform 1722.1 of the source stream identifier
 * for a particular stream.
 */
void avb_1722_1_talker_set_stream_id(unsigned talker_unique_id, unsigned streamId[2]);


/** \fn avb_1722_1_acmp_configure_source
 *
 *  Utility function to configure a source according to the value of the ACMP
 *  default format parameter.  Returns a status which can be passed back into
 *  the code parameter of the avb_1722_1_acmp_talker_connection_complete function
 *
 *  \param talker_unique_id the index of the talker source
 *  \param default_format the default format from the 1722.1 ACMP message
 *  \returns a 1722.1 ACMP status code
 */
short avb_1722_1_acmp_configure_source(unsigned talker_unique_id, unsigned int default_format);

/** \fn avb_1722_1_acmp_configure_sink
 *
 *  Utility function to configure a sink according to the value of the ACMP
 *  default format parameter.  Returns a status which can be passed back into
 *  the code parameter of the avb_1722_1_acmp_listener_connection_complete function
 *
 *  \param listener_unique_id the index of the listener sink
 *  \param default_format the default format from the 1722.1 ACMP message
 *  \returns a 1722.1 ACMP status code
 */
short avb_1722_1_acmp_configure_sink(unsigned listener_unique_id, unsigned int default_format);

#endif





