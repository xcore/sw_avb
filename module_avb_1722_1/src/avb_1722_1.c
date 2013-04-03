#include <print.h>
#include <string.h>
#include <xccompat.h>
#include "avb_1722_common.h"
#include "avb_1722_1.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_adp.h"
#include "avb_1722_1_acmp.h"
#include "avb_1722_1_aecp.h"

typedef union {
    avb_1722_1_adp_packet_t adp;
    avb_1722_1_acmp_packet_t acmp;
    avb_1722_1_aecp_packet_t aecp;
} avb_1722_1_packet_t;

unsigned char my_mac_addr[6];

// Buffer for constructing 1722.1 transmit packets
unsigned int avb_1722_1_buf[(sizeof(avb_1722_1_packet_t)+sizeof(ethernet_hdr_t)+3)/4];

// The GUID of this device
guid_t my_guid;

void avb_1722_1_init(unsigned char macaddr[6])
{
    memcpy(my_mac_addr, macaddr, 6);

    my_guid.c[0] = macaddr[5];
    my_guid.c[1] =  macaddr[4];
    my_guid.c[2] = macaddr[3];
    my_guid.c[3] = 0xfe;
    my_guid.c[4] = 0xff;
    my_guid.c[5] = macaddr[2];
    my_guid.c[6] = macaddr[1];
    my_guid.c[7] = macaddr[0];

    avb_1722_1_adp_init();
#if (AVB_1722_1_AEM_ENABLED)
    avb_1722_1_aecp_aem_init();
#endif

#if (AVB_1722_1_CONTROLLER_ENABLED)
    avb_1722_1_acmp_controller_init();
#endif
#if (AVB_1722_1_TALKER_ENABLED)
    // Talker state machine is initialised once MAAP has finished
#endif
#if (AVB_1722_1_LISTENER_ENABLED)
    avb_1722_1_acmp_listener_init();
#endif

}

void avb_1722_1_process_packet(unsigned char buf[], unsigned char src_addr[6], int len, chanend c_tx)
{
    struct avb_1722_1_packet_header_t *pkt = (struct avb_1722_1_packet_header_t *) &buf[0];
    unsigned subtype = GET_1722_1_SUBTYPE(pkt);

    switch (subtype)
    {
    case DEFAULT_1722_1_ADP_SUBTYPE:
        process_avb_1722_1_adp_packet((avb_1722_1_adp_packet_t*)pkt, c_tx);
        return;
    case DEFAULT_1722_1_AECP_SUBTYPE:
        process_avb_1722_1_aecp_packet(src_addr, (avb_1722_1_aecp_packet_t*)pkt, len, c_tx);
        return;
    case DEFAULT_1722_1_ACMP_SUBTYPE:
        process_avb_1722_1_acmp_packet((avb_1722_1_acmp_packet_t*)pkt, c_tx);
        return;
    default:
        return;
    }
}

void avb_1722_1_periodic(chanend c_tx, chanend c_ptp)
{
	avb_1722_1_adp_advertising_periodic(c_tx, c_ptp);
    avb_1722_1_adp_discovery_periodic(c_tx);
#if (AVB_1722_1_CONTROLLER_ENABLED)
	avb_1722_1_acmp_controller_periodic(c_tx);
#endif
#if (AVB_1722_1_TALKER_ENABLED)
	avb_1722_1_acmp_talker_periodic(c_tx);
#endif
#if (AVB_1722_1_LISTENER_ENABLED)
	avb_1722_1_acmp_listener_periodic(c_tx);
#endif
}

