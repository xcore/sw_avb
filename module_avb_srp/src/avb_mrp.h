
#ifndef _avb_mrp_h_
#define _avb_mrp_h_
#include "avb_control_types.h"
#ifdef __mrp_conf_h_exist__
#include "mrp_conf.h"
#endif

#include "misc_timer.h"

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

#ifndef MRP_MAX_ATTRS
// There are 3 attributes per stream (talker_advertise, talker_failed
// and listener). Therefore the number of attributes needed is:
// (nTalkers * 3) + (nListeners * 3) + (nDomains=1) + AVB_MAX_NUM_VLAN + AVB_MAX_MMRP_GROUPS
#define MRP_MAX_ATTRS 20
#endif

#define MRP_FULL_PARTICIPANT 1


typedef enum {
  MRP_UNUSED,
  MRP_DISABLED,  
  MRP_VO, // Very anxious Observer
  MRP_VP, // Very anxious Passive
  MRP_VN, // Very anxious New
  MRP_AN, // Anxious New
  MRP_AA, // Anxious Active
  MRP_QA, // Quiet Active
  MRP_LA, // Leaving Active
  MRP_AO, // Anxious Observer
  MRP_QO, // Quiet Observer
  MRP_AP, // Anxious Passive
  MRP_QP, // Quiet Passive
#ifdef MRP_FULL_PARTICIPANT
  MRP_LO, // Leaving Observer
#endif
} mrp_applicant_state;

typedef enum {
  MRP_IN,
  MRP_LV,
  MRP_MT
} mrp_registrar_state;

typedef enum {
  MRP_EVENT_BEGIN,
  MRP_EVENT_NEW,
  MRP_EVENT_FLUSH,
  MRP_EVENT_JOIN,
  MRP_EVENT_LV,
  MRP_EVENT_RECEIVE_NEW,
  MRP_EVENT_RECEIVE_JOININ,
  MRP_EVENT_RECEIVE_IN,
  MRP_EVENT_RECEIVE_JOINMT,
  MRP_EVENT_RECEIVE_MT,
  MRP_EVENT_RECEIVE_LEAVE,
  MRP_EVENT_RECEIVE_LEAVE_ALL,
  MRP_EVENT_REDECLARE,
  MRP_EVENT_PERIODIC,
  MRP_EVENT_TX,
#ifdef MRP_FULL_PARTICIPANT
  MRP_EVENT_TX_LEAVE_ALL,
  MRP_EVENT_TX_LEAVE_ALL_FULL,
  MRP_EVENT_LEAVETIMER
#endif
} mrp_event;


typedef enum {
  MSRP_TALKER_ADVERTISE,
  MSRP_TALKER_FAILED,
  MSRP_LISTENER,
  MSRP_DOMAIN_VECTOR,
  MMRP_MAC_VECTOR,
  MVRP_VID_VECTOR,
  MRP_NUM_ATTRIBUTE_TYPES
} mrp_attribute_type;

#define FIRST_VALUE_LENGTHS \
  { sizeof(srp_talker_first_value),             \
    sizeof(srp_talker_failed_first_value),\
    sizeof(srp_listener_first_value),\
    sizeof(srp_domain_first_value),\
    sizeof(mmrp_mac_vector_first_value),\
    sizeof(mvrp_vid_vector_first_value)\
  }






typedef enum {
  MRP_ATTRIBUTE_EVENT_NEW = 0,
  MRP_ATTRIBUTE_EVENT_JOININ = 1,
  MRP_ATTRIBUTE_EVENT_IN = 2,
  MRP_ATTRIBUTE_EVENT_JOINMT = 3,
  MRP_ATTRIBUTE_EVENT_MT = 4,
  MRP_ATTRIBUTE_EVENT_LV = 5
} mrp_attribute_event;


#ifndef __XC__

#define PENDING_JOIN_NEW 0x01
#define PENDING_JOIN     0x02
#define PENDING_LEAVE    0x04


typedef struct mrp_attribute_state {
  unsigned char attribute_type;
  unsigned char applicant_state;

#ifdef MRP_FULL_PARTICIPANT
  unsigned char registrar_state;
  avb_timer leaveTimer;
#endif

  //! used to note indications that have been detected by the state machine but not
  //! indicated to the application yet
  short pending_indications;

  //! if there is a pending indication and it had an associated four vector parameter
  //! then the parameter is stored here
  short four_vector_parameter;

  //! While sorting the attributes, this contains a linked list of sorted attributes
  struct mrp_attribute_state *next;

  //! Generic pointer to allow random data to be stored alongside the attribute
  void *attribute_info;
} mrp_attribute_state;

#define MRP_JOINTIMER_PERIOD_CENTISECONDS 20

#define MRP_LEAVETIMER_PERIOD_CENTISECONDS 80

#define MRP_LEAVEALL_TIMER_PERIOD_CENTISECONDS 1000
#define MRP_LEAVEALL_TIMER_MULTIPLIER 100

#define MRP_PERIODIC_TIMER_PERIOD_CENTISECONDS 100
#define MRP_PERIODIC_TIMER_MULTIPLIER 10

/** Function: mrp_init

   This function initializes the MRP state machines. It just 
   needs to be called once at the start of the program.
*/      
void mrp_init(char macaddr[]);

/** Function: mrp_attribute_init

   This function initializes the state of an MRP attribute.

   \param st the attribute state structure to intialize
   \param t the type of the attribute
   \param info a void * pointer that will be associated with the attribute.
               This pointer is passed on to attribute specific functions
               (e.g. functions to send particulars PDUs)

   \note: see also mrp_update_state
*/
void mrp_attribute_init(mrp_attribute_state *st,
                        mrp_attribute_type t,
                        void *info);



/** Function: mrp_mad_begin

   Move the state machines for the attribute into the starting state
   from the initial (unused) state. The Begin! transition is made from
   the state table and descriptions in IEEE802.1ak 10.7.

   \param st the attribute
   
*/
void mrp_mad_begin(mrp_attribute_state *st);


/** Function: mrp_mad_join

   Issue a MAD_Join.request service primitive for the attribute. The MRP state
   transitions occur as if either the New! or Join! events have occurred (see
   IEEE802.1ak 10.7)

   \param st the attribute to join
   \param new whether the attribute is a new one or not

*/
void mrp_mad_join(mrp_attribute_state *st, int new);

/** Function: mrp_mad_leave

   This function registers a MAD_Leave request for a particular attribute. The
   state machines transition as if the Leave! event has occurred (see IEEE802.1ak 10.7)

   \param st The attribute to leave
   
*/
void mrp_mad_leave(mrp_attribute_state *st);

/** Function: mrp_periodic

   This function performs periodic MRP processing. It must be called 
   approximately 4 times a second.

   See also:
   
       mrp_init
 */
void mrp_periodic(avb_status_t *status);

int mrp_is_observer(mrp_attribute_state *st);


void mrp_encode_three_packed_event(char *buf,
                                   int event,
                                   mrp_attribute_type attr);

void mrp_encode_four_packed_event(char *buf,
                                  int event,
                                  mrp_attribute_type attr);

mrp_attribute_state *mrp_get_attr(void);
#endif

void avb_mrp_process_packet(REFERENCE_PARAM(avb_status_t, status), unsigned char buf[], int etype, int len);

void avb_mrp_set_legacy_mode(int mode);

#endif  //_avb_mrp_h_
