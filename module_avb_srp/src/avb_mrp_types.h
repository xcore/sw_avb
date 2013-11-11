#ifndef __avb_mrp_types_h__
#define __avb_mrp_types_h__

#include "misc_timer.h"
#include "xc2compat.h"

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
  MRP_EVENT_LEAVETIMER,
#endif
  MRP_EVENT_DUMMY
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

typedef enum {
  MRP_ATTRIBUTE_EVENT_NEW = 0,
  MRP_ATTRIBUTE_EVENT_JOININ = 1,
  MRP_ATTRIBUTE_EVENT_IN = 2,
  MRP_ATTRIBUTE_EVENT_JOINMT = 3,
  MRP_ATTRIBUTE_EVENT_MT = 4,
  MRP_ATTRIBUTE_EVENT_LV = 5
} mrp_attribute_event;

#ifdef __XC__
extern "C" {
#endif
typedef struct mrp_attribute_state {
  unsigned int port_num;
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

  char here;
  char propagated;

  //! Generic pointer to allow random data to be stored alongside the attribute
  void *attribute_info;
} mrp_attribute_state;
#ifdef __XC__
}
#endif

#endif
