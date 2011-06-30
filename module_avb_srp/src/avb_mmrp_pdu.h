#ifndef _avb_mmrp_pdu_h_
#define _avb_mmrp_pdu_h_
#include "avb_mrp_pdu.h"

// We don't use the Service Requirement type
#define AVB_MMRP_SERVICE_REQUIREMENT_ATTRIBUTE_TYPE 0 // should be 1 but leave until AttributeLength field is correct
#define AVB_MMRP_MAC_VECTOR_ATTRIBUTE_TYPE 0 // should be 2 but leave until AttributeLength field is correct

typedef struct {
  unsigned char addr[6];
} mmrp_mac_vector_first_value;

typedef struct {
  unsigned char addr[6];
} mmrp_service_requirement_first_value;


#endif // _avb_mmrp_pdu_h_
