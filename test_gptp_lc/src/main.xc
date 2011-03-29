#include <xs1.h>
#include "gptp.h"
#include "ethernet_server.h"
#include <platform.h>
#include "getmac.h"
#include "xlog_server.h"
#include "print.h"



//***** UART port ****
on stdcore[0]: port p_uart_tx = PORT_UART_TX;

//***** Ethernet Configuration ****
on stdcore[1]: port otp_data = XS1_PORT_32B; // OTP_DATA_PORT
on stdcore[1]: out port otp_addr = XS1_PORT_16C; // OTP_ADDR_PORT
on stdcore[1]: port otp_ctrl = XS1_PORT_16D; // OTP_CTRL_PORT

on stdcore[1]: mii_interface_t mii = {
		XS1_CLKBLK_1,
		XS1_CLKBLK_2,
		PORT_ETH_RXCLK,
		PORT_ETH_RXER,
		PORT_ETH_RXD,
		PORT_ETH_RXDV,
		PORT_ETH_TXCLK,
		PORT_ETH_TXEN,
		PORT_ETH_TXD
};

on stdcore[1]: out port p_mii_resetn = PORT_ETH_RSTN;
on stdcore[1]: clock clk_smi = XS1_CLKBLK_5;

on stdcore[1]: smi_interface_t smi = { PORT_ETH_MDIO, PORT_ETH_MDC, 0 };

// Test port
on stdcore[0]: port test_port = XS1_PORT_8B;

int main() 
{
  chan c_mac_rx[1], c_mac_tx[1];
  chan ptp_link[1];
  chan connect_status;

  par {
	// AVB - Ethernet
	on stdcore[1]:
	{
		int mac_address[2];
		ethernet_getmac_otp(otp_data, otp_addr, otp_ctrl, (mac_address, char[]));
		phy_init(clk_smi, p_mii_resetn,
				smi,
				mii);

		ethernet_server(mii, mac_address,
				c_mac_rx, 1,
				c_mac_tx, 1,
				smi, connect_status);
	}

    on stdcore[1]: ptp_server(c_mac_rx[0], 
                              c_mac_tx[0], 
                              ptp_link, 
                              1, 
                              PTP_GRANDMASTER_CAPABLE);

                                
    on stdcore[0]: 
    {
        int x = 0;
        timer tmr;
        int t;
        ptp_timestamp ptp_ts;
        ptp_time_info ptp_info;

        ptp_get_time_info(ptp_link[0], ptp_info);

        tmr :> t;
        local_timestamp_to_ptp(ptp_ts, t, ptp_info);

        ptp_ts.seconds[0] += 2;
        ptp_ts.nanoseconds = 0;

        t = ptp_timestamp_to_local(ptp_ts, ptp_info);

        x = ptp_ts.seconds[0] & 1;
 
        while (1) {
          tmr when timerafter(t) :> void;
          test_port <: x;
          x = ~x;
          ptp_get_time_info(ptp_link[0], ptp_info);
          ptp_timestamp_offset(ptp_ts, 10000000);
          t = ptp_timestamp_to_local(ptp_ts, ptp_info);
        }
     }
  
     // Xlog server
     on stdcore[0]: xlog_server_uart(p_uart_tx);
  }

  return 0;
}

