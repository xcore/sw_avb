#ifndef _avb_mmrp_pdu_h_
#define _avb_mmrp_pdu_h_
#include "avb_mrp_pdu.h"

#define AVB_MMRP_ATTRIBUTE_TYPE 0

typedef struct mmrp_first_value {
  unsigned char addr[6];
} mmrp_first_value;


#endif // _avb_mmrp_pdu_h_
