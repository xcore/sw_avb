#ifndef AVB_1722_1_COMMON_H_
#define AVB_1722_1_COMMON_H_

#include <xclib.h>
#include "nettypes.h"
#include "ethernet_tx_client.h"
#include "avb_1722_1_protocol.h"

void set_guid(guid_t *guid, unsigned char *a);
unsigned compare_guid(unsigned char *a, guid_t *b);
int qlog2(unsigned n);
void avb_1722_1_create_1722_1_header(const unsigned char* dest_addr, int subtype, int message_type, unsigned char valid_time_status, unsigned data_len, ethernet_hdr_t *hdr);

#endif /* AVB_1722_1_COMMON_H_ */
