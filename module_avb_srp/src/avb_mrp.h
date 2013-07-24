#ifndef _avb_mrp_h_
#define _avb_mrp_h_

#include "avb_api.h"
#include "avb_conf.h"
#include "xc2compat.h"

#include "avb_mrp_types.h"
#include "avb_stream.h"


/** \file avb_mrp.h
 *
 *  MRP (from 802.1ak) is the Multiple Registration Protocol which
 *  allows units in an 802.1 LAN to register attributes with other
 *  nodes.  It is used, in this case, for registering membership of
 *  VLANs and Multicast groups, and making and withdrawing stream
 *  reservations.
 *
 *  Switches in the LAN will respond to this protocol and update
 *  whatever tables are necessary to ensure that the node receives
 *  the correct traffic.
 */

#define MRP_NUM_PORTS (2)


#ifndef MRP_MAX_ATTRS
#if MRP_NUM_PORTS == 1
// There are 3 attributes per stream (talker_advertise, talker_failed
// and listener). Therefore the number of attributes needed is:
// (nTalkers * 3) + (nListeners * 3) + (nDomains=1) + AVB_MAX_NUM_VLAN + AVB_MAX_MMRP_GROUPS
#define MRP_MAX_ATTRS ((3*(AVB_NUM_SOURCES)) + (3*(AVB_NUM_SINKS)) + 1 + (AVB_MAX_NUM_VLAN))
#else
#define MRP_MAX_ATTRS 15
#endif
#endif

#define MRP_DEBUG_ATTR_EGRESS 0
#define MRP_DEBUG_ATTR_INGRESS 0

#define FIRST_VALUE_LENGTHS \
  { sizeof(srp_talker_first_value),             \
    sizeof(srp_talker_failed_first_value),\
    sizeof(srp_listener_first_value),\
    sizeof(srp_domain_first_value),\
    sizeof(mmrp_mac_vector_first_value),\
    sizeof(mvrp_vid_vector_first_value)\
  }

#define PENDING_JOIN_NEW 0x01
#define PENDING_JOIN     0x02
#define PENDING_LEAVE    0x04


#define MRP_JOINTIMER_PERIOD_CENTISECONDS 20

#define MRP_LEAVETIMER_PERIOD_CENTISECONDS 80

#define MRP_LEAVEALL_TIMER_PERIOD_CENTISECONDS 1000
#define MRP_LEAVEALL_TIMER_MULTIPLIER 100

#define MRP_PERIODIC_TIMER_PERIOD_CENTISECONDS 100
#define MRP_PERIODIC_TIMER_MULTIPLIER 10

void mrp_debug_dump_attrs(void);

/** Function: mrp_init

   This function initializes the MRP state machines. It just 
   needs to be called once at the start of the program.
*/      
void mrp_init(char macaddr[]);

/** Function: mrp_attribute_init

   This function initializes the state of an MRP attribute.
  
   \param st the attribute state structure to intialize
   \param t the type of the attribute
   \param port_num the id number of the Ethernet port to associate the attribute with
   \param info a void * pointer that will be associated with the attribute.
               This pointer is passed on to attribute specific functions
               (e.g. functions to send particulars PDUs)

   \note: see also mrp_update_state
*/
void mrp_attribute_init(mrp_attribute_state *unsafe st,
                        mrp_attribute_type t,
                        unsigned int port_num,
                        unsigned int here,
                        void *unsafe info);


void mrp_attribute_init_source_info(mrp_attribute_state *unsafe st,
                        mrp_attribute_type t,
                        unsigned int port_num,
                        unsigned int here,
                        avb_source_info_t *unsafe source);

void mrp_attribute_init_sink_info(mrp_attribute_state *unsafe st,
                        mrp_attribute_type t,
                        unsigned int port_num,
                        unsigned int here,
                        avb_sink_info_t *unsafe sink);

void mrp_attribute_init_null(mrp_attribute_state *unsafe st,
                        mrp_attribute_type t,
                        unsigned int port_num,
                        unsigned int here);

/** Function: mrp_mad_begin

   Move the state machines for the attribute into the starting state
   from the initial (unused) state. The Begin! transition is made from
   the state table and descriptions in IEEE802.1ak 10.7.

   \param st the attribute
   
*/
void mrp_mad_begin(mrp_attribute_state *unsafe st);


/** Function: mrp_mad_join

   Issue a MAD_Join.request service primitive for the attribute. The MRP state
   transitions occur as if either the New! or Join! events have occurred (see
   IEEE802.1ak 10.7)

   \param st the attribute to join
   \param new whether the attribute is a new one or not

*/
void mrp_mad_join(mrp_attribute_state *unsafe st, int new);

/** Function: mrp_mad_leave

   This function registers a MAD_Leave request for a particular attribute. The
   state machines transition as if the Leave! event has occurred (see IEEE802.1ak 10.7)

   \param st The attribute to leave
   
*/
void mrp_mad_leave(mrp_attribute_state *unsafe st);

/** Function: mrp_periodic

   This function performs periodic MRP processing. It must be called 
   approximately 4 times a second.

   See also:
   
       mrp_init
 */
void mrp_periodic(CLIENT_INTERFACE(avb_interface, avb));

mrp_attribute_state *unsafe mrp_match_attr_by_stream_and_type(mrp_attribute_state *unsafe attr, int opposite_port);
int mrp_match_multiple_attrs_by_stream_and_type(mrp_attribute_state *unsafe attr, int opposite_port);
mrp_attribute_state *unsafe mrp_match_attribute_by_stream_id(mrp_attribute_state *unsafe attr, int opposite_port);

int mrp_is_observer(mrp_attribute_state *unsafe st);


void mrp_encode_three_packed_event(char *unsafe buf,
                                   int event,
                                   mrp_attribute_type attr);

void mrp_encode_four_packed_event(char *unsafe buf,
                                  int event,
                                  mrp_attribute_type attr);

mrp_attribute_state *unsafe mrp_get_attr(void);

void avb_mrp_process_packet(unsigned char *unsafe buf, int etype, int len, unsigned int port_num);

void avb_mrp_set_legacy_mode(int mode);

#endif  //_avb_mrp_h_
