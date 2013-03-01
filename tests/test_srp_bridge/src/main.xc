#include <xs1.h>
#include <xclib.h>
#include <print.h>
#include <platform.h>
#include <stdlib.h>
#include "otp_board_info.h"
#include "ethernet.h"
#include "ethernet_board_support.h"
#include <xscope.h>
#include "gptp.h"
#include "avb.h"

void xscope_user_init(void) {
  xscope_register(0);
  xscope_config_io(XSCOPE_IO_BASIC);
}

on ETHERNET_DEFAULT_TILE: otp_ports_t otp_ports = OTP_PORTS_INITIALIZER;

smi_interface_t smi1 = ETHERNET_DEFAULT_SMI_INIT;

// Circle slot
mii_interface_t mii1 = ETHERNET_DEFAULT_MII_INIT;

// Square slot
on tile[1]: mii_interface_t mii2 = {
  XS1_CLKBLK_3,
  XS1_CLKBLK_4,
  XS1_PORT_1B,
  XS1_PORT_4D,
  XS1_PORT_4A,
  XS1_PORT_1C,
  XS1_PORT_1G,
  XS1_PORT_1F,
  XS1_PORT_4B      
};

// PTP sync port
on stdcore[0]: port ptp_sync_port = XS1_PORT_1C;

#define PERIODIC_POLL_TIME 5000

int main() 
{
  chan c_mac_rx[2], c_mac_tx[2];
  chan c_ptp[1];

  par
  {
    on ETHERNET_DEFAULT_TILE:
    {
      char mac_address[6];
      otp_board_info_get_mac(otp_ports, 0, mac_address);
      smi_init(smi1);
      eth_phy_config(1, smi1);
      ethernet_server_full_two_port(mii1,
                                    mii2,
                                    smi1,
                                    null,
                                    mac_address,
                                    c_mac_rx, 2,
                                    c_mac_tx, 2);
    }

    on stdcore[0]: ptp_server(c_mac_rx[0], 
                              c_mac_tx[0], 
                              c_ptp, 
                              1, 
                              PTP_GRANDMASTER_CAPABLE);

                                
    on stdcore[0]: ptp_output_test_clock(c_ptp[0], ptp_sync_port, 100000000);

    on stdcore[0]:
    {
      timer tmr;
      unsigned periodic_timeout;

      avb_init_srp_only(c_mac_rx[1], c_mac_tx[1]);

      tmr :> periodic_timeout;
      while (1)
      {
        unsigned int nbytes;
        unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
        unsigned int port_num;

        select
        {
          // Receive any incoming AVB packets (802.1Qat, 1722_MAAP)
          case avb_get_control_packet(c_mac_rx[1], buf, nbytes, port_num):
          {
            // Test for a control packet and process it
            avb_process_control_packet(buf, nbytes, c_mac_tx[1], port_num);

            // add any special control packet handling here
            break;
          }

          // Periodic processing
          case tmr when timerafter(periodic_timeout) :> void:
          {
            avb_periodic();

            periodic_timeout += PERIODIC_POLL_TIME;
            break;
          }

        } // end select
      } // end while
    }
  }

  return 0;
}

