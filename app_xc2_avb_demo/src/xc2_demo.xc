#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include "xtcp_client.h"
#include "xlog_server.h"
#include "uip_server.h"
#include "i2c.h"
#include "avb.h"
#include "simple_printf.h"
#include "media_fifo.h"
#include "mdns.h"
#include "osc.h"
#include "synth.h"

/* Defines to determine the sample rate of the demo */
#define SAMPLE_RATE 48000

void demo(chanend talker_ctl[], chanend media_ctl[],
          chanend media_clock_ctl,
          chanend c_ptp,
          chanend tcp_svr,
          chanend c_rx,
          chanend c_tx);

//***** Ethernet Configuration ****
on stdcore[2]: port otp_data = XS1_PORT_32B; 		// OTP_DATA_PORT
on stdcore[2]: out port otp_addr = XS1_PORT_16C;	// OTP_ADDR_PORT
on stdcore[2]: port otp_ctrl = XS1_PORT_16D;		// OTP_CTRL_PORT

on stdcore[2]: mii_interface_t mii =
  {
    XS1_CLKBLK_1,
    XS1_CLKBLK_2,

    PORT_ETH_RXCLK,
    PORT_ETH_RXER,
    PORT_ETH_RXD,
    PORT_ETH_RXDV,
    PORT_ETH_TXCLK,
    PORT_ETH_TXEN,
    PORT_ETH_TXD,
  };


on stdcore[2]: clock clk_smi = XS1_CLKBLK_5;

on stdcore[2]: smi_interface_t smi = { PORT_ETH_RST_N_MDIO, PORT_ETH_MDC, 1 };

//***** Buttons/LEDS **********

on stdcore[0]: port p_uart_tx = PORT_UART_TX;

media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];

int main(void)
{
  // ethernet tx channels
  chan tx_link[4];
  chan rx_link[3];
  chan connect_status;

  //ptp channels
  chan ptp_link[3];

  // avb unit control
  chan talker_ctl[AVB_NUM_TALKER_UNITS];

  // media control
  chan media_ctl[AVB_NUM_MEDIA_UNITS];
  chan clk_ctl[AVB_NUM_MEDIA_CLOCKS];
  chan media_clock_ctl;
  
  // tcp/ip channels
  chan xtcp[1];

par
{


  // AVB - Ethernet
  on stdcore[2]:
  {
    int mac_address[2];
    ethernet_getmac_otp(otp_data, otp_addr, otp_ctrl, 
                        (mac_address, char[]));
    phy_init(clk_smi, null,
             smi,
             mii);

    ethernet_server(mii, mac_address, 
                    rx_link, 3, 
                    tx_link, 4,
                    smi, connect_status);
  }


  // TCP/IP stack
  on stdcore[1]: uip_server(rx_link[1], tx_link[2], 
                            xtcp, 1, null, connect_status);

  
  // AVB - PTP 
  on stdcore[0]:
  {
    ptp_server(rx_link[0], tx_link[0], 
               ptp_link,
               3,
               PTP_GRANDMASTER_CAPABLE);
  }

  on stdcore[1]:
  {
    media_clock_server(media_clock_ctl,
                       ptp_link[1], 
                       null, 
                       0,
                       clk_ctl,
                       1);
  }

  // AVB - Audio
  on stdcore[0]: {
    init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
    synth(50, media_ctl[0], clk_ctl[0], 0, ififos, 2);
  }

  // AVB Talker - must be on the same core as the audio interface
  on stdcore[0]: avb_1722_talker(ptp_link[0],
                                 tx_link[1],
                                 talker_ctl[0],
                                 AVB_NUM_SOURCES);

  // Xlog server
  on stdcore[0]: xlog_server_uart(p_uart_tx);

  // Application threads
  on stdcore[0]: demo(talker_ctl, media_ctl, 
                      media_clock_ctl,   
                      ptp_link[2],
                      xtcp[0], rx_link[2], tx_link[3]);





 }

 return 0;
}



// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

/** The main application control thread **/
void demo(chanend talker_ctl[],
          chanend media_ctl[], chanend media_clock_ctl,
          chanend c_ptp,
          chanend tcp_svr,
          chanend c_rx,
          chanend c_tx)
 {
  timer tmr;
  int avb_status = 0;
  int map[8];
  unsigned char macaddr[6];
  unsigned timeout;

  // First initialize avb higher level protocols
  avb_init(media_ctl, null, talker_ctl, media_clock_ctl, 
           c_rx, c_tx, c_ptp);

  // Set AVB to be in "legacy" mode
  //avb_set_legacy_mode(1);

  // Initialize Zeroconf
  mdns_init(tcp_svr);

  // Register all the zeroconf names
  mdns_register_canonical_name("xc2_avb_endpoint");

  // Initialize the media clock (a ptp derived clock)
  set_device_media_clock_type(0, PTP_DERIVED);
  set_device_media_clock_rate(0, SAMPLE_RATE);
  set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
  
  // Configure the source stream
  set_avb_source_name(0, "2 channel sine out");

  set_avb_source_channels(0, 2);
  for (int i=0;i<2;i++)
    map[i] = i;
  set_avb_source_map(0, map, 2);
  set_avb_source_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, SAMPLE_RATE);
  set_avb_source_sync(0, 0); // use the media_clock defined above
  
  // Request a multicast addresses for stream transmission
  avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);

  tmr :> timeout;
  while (1) {
    unsigned char tmp;
    xtcp_connection_t conn;
    unsigned int streamId[2];
    unsigned int nbytes;
    unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
    int already_seen;

    select
      {
      // Receive any incoming AVB packets (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_rx,
                                  buf,
                                  nbytes):

        // Process AVB control packet if it is one
        avb_status = avb_process_control_packet(buf, nbytes, c_tx);
        switch (avb_status)
          {
          case AVB_SRP_TALKER_ROUTE_FAILED:
            avb_srp_get_failed_stream(streamId);
            // handle a routing failure here
            break;
          case AVB_SRP_LISTENER_ROUTE_FAILED:
            avb_srp_get_failed_stream(streamId);
            // handle a routing failure here
            break;
          case AVB_MAAP_ADDRESSES_LOST:
            // oh dear, someone else is using our multicast address
            for (int i=0;i<AVB_NUM_SOURCES;i++)
              set_avb_source_state(i, AVB_SOURCE_STATE_DISABLED);

            // request a different address
            avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);
            break;
          default:
            break;
          }

        // add any special control packet handling here
        break;

      // Process TCP/IP events
      case xtcp_event(tcp_svr, conn):
        if (conn.event == XTCP_IFUP)
          avb_start();

        mdns_xtcp_handler(tcp_svr, conn);

        // add any special tcp/ip packet handling here
        break;



      // Periodic processing
      case tmr when timerafter(timeout) :> void:        
        timeout += PERIODIC_POLL_TIME;

		do {
			avb_status = avb_periodic();
			switch (avb_status)
			{
			case AVB_MAAP_ADDRESSES_RESERVED:
				for(int i=0;i<AVB_NUM_SOURCES;i++) {
					avb_1722_maap_get_offset_address(macaddr, i);
					// activate the source
					set_avb_source_dest(i, macaddr, 6);
					set_avb_source_state(i, AVB_SOURCE_STATE_POTENTIAL);
				}
				break;
			}
		} while (avb_status != AVB_NO_STATUS);

		break;
      }
  }
}



