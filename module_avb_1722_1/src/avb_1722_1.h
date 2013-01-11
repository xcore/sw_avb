/** \file avb_1722_1.h
 *
 */


#ifndef _avb_1722_1_h_
#define _avb_1722_1_h_

#include "avb_control_types.h"

/** \fn avb_1722_1_init
 *
 *  Initialization
 */
void avb_1722_1_init(unsigned char macaddr[6], unsigned char serial_number[2]);

/** \fn avb_1722_1_periodic
 *
 *  This function performs periodic processing. It must be called frequently
 */
void avb_1722_1_periodic(chanend tx, chanend c_ptp);

/** \fn avb_1722_1_process_packet
 *
 *  Process a received 1722_1 packet
 */
void avb_1722_1_process_packet(unsigned char buf[], unsigned char src_addr[6], int len, chanend c_tx);

#endif





