#ifndef __AVB_1722_MAAP_H_
#define __AVB_1722_MAAP_H_
#include <xccompat.h>
#include "avb_control_types.h"

#define MAX_AVB_1722_MAAP_PDU_SIZE (64)

/** Request a range of multicast addresses.
 * 
 *  This function requests a range of multicast addresses to use as destination
 *  addresses for IEEE 1722 streams. It starts the reservation process 
 *  according to the 1722 MAAP protocol. If the reservation is successful it 
 *  is reported via the status return value of avb_periodic().
 *
 *  \param num_addresses    number of addresses to try and reserve; 
 *                          will be reserved in a contiguous range
 *  \param start_address    an optional six byte array specifying the required 
 *                          start address of the range; if argument 
 *                          is null then the start address will be picked at
 *                          random                        
 *  
 **/
#ifdef __XC__
void avb_1722_maap_request_addresses(int num_addresses, char ?start_address[]);
#else
void avb_1722_maap_request_addresses(int num_addresses, char start_address[]);
#endif

void avb_1722_maap_init(unsigned char macaddr[6]);

avb_status_t avb_1722_maap_process_packet_(unsigned int buf[], int nbytes, chanend c_tx);

/** Get the base address of the reserved range. 
 *
 *  This function returns the first address of the reserved multicast
 *  address range. 
 * 
 *  \param addr array to be filled with the 6-byte MAC address
 *
 **/
void avb_1722_maap_get_base_address(unsigned char addr[6]);


/** Get the address offset into the reserved range. 
 *
 *  This function returns a specific address within the reserved multicast
 *  address range. 
 * 
 *  \param addr     array to be filled with the 6-byte MAC address
 *  \param offset   the offset into that range required
 *
 **/
void avb_1722_maap_get_offset_address(unsigned char addr[6], int offset);

/** Relinquish the reserved MAAP address range
 *
 *  This function abandons the claim to the reserved address range
 */
void avb_1722_maap_relinquish_addresses();

/** Perform MAAP periodic functions
 *
 *  This function performs the various functions needed for the periodic
 *  operation of the MAAP protocol.  For instance, the periodic transmission
 *  of announcement messages.
 *
 *  This function is called internally by the AVB general periodic function.
 *
 *  \param c_tx    Channel for ethernet transmission
 */
int avb_1722_maap_periodic(chanend c_tx);

/** Re-request a claim on the existing address range
 *
 *  If there is a current address reservation, this will reset the state
 *  machine into the PROBE state, in order to cause the protocol to
 *  re-probe and re-allocate the addresses.
 */
void avb_1722_maap_rerequest_addresses();

#endif
