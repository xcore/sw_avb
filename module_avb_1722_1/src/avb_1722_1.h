/** \file avb_1722_1.h
 *
 */


#ifndef _avb_1722_1_h_
#define _avb_1722_1_h_

#include "avb_control_types.h"

/** Initialisation of 1722.1 state machines
 *
 *  \param  macaddr     the Ethernet MAC address (6 bytes) of the endpoint,
                        used to form the 64 bit 1722.1 entity GUID
 */
void avb_1722_1_init(unsigned char macaddr[6]);

/** This function performs periodic processing for 1722.1 state machines. It must be called frequently.
 *
 *  \param  c_tx        a transmit chanend to the Ethernet server
 *  \param  c_ptp       a chanend to the PTP server
 */
void avb_1722_1_periodic(chanend c_tx, chanend c_ptp);

/** Process a received 1722.1 packet
 *
 *  \param  buf         an array of received packet data to be processed
 *  \param  src_addr    an array of size 6 with the source MAC address of the packet
 *  \param  len         the number of bytes in the buf array
*   \param  c_tx        a transmit chanend to the Ethernet server
 */
void avb_1722_1_process_packet(unsigned char buf[], unsigned char src_addr[6], int len, chanend c_tx);

#endif





