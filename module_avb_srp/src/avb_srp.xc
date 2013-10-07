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
#include "avb_srp_interface.h"

#define PERIODIC_POLL_TIME 5000

[[combinable]]
void avb_srp_task(client interface avb_interface i_avb,
                  server interface srp_interface i_srp,
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

  i_avb.initialise();

  tmr :> periodic_timeout;

  while (1) {
    select {
      case avb_get_control_packet(c_mac_rx, buf, nbytes, port_num):
      {
        avb_process_control_packet(i_avb, buf, nbytes, c_mac_tx, port_num);
        break;
      }
      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> unsigned int time_now:
      {
        mrp_periodic(i_avb);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }
      case i_srp.register_stream_request(avb_srp_info_t stream_info):
      {
        avb_srp_info_t local_stream_info = stream_info;
        printstrln("REGISTER STREAM REQUEST");
        avb_srp_create_and_join_talker_advertise_attrs(&local_stream_info);
        break;
      }
      case i_srp.deregister_stream_request(unsigned stream_id[2]):
      {
         unsigned int local_stream_id[2];
        local_stream_id[0] = stream_id[0];
        local_stream_id[1] = stream_id[1];
        printstrln("DEREGISTER STREAM REQUEST");
        avb_srp_leave_talker_attrs(local_stream_id);
        break;
      }
      case i_srp.register_attach_request(unsigned stream_id[2]):
      {
        unsigned int local_stream_id[2];
        local_stream_id[0] = stream_id[0];
        local_stream_id[1] = stream_id[1];
        avb_srp_join_listener_attrs(local_stream_id);
        printstrln("REGISTER ATTACH REQUEST");
        break;
      }
      case i_srp.deregister_attach_request(unsigned stream_id[2]):
      {
        unsigned int local_stream_id[2];
        local_stream_id[0] = stream_id[0];
        local_stream_id[1] = stream_id[1];
        avb_srp_leave_listener_attrs(local_stream_id);
        printstrln("DEREGISTER ATTACH REQUEST");
        break;
      }
    }
  }
}