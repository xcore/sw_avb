#ifndef _avb_h_
#define _avb_h_

#include <xccompat.h>
#include "xc2compat.h"
#include "avb_api.h"


#ifndef MAX_AVB_CONTROL_PACKET_SIZE
#define MAX_AVB_CONTROL_PACKET_SIZE (1518)
#endif


/** Initialize the AVB control thread.
 * 
 *  This function initializes the AVB system. It needs to be called in
 *  the main user control thread before any other AVB control call.
 *  The function takes chanends connected to other parts of the system and
 *  registers all of these components. 
 *
 *  At this point the sinks, sources and media FIFOs are allocated numbers. 
 *  The allocation
 *  is performed by registering numbers from 0 upwards working through the
 *  listener_ctl/talker_ctl/media_ctl arrays. 
 *  Each component in this array may register
 *  several sink/sources/FIFOs. For example, if the listener_ctl array connects
 *  to two listener units each registering 3 sinks then the first unit will 
 *  be allocated sink numbers 0,1,2 and the second 3,4,5.
 *
 *  Note that this call does not start any protocols communicating over the 
 *  network (e.g. advertising talkers via IEEE 802.1Qat). That is deferred 
 *  until the call to avb_start().
 *
 *  \param media_ctl    array of chanends connected to components that
 *                      register/control media FIFOs
 *  \param listener_ctl array of chanends connected to components 
 *                      that register/control IEEE 1722 sinks
 *  \param talker_ctl   array of chanends connected to components that
 *                      register/control IEEE 1722 sources
 *  \param media_clock_ctl chanend connected to the media clock server
 *  \param c_mac_rx        chanend connected to the ethernet server (RX)
 *  \param c_mac_tx        chanend connected to the ethernet server (TX)
 *  \param c_ptp           chanend connected to the ptp server
 *
 **/
#ifdef __XC__
unsafe void avb_init(chanend c_media_ctl[],
              chanend ?c_listener_ctl[],
              chanend ?c_talker_ctl[],
              chanend ?c_media_clock_ctl,
              chanend c_mac_rx,
              chanend c_mac_tx,
              chanend c_ptp);
#else
void avb_init(chanend media_ctl[],
              chanend listener_ctl[],
              chanend talker_ctl[],
              chanend media_clock_ctl,
              chanend c_mac_rx,
              chanend c_mac_tx,
              chanend c_ptp);
#endif

void avb_init_srp_only(chanend c_mac_rx, chanend c_mac_tx);


/** Start any AVB protocol state machines.
 * 
 *  This call starts any AVB protocol state machines running. It should be
 *  called after the ethernet link goes up.
 **/
void avb_start(void);

/** Perform AVB periodic processing.
 *
 *  This function performs AVB periodic processing. It should be called 
 *  from the main control thread at least once each ms.
 * 
 **/
#ifdef __XC__
void avb_periodic(chanend c_mac_tx, unsigned int time_now);

[[combinable]]
void avb_manager(server interface avb_interface avb[2],
                 chanend c_media_ctl[],
                 chanend ?c_listener_ctl[],
                 chanend ?c_talker_ctl[],
                 chanend ?c_media_clock_ctl,
                 chanend c_mac_rx,
                 chanend c_mac_tx,
                 chanend c_ptp);
#endif

/** Receives an 802.1Qat SRP packet or an IEEE P1722 MAAP packet.
 *
 *  This function receives an AVB control packet from the ethernet MAC.
 *  It is selectable so can be used in a select statement as a case.
 * 
 *  \param c_rx     chanend connected to the ethernet component
 *  \param buf      buffer to retrieve the packet into; buffer
 *                  must have length at least ``MAX_AVB_CONTROL_PACKET_SZIE``
 *                  bytes
 *  \param nbytes   a reference parameter that is filled with the length
 *                  of the received packet
 **/
#ifdef __XC__
#pragma select handler
#endif
void avb_get_control_packet(chanend c_rx,
                            unsigned int buf[],
                            REFERENCE_PARAM(unsigned int, nbytes),
                            REFERENCE_PARAM(unsigned int, port_num));



void avb_process_1722_control_packet(unsigned int buf0[], int nbytes, chanend c_tx);

/** Process an AVB control packet.

   This function processes an ethernet packet and if it is a 802.1Qat or
   IEEE 1722 MAAP packet will handle it.
  
   This function should always be called on the buffer filled by 
   avb_get_control_packet(). 

   \param buf the incoming message buffer
   \param len the length (in bytes) of the incoming buffer
   \param c_tx           chanend connected to the ethernet mac (TX)
   \param port_num the id of the Ethernet interface the packet was received
          
 **/
void avb_process_control_packet(unsigned int buf[], int len,
                               chanend c_tx,
                               NULLABLE_RESOURCE(chanend, media_clock_ctl),
                               unsigned int port_num);


/**
 *   \brief Set the volume multipliers for the audio channels
 *
 *   The number of channels in the array should be equal to the number
 *   of channels set in the set_avb_source_map function call.
 *
 *   This function adjusts the stream channels while the stream is
 *   active, and therefore cannot be called while the stream is
 *   inactive.
 *
 *   \param sink_num the stream number to apply the change to
 *   \param volumes a set of volume values in 2.30 signed fixed point linear format
 *   \param count the number of channels to set
 *
 */
void set_avb_source_volumes(int sink_num, int volumes[], int count);


#endif // _avb_h_
