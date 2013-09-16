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


#define PERIODIC_POLL_TIME 5000

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

// TODO: Move/rename this task?
[[combinable]]
void avb_1722_1_task(client interface avb_interface i_avb,
                     client interface avb_1722_1_control_callbacks i_1722_1_entity,
                     chanend c_mac_rx,
                     chanend c_mac_tx,
                     chanend c_ptp) {
  unsigned periodic_timeout;
  timer tmr;
  unsigned int nbytes;
  unsigned int buf[AVB_1722_1_PACKET_SIZE_WORDS];
  unsigned int port_num;
  unsigned char mac_addr[6];

  mac_get_macaddr(c_mac_tx, mac_addr);
  avb_1722_1_init(mac_addr);
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
        avb_process_1722_control_packet(buf, nbytes, c_mac_tx, i_avb, i_1722_1_entity);
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