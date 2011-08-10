#ifndef AVB_1722_1_AECP_H_
#define AVB_1722_1_AECP_H_

#include <xccompat.h>
#include "avb_1722_1_aecp_pdu.h"
#include "avb_control_types.h"

#define AVB_1722_1_AECP_DEST_MAC {0x91, 0xe0, 0xf0, 0x00, 0xff, 0x01}

avb_status_t process_avb_1722_1_aecp_packet(REFERENCE_PARAM(avb_1722_1_aecp_packet_t, pkt), chanend c_tx);

unsigned int avb_1722_1_walk_tree(REFERENCE_PARAM(unsigned char, address), unsigned set, REFERENCE_PARAM(unsigned char, data));

#endif /* AVB_1722_1_AECP_H_ */
