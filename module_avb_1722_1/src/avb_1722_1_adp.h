#ifndef AVB_1722_1_ADP_H_
#define AVB_1722_1_ADP_H_

#include "avb_api.h"
#include "avb_1722_1_adp_pdu.h"
#include "avb_control_types.h"

#define AVB_1722_1_ADP_DEST_MAC {0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00};

void avb_1722_1_adp_init();

void process_avb_1722_1_adp_packet(REFERENCE_PARAM(avb_1722_1_adp_packet_t, pkt), chanend c_tx);
void avb_1722_1_adp_advertising_periodic(chanend c_tx, chanend ptp);
#ifdef __XC__
void avb_1722_1_adp_discovery_periodic(chanend c_tx, client interface avb_interface avb);
#endif
int avb_1722_1_get_latest_new_entity_idx();

/**
 *
 *  Start advertising information about this entity via ADP
 */
void avb_1722_1_adp_announce(void);

/**
 *
 *  Stop advertising information about this entity via ADP
 */
void avb_1722_1_adp_depart(void);


/**
 *
 *  Stop then start advertising information about this entity via ADP
 */
void avb_1722_1_adp_depart_then_announce(void);

/** Ask to discover the information for a specific entity GUID
 *
 * \param   guid    The GUID of the entity to discover
 */
void avb_1722_1_adp_discover(const_guid_ref_t guid);

/**
 *
 *  Ask to discover all available entities via ADP
 */
void avb_1722_1_adp_discover_all(void);

/** Set the current PTP grandmaster used by ADP when it changes
 *
 *  \param grandmaster  a 6 byte array containing the AS Grandmaster ID
 */
void avb_1722_1_adp_change_ptp_grandmaster(unsigned char grandmaster[8]);

/** Find a GUID within the entities list.
 *
 *  \param guid  the GUID to be found
 *
 *  \return      AVB_1722_1_MAX_ENTITIES if not found, otherwise the index of the entity.
 */
int avb_1722_1_entity_database_find(const_guid_ref_t guid);

/** Remove all discovered entities from the database
 *
 */
void avb_1722_1_entity_database_flush(void);


#endif /* AVB_1722_1_ADP_H_ */
