#include "avb_ethernet.h"

void avb_ethernet_server(avb_ethernet_ports_t &ports,
                         chanend c_mac_rx[], int num_rx,
                         chanend c_mac_tx[], int num_tx)
{
  char mac_address[6];
  otp_board_info_get_mac(ports.otp_ports, 0, mac_address);
  // Start server
  eth_phy_reset(ports.eth_rst);
  ethernet_server_full(ports.mii, ports.smi,
                       mac_address,
                       c_mac_rx, num_rx,
                       c_mac_tx, num_tx);

}