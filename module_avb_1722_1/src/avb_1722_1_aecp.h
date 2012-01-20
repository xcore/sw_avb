#ifndef AVB_1722_1_AECP_H_
#define AVB_1722_1_AECP_H_

#include <xccompat.h>
#include "avb_1722_1_aecp_pdu.h"
#include "avb_control_types.h"

avb_status_t process_avb_1722_1_aecp_packet(unsigned char dest_addr[6], REFERENCE_PARAM(avb_1722_1_aecp_packet_t, pkt), chanend c_tx);

#endif /* AVB_1722_1_AECP_H_ */
