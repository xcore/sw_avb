#ifndef AVB_1722_1_COMMON_H_
#define AVB_1722_1_COMMON_H_

#include <xclib.h>
#include "nettypes.h"
#include "ethernet_tx_client.h"
#include "avb_1722_1_protocol.h"
#include "xccompat.h"
#include "xc2compat.h"

void print_guid_ln(const_guid_ref_t g);
void print_mac_ln(unsigned char c[6]);
unsigned compare_guid(unsigned char a[6], const_guid_ref_t b);

int qlog2(unsigned n);


#ifdef __XC__
extern "C" {
#endif
void avb_1722_1_create_1722_1_header(const unsigned char *dest_addr, int subtype, int message_type, unsigned char valid_time_status, unsigned data_len, ethernet_hdr_t *hdr);
#ifdef __XC__
}
#endif


#endif /* AVB_1722_1_COMMON_H_ */
