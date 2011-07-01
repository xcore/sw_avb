#ifndef _avb_mvrp_pdu_h_
#define _avb_mvrp_pdu_h_
#include "avb_mrp_pdu.h"
#define AVB_MVRP_VID_VECTOR_ATTRIBUTE_TYPE 1

typedef struct {
  unsigned char vlan[2];
} mvrp_vid_vector_first_value;


#endif // _avb_mvrp_pdu_h_
