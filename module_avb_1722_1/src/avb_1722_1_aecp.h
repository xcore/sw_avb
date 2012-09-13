#ifndef AVB_1722_1_AECP_H_
#define AVB_1722_1_AECP_H_

#include <xccompat.h>
#include "avb_1722_1_aecp_pdu.h"
#include "avb_1722_1_aecp_aem.h"
#include "avb_control_types.h"

void avb_1722_1_aecp_aem_init();
void avb_1722_1_aem_set_grandmaster_id(REFERENCE_PARAM(unsigned char, as_grandmaster_id));
void process_avb_1722_1_aecp_packet(unsigned char src_addr[6], REFERENCE_PARAM(avb_1722_1_aecp_packet_t, pkt), int num_packet_bytes, chanend c_tx);

#endif /* AVB_1722_1_AECP_H_ */
