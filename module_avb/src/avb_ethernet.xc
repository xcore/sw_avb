#include "avb_ethernet.h"

#if (NUM_ETHERNET_PORTS == 1)

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

#endif

#if (NUM_ETHERNET_PORTS == 2) && (NUM_ETHERNET_SLAVE_PORTS == 1)

void avb_ethernet_server_with_phy_mode_port(avb_ethernet_ports_t &avb_port,
                                            mii_slave_interface_full_t &phy_mode_port, 
                                            chanend c_mac_rx[], int num_rx,
                                            chanend c_mac_tx[], int num_tx)
{
  char mac_address[6];
  otp_board_info_get_mac(avb_port.otp_ports, 0, mac_address);
  // Start server
  eth_phy_reset(avb_port.eth_rst);
  ethernet_server_full_with_phy_mode_port(avb_port.mii, avb_port.smi, phy_mode_port,
                                          mac_address,
                                          c_mac_rx, num_rx,
                                          c_mac_tx, num_tx);

}

#endif