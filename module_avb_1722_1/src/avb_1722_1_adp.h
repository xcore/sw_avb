#ifndef AVB_1722_1_ADP_H_
#define AVB_1722_1_ADP_H_

#include "avb_1722_1_adp_pdu.h"
#include "avb_control_types.h"

#define AVB_1722_1_ADP_DEST_MAC {0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00};

void avb_1722_1_adp_init();

avb_status_t process_avb_1722_1_adp_packet(REFERENCE_PARAM(avb_1722_1_adp_packet_t, pkt), chanend c_tx);
avb_status_t avb_1722_1_adp_advertising_periodic(chanend c_tx, chanend ptp);
avb_status_t avb_1722_1_adp_discovery_periodic(chanend c_tx);
int avb_1722_1_get_latest_new_entity_idx();

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
void avb_1722_1_adp_discover(REFERENCE_PARAM(guid_t, guid));

/** \fn avb_1722_1_adp_discover_all
 *
 *  Ask to discover all available entities
 */
void avb_1722_1_adp_discover_all();

/** \fn avb_1722_1_adp_change_ptp_grandmaster
 *
 *  Set the current PTP grandmaster used by ADP when it changes
 */
void avb_1722_1_adp_change_ptp_grandmaster(unsigned char grandmaster[8]);

/** \fn avb_1722_1_acmp_get_talker_connection_info
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_TALKER and
 * AVB_1722_1_DISCONNECT_TALKER notifications.
 */


#endif /* AVB_1722_1_ADP_H_ */
