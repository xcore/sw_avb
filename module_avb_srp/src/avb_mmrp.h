#ifndef __AVB_MMRP_H__
#define __AVB_MMRP_H__
#include "avb_conf.h"
#include "avb_mrp.h"

/** \file avb_mmrp.h
 *
 *  MMRP is the multicast group join/leave protocol.  It is based on the
 *  generic MRP protocol, and operates by generating MRP attributes describing
 *  multicast join/leave and passing them off to the MRP system.
 */

//! Ethertype for MMRP
#define AVB_MMRP_ETHERTYPE (0x88f6) 

#ifndef AVB_MAX_MMRP_GROUPS
#define AVB_MAX_MMRP_GROUPS (AVB_NUM_SINKS*2)
#endif

//! The multicast MAC address that MVP packets are sent to
#define AVB_MMRP_MACADDR { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x20 }

/** Initialise the MMRP module
 *
 */
void avb_mmrp_init(void);

/** Create a MRP attribute set for Multicast group membership join
 *  \param addr the multicast MAC address
 */
int avb_join_multicast_group(unsigned char addr[6]);

/** Create an MRP attribute for leaving the multicast group
 *  \param addr the multicast group to leave
 */
void avb_leave_multicast_group(unsigned char addr[6]);



#ifndef __XC__



//! Callback from MRP because it is merging several attribute sets into one TX packet
int avb_mmrp_merge_message(char *buf,
                          mrp_attribute_state *st,
                          int vector);

//! Callback from MRP because it is checking if an MMRP attribute matches something we are looking for
int avb_mmrp_match_mac_vector(mrp_attribute_state *attr,
                   char *msg,
                   int i);

//!@{
//! \name Indications from the MRP state machine
void avb_mmrp_mac_vector_join_ind(mrp_attribute_state *attr, int new);
void avb_mmrp_mac_vector_leave_ind(mrp_attribute_state *attr);
//!@}

#endif

#endif
