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

/** \fn avb_1722_1_init
 *
 *  Initialization
 */
void avb_1722_1_init(unsigned char macaddr[6], unsigned char serial_number[2]);

/** \fn avb_1722_1_periodic
 *
 *  This function performs periodic processing. It must be called frequently
 */
void avb_1722_1_periodic(avb_status_t *status, chanend c_tx, chanend c_ptp);

/** \fn avb_1722_1_process_packet
 *
 *  Process a received 1722_1 packet
 */
void avb_1722_1_process_packet(avb_status_t *status, unsigned int buf[], int len, chanend c_tx);

#endif





