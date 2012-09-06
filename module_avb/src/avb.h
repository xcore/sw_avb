#ifndef _avb_h_
#define _avb_h_

#include "ethernet_server.h"
#include "ethernet_rx_client.h"
#include "ethernet_tx_client.h"
#include "avb_1722.h"
#include "avb_1722_router.h"
#include "avb_1722_maap.h"
#include "avb_srp.h"
#include "gptp.h"
#include "getmac.h"
#include "media_clock_server.h"
#include "media_clock_client.h"
#include "avb_control_types.h"
#include "avb_stream_detect.h"
#include "avb_api.h"
#include "avb_unit.h"

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
void avb_init(chanend media_ctl[],
              chanend ?listener_ctl[],
              chanend ?talker_ctl[],
              chanend ?media_clock_ctl,
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


/** Start any AVB protocol state machines.
 * 
 *  This call starts any AVB protocol state machines running. It should be
 *  called after the ethernet link goes up.
 **/
void avb_start(void);

/** Perform AVB periodic processing.
 *
 *  This function performs AVB periodic processing. It should be called 
 *  from the main control thread at least once each ms. If it returns
 *  a state other than AVB_NO_STATUS then it should be called again
 *  immediately.
 * 
 *  \returns A status update from the periodic processing to indicate
 *           an event due to a timeout etc. (see :c:type:`avb_status_t`)
 **/
void avb_periodic(REFERENCE_PARAM(avb_status_t, status));

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
                            REFERENCE_PARAM(unsigned int, nbytes));



/** Process an AVB control packet.

   This function processes an ethernet packet and if it is a 802.1Qat or
   IEEE 1722 MAAP packet will handle it.
  
   This function should always be called on the buffer filled by 
   avb_get_control_packet(). 

   \param buf the incoming message buffer
   \param len the length (in bytes) of the incoming buffer
   \param c_tx           chanend connected to the ethernet mac (TX)
          
   \returns ``AVB_SRP_TALKER_ROUTE_FAILED`` if the incoming packet reports a 
            talker routing failure (i.e. a failure in getting an outgoing stream
            to its intended listener). ``AVB_SRP_LISTENER_ROUTE_FAILED``
            if the incoming packet reports a
            listener routing failure (i.e. a failure in listening to a stream).
            If the packet causes a previously asked for, or 
            reserved, multicast address range to be no longer available,
           then ``AVB_MAAP_ADDRESSES_LOST`` is returned.
 **/
void avb_process_control_packet(REFERENCE_PARAM(avb_status_t, status),
                               unsigned int buf[], int len,
                               chanend c_tx);

/** Set the endpoint into "legacy mode".
 *
 *  This function sets the endpoint into "legacy mode" to work with 
 *  non-AVB switches for testing or demonstration purposes. In this
 *  mode the destination address of certain protocols change to become 
 *  legacy (non-AVB) traffic  and the PTP 802.1as protocol behaves in a
 *  slightly different manner. In this case:
 * 
 *    * The protocols are non-longer AVB standard so will not work with
 *      any other AVB hardware. They will only work with other endpoints 
 *      set to this mode.
 *
 *    * There is no longer any quality of service guarantee so audio        
 *      may be disrupted. Furthermore the traffic from the endpoint may
 *      disrupt other non-AVB ethernet devices on the network.
 *
 *  \param mode    non-zero to set legacy mode, zero to unset it
 *
 **/
void avb_set_legacy_mode(int mode);

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


#ifndef __XC__

/** Utility function to get the index of a source stream based on its
 * pointer.  This is used by SRP, which stores a pointer to the stream
 * structure rather than an index.
 */
unsigned avb_get_source_stream_index_from_pointer(void* ptr);

/** Utility function to get the index of a sink stream based on its
 * pointer.  This is used by SRP, which stores a pointer to the stream
 * structure rather than an index.
 */
unsigned avb_get_sink_stream_index_from_pointer(void* ptr);

#endif

#endif // _avb_h_
