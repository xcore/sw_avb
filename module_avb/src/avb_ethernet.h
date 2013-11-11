#ifndef __avb_ethernet_h__
#define __avb_ethernet_h__
#include "xccompat.h"
#include "otp_board_info.h"
#include "ethernet.h"
#include "mii.h"

#ifdef __XC__
typedef struct avb_ethernet_ports_s {
  otp_ports_t otp_ports;
  smi_interface_t smi;
  mii_interface_full_t mii;
  ethernet_reset_interface_t eth_rst;
} avb_ethernet_ports_t;

void avb_ethernet_server(avb_ethernet_ports_t &ports,
                         chanend c_mac_rx[], int num_rx,
                         chanend c_mac_tx[], int num_tx);

#endif


#endif // __avb_ethernet_h__
