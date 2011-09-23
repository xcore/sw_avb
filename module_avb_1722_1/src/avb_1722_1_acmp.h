#ifndef AVB_1722_1_ACMP_H_
#define AVB_1722_1_ACMP_H_

#include "avb_1722_1_acmp_pdu.h"
#include "avb_control_types.h"

#define AVB_1722_1_ACMP_DEST_MAC {0x91, 0xe0, 0xf0, 0x00, 0xff, 0x01};

void avb_1722_1_acmp_controller_init();
void avb_1722_1_acmp_talker_init();
void avb_1722_1_acmp_listener_init();

avb_status_t process_avb_1722_1_acmp_packet(REFERENCE_PARAM(avb_1722_1_acmp_packet_t, pkt), chanend c_tx);
avb_status_t avb_1722_1_acmp_controller_periodic(chanend c_tx);
avb_status_t avb_1722_1_acmp_talker_periodic(chanend c_tx);
avb_status_t avb_1722_1_acmp_listener_periodic(chanend c_tx);

void acmp_controller_connect(REFERENCE_PARAM(guid_t, talker_guid), REFERENCE_PARAM(guid_t, listener_guid), chanend c_tx);


unsigned avb_1722_1_acmp_get_talker_connection_info(REFERENCE_PARAM(short,talker));

/** \fn avb_1722_1_acmp_get_listener_connection_info
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_LISTENER and
 * AVB_1722_1_DISCONNECT_LISTENER notifications.
 */
unsigned avb_1722_1_acmp_get_listener_connection_info(REFERENCE_PARAM(short,listener), unsigned char address[6], unsigned streamId[2], REFERENCE_PARAM(unsigned,vlan));

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
void avb_1722_1_talker_set_mac_address(unsigned talker_unique_id, unsigned char macaddr[]);

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
avb_1722_1_acmp_status_t avb_1722_1_acmp_configure_source(unsigned talker_unique_id, unsigned int default_format);

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
avb_1722_1_acmp_status_t avb_1722_1_acmp_configure_sink(unsigned listener_unique_id, unsigned int default_format);

#endif /* AVB_17221_ACMP_H_ */
