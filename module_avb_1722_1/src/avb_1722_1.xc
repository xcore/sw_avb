#include <print.h>
#include <string.h>
#include "avb.h"
#include "avb_api.h"
#include "avb_1722_common.h"
#include "avb_1722_1.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_adp.h"
#include "avb_1722_1_acmp.h"
#include "avb_1722_1_aecp.h"
#include "avb_1722_maap.h"
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"
#include "ethernet_server_def.h"
#include "avb_mac_filter.h"
#include "spi.h"
#include "avb_1722_1_protocol.h"


#define PERIODIC_POLL_TIME 5000

unsigned char my_mac_addr[6];

// Buffer for constructing 1722.1 transmit packets
unsigned int avb_1722_1_buf[AVB_1722_1_PACKET_SIZE_WORDS];

// The GUID of this device
guid_t my_guid;

__attribute__((overlay))
void avb_1722_1_init(unsigned char macaddr[6], unsigned serial_num)
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
    avb_1722_1_aecp_aem_init(serial_num);
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

void avb_1722_1_process_packet(unsigned char buf[len], unsigned len,
                                unsigned char src_addr[6],
                                chanend c_tx,
                                CLIENT_INTERFACE(avb_interface, i_avb_api),
                                CLIENT_INTERFACE(avb_1722_1_control_callbacks, i_1722_1_entity),
                                CLIENT_INTERFACE(spi_interface, i_spi))
{
    avb_1722_1_packet_header_t *pkt = (avb_1722_1_packet_header_t *) &buf[0];
    unsigned subtype = GET_1722_1_SUBTYPE(pkt);

    switch (subtype)
    {
    case DEFAULT_1722_1_ADP_SUBTYPE:
        process_avb_1722_1_adp_packet(*(avb_1722_1_adp_packet_t*)pkt, c_tx);
        return;
    case DEFAULT_1722_1_AECP_SUBTYPE:
        process_avb_1722_1_aecp_packet(src_addr, (avb_1722_1_aecp_packet_t*)pkt, len, c_tx, i_avb_api, i_1722_1_entity, i_spi);
        return;
    case DEFAULT_1722_1_ACMP_SUBTYPE:
        process_avb_1722_1_acmp_packet((avb_1722_1_acmp_packet_t*)pkt, c_tx);
        return;
    default:
        return;
    }
}

void avb_1722_1_periodic(chanend c_tx, chanend c_ptp, client interface avb_interface i_avb)
{
    avb_1722_1_adp_advertising_periodic(c_tx, c_ptp);
    avb_1722_1_adp_discovery_periodic(c_tx, i_avb);
#if (AVB_1722_1_CONTROLLER_ENABLED)
    avb_1722_1_acmp_controller_periodic(c_tx, i_avb);
#endif
#if (AVB_1722_1_TALKER_ENABLED)
    avb_1722_1_acmp_talker_periodic(c_tx, i_avb);
#endif
#if (AVB_1722_1_LISTENER_ENABLED)
    avb_1722_1_acmp_listener_periodic(c_tx, i_avb);
#endif
    avb_1722_1_aecp_aem_periodic(c_tx);
}



[[combinable]]
void avb_1722_1_task(otp_ports_t &otp_ports,
                     client interface avb_interface i_avb,
                     client interface avb_1722_1_control_callbacks i_1722_1_entity,
                     client interface spi_interface i_spi,
                     chanend c_mac_rx,
                     chanend c_mac_tx,
                     chanend c_ptp) {
  unsigned periodic_timeout;
  timer tmr;
  unsigned int nbytes;
  unsigned int buf[AVB_1722_1_PACKET_SIZE_WORDS];
  unsigned int port_num;
  unsigned char mac_addr[6];
  unsigned int serial;

  otp_board_info_get_serial(otp_ports, serial);

  mac_get_macaddr(c_mac_tx, mac_addr);
  avb_1722_1_init(mac_addr, serial);
  avb_1722_maap_init(mac_addr);

  mac_set_custom_filter(c_mac_rx, MAC_FILTER_AVB_CONTROL);
  mac_request_status_packets(c_mac_rx);

  avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);

  tmr :> periodic_timeout;

  while (1) {
    select {
      // Receive and process any incoming AVB packets (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_mac_rx, buf, nbytes, port_num):
      {
        avb_process_1722_control_packet(buf, nbytes, c_mac_tx, i_avb, i_1722_1_entity, i_spi);
        break;
      }
      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> unsigned int time_now:
      {
        avb_1722_1_periodic(c_mac_tx, c_ptp, i_avb);
        avb_1722_maap_periodic(c_mac_tx, i_avb);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }
    }
  }
}
