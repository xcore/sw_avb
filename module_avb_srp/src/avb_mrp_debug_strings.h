#ifndef _avb_mrp_debug_strings_h_
#define _avb_mrp_debug_strings_h_

char *debug_mrp_applicant_state[] = {
  "MRP_UNUSED",
  "MRP_DISABLED",
  "MRP_VO", // Very anxious Observer
  "MRP_VP", // Very anxious Passive
  "MRP_VN", // Very anxious New
  "MRP_AN", // Anxious New
  "MRP_AA", // Anxious Active
  "MRP_QA", // Quiet Active
  "MRP_LA", // Leaving Active
  "MRP_AO", // Anxious Observer
  "MRP_QO", // Quiet Observer
  "MRP_AP", // Anxious Passive
  "MRP_QP", // Quiet Passive
  "MRP_LO" // Leaving Observer
};

char *debug_mrp_registrar_state[] = {
  "MRP_IN",
  "MRP_LV",
  "MRP_MT"
};

char *debug_mrp_event[] = {
  "MRP_EVENT_BEGIN",
  "MRP_EVENT_NEW",
  "MRP_EVENT_FLUSH",
  "MRP_EVENT_JOIN",
  "MRP_EVENT_LV",
  "MRP_EVENT_RECEIVE_NEW",
  "MRP_EVENT_RECEIVE_JOININ",
  "MRP_EVENT_RECEIVE_IN",
  "MRP_EVENT_RECEIVE_JOINMT",
  "MRP_EVENT_RECEIVE_MT",
  "MRP_EVENT_RECEIVE_LEAVE",
  "MRP_EVENT_RECEIVE_LEAVE_ALL",
  "MRP_EVENT_REDECLARE",
  "MRP_EVENT_PERIODIC",
  "MRP_EVENT_TX",
  "MRP_EVENT_TX_LEAVE_ALL",
  "MRP_EVENT_TX_LEAVE_ALL_FULL",
  "MRP_EVENT_LEAVETIMER",
  "MRP_EVENT_DUMMY"
};

char *debug_attribute_type[] = {
  "MSRP_TALKER_ADVERTISE",
  "MSRP_TALKER_FAILED",
  "MSRP_LISTENER",
  "MSRP_DOMAIN_VECTOR",
  "MMRP_MAC_VECTOR",
  "MVRP_VID_VECTOR",
  "MRP_NUM_ATTRIBUTE_TYPES"
};

char *debug_attribute_event[] = {
  "MRP_ATTRIBUTE_EVENT_NEW",
  "MRP_ATTRIBUTE_EVENT_JOININ",
  "MRP_ATTRIBUTE_EVENT_IN",
  "MRP_ATTRIBUTE_EVENT_JOINMT",
  "MRP_ATTRIBUTE_EVENT_MT",
  "MRP_ATTRIBUTE_EVENT_LV"
};

#endif
