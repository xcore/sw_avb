/** \file avb_1722_1.h
 *
 */


#ifndef _avb_1722_1_h_
#define _avb_1722_1_h_

#include "xc2compat.h"
#include "avb_control_types.h"
#include "avb_1722_1_adp_pdu.h"
#include "avb_1722_1_acmp_pdu.h"
#include "avb_1722_1_aecp_pdu.h"
#include "avb_1722_1_callbacks.h"
#include "otp_board_info.h"

typedef union {
    avb_1722_1_adp_packet_t adp;
    avb_1722_1_acmp_packet_t acmp;
    avb_1722_1_aecp_packet_t aecp;
} avb_1722_1_packet_t;

#define AVB_1722_1_PACKET_SIZE_WORDS ((sizeof(avb_1722_1_packet_t)+sizeof(ethernet_hdr_t)+3)/4)

/** Initialisation of 1722.1 state machines
 *
 *  \param  macaddr     the Ethernet MAC address (6 bytes) of the endpoint,
                        used to form the 64 bit 1722.1 entity GUID
    \param  serial_num  Device serial number
 */
void avb_1722_1_init(unsigned char macaddr[6], unsigned int serial_num);

#ifdef __XC__
/** This function performs periodic processing for 1722.1 state machines. It must be called frequently.
 *
 *  \param  c_tx        a transmit chanend to the Ethernet server
 *  \param  c_ptp       a chanend to the PTP server
 */
void avb_1722_1_periodic(chanend c_tx, chanend c_ptp, client interface avb_interface avb);

[[combinable]]
void avb_1722_1_task(otp_ports_t &ports,
                     client interface avb_interface i_avb,
                     client interface avb_1722_1_control_callbacks i_1722_1_entity,
                     chanend c_mac_rx,
                     chanend c_mac_tx,
                     chanend c_ptp);
#endif

/** Process a received 1722.1 packet
 *
 *  \param  buf         an array of received packet data to be processed
 *  \param  src_addr    an array of size 6 with the source MAC address of the packet
 *  \param  len         the number of bytes in the buf array
*   \param  c_tx        a transmit chanend to the Ethernet server
 */
#ifdef __XC__
extern "C" {
#endif
void avb_1722_1_process_packet(unsigned char *buf,
                                unsigned char src_addr[6],
                                int len,
                                chanend c_tx,
                                CLIENT_INTERFACE(avb_interface, i_avb_api),
                                CLIENT_INTERFACE(avb_1722_1_control_callbacks, i_1722_1_entity));
#ifdef __XC__
}
#endif

#endif





