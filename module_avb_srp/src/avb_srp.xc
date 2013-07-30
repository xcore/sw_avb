#include <print.h>
#include "avb.h"
#include "avb_api.h"
#include "avb_mrp.h"
#include "avb_srp.h"
#include "avb_mvrp.h"
#include "ethernet_tx_client.h"
#include "ethernet_rx_client.h"
#include "avb_1722_router.h"
#include "ethernet_server_def.h"
#include "avb_mac_filter.h"

#define PERIODIC_POLL_TIME 5000

[[combinable]]
void avb_srp_task(client interface avb_interface avb,
                     chanend c_mac_rx,
                     chanend c_mac_tx) {
  unsigned periodic_timeout;
  timer tmr;
  unsigned int nbytes;
  unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
  unsigned int port_num;
  unsigned char mac_addr[6];

  srp_store_mac_tx_chanend(c_mac_tx);
  mrp_store_mac_tx_chanend(c_mac_tx);

  mac_get_macaddr(c_mac_tx, mac_addr);
  mrp_init(mac_addr);

  srp_domain_init();

  #ifdef AVB_INCLUDE_MMRP
  avb_mmrp_init();
#endif

#ifndef AVB_EXCLUDE_MVRP
  avb_mvrp_init();
#endif

  mac_initialize_routing_table(c_mac_tx);

  mac_set_custom_filter(c_mac_rx, MAC_FILTER_AVB_SRP);
  mac_request_status_packets(c_mac_rx);

  avb.initialise();

  tmr :> periodic_timeout;

  while (1) {
    select {
      case avb_get_control_packet(c_mac_rx, buf, nbytes, port_num):
      {
        avb_process_control_packet(avb, buf, nbytes, c_mac_tx, port_num);
        break;
      }
      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> unsigned int time_now:
      {
        mrp_periodic(avb);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }
    }
  }
}