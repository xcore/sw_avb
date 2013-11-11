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


void xscope_user_init(void) {

#if 0
  xscope_register(3, XSCOPE_CONTINUOUS, "local_egress_ts", XSCOPE_UINT, "Value",
    XSCOPE_CONTINUOUS, "received_sync_ts", XSCOPE_INT, "Value",
    XSCOPE_CONTINUOUS, "residence", XSCOPE_INT, "Value");
/*
  xscope_register(4, XSCOPE_CONTINUOUS, "rdptr", XSCOPE_UINT, "Value",
    XSCOPE_CONTINUOUS, "wrptr", XSCOPE_UINT, "Value",
    XSCOPE_CONTINUOUS, "hdr", XSCOPE_UINT, "Value",
    XSCOPE_CONTINUOUS, "hdr->next", XSCOPE_INT, "Value");
*/
/*
  xscope_register(2, XSCOPE_CONTINUOUS, "commit", XSCOPE_UINT, "Value",
    XSCOPE_CONTINUOUS, "buf", XSCOPE_INT, "Value");
*/
    // XSCOPE_CONTINUOUS, "buf", XSCOPE_INT, "Value");
    // XSCOPE_CONTINUOUS, "fwdbuf", XSCOPE_INT, "Value");
#else
  xscope_register(0);
#endif
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

int main()
{
  chan c_mac_rx[1], c_mac_tx[1];
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
                                    c_mac_rx, 1,
                                    c_mac_tx, 1);
    }

    on stdcore[0]: ptp_server(c_mac_rx[0],
                              c_mac_tx[0],
                              c_ptp,
                              1,
                              PTP_GRANDMASTER_CAPABLE);


    on stdcore[0]: ptp_output_test_clock(c_ptp[0], ptp_sync_port, 100000000);
  }

  return 0;
}

