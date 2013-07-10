#ifndef __avb_c_support_h__
#define __avb_c_support_h__

#include "xc2compat.h"

void avb_get_packet_overview(unsigned int buf[], int *etype, int *eth_hdr_size, unsigned char src_addr[6]);

#endif