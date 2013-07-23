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

#define PERIODIC_POLL_TIME 5000

void avb_1722_1_periodic(chanend c_tx, chanend c_ptp, client interface avb_interface avb)
{
    avb_1722_1_adp_advertising_periodic(c_tx, c_ptp);
    avb_1722_1_adp_discovery_periodic(c_tx);
#if (AVB_1722_1_CONTROLLER_ENABLED)
    avb_1722_1_acmp_controller_periodic(c_tx, avb);
#endif
#if (AVB_1722_1_TALKER_ENABLED)
    avb_1722_1_acmp_talker_periodic(c_tx, avb);
#endif
#if (AVB_1722_1_LISTENER_ENABLED)
    avb_1722_1_acmp_listener_periodic(c_tx, avb);
#endif
}

[[combinable]]
void avb_1722_1_task(client interface avb_interface avb,
                     chanend c_mac_rx,
                     chanend c_mac_tx,
                     chanend c_ptp) {
  unsigned periodic_timeout;
  timer tmr;
  unsigned int nbytes;
  unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
  unsigned int port_num;
  unsigned char mac_addr[6];

  mac_get_macaddr(c_mac_tx, mac_addr);
  avb_1722_1_init(mac_addr);

  tmr :> periodic_timeout;

  while (1) {
    select {
      // Receive and process any incoming AVB packets (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_mac_rx, buf, nbytes, port_num):
      {
        avb_process_1722_1_packet(buf, nbytes, c_mac_tx);
        break;
      }
      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> unsigned int time_now:
      {
        avb_1722_1_periodic(c_mac_tx, c_ptp, avb);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }
    }
  }
}