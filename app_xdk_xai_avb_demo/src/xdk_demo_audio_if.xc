#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include "xtcp_client.h"
#include "uip_server.h"
#include "audio_i2s.h"
#include "xlog_server.h"
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "audio_codec_CS42448.h"
#include "simple_printf.h"
#include "media_fifo.h"
#include "mdns.h"
#include "osc.h"
#include "demo_stream_manager.h"

// this is the sample rate, the frequency of the word clock
#define SAMPLE_RATE 48000

// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512

void demo(chanend listener_ctl[], chanend talker_ctl[], chanend media_ctl[],
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


on stdcore[2]: out port p_mii_resetn = PORT_ETH_RST_N;
on stdcore[2]: clock clk_smi = XS1_CLKBLK_5;

on stdcore[2]: smi_interface_t smi = { PORT_ETH_MDIO, PORT_ETH_MDC, 0 };

on stdcore[1]: struct r_i2c r_i2c = {XS1_PORT_4B, XS1_PORT_4A};


//***** AVB audio ports ****
on stdcore[1]: out port p_fs = XS1_PORT_1N;
on stdcore[1]: clock b_mclk = XS1_CLKBLK_1;
on stdcore[1]: clock b_bclk = XS1_CLKBLK_2;
on stdcore[1]: in port p_aud_mclk = XS1_PORT_1M;
on stdcore[1]: buffered out port:32 p_aud_bclk = XS1_PORT_1A;
on stdcore[1]: out buffered port:32 p_aud_lrclk = XS1_PORT_1B;
on stdcore[1]: out buffered port:32 p_aud_dout[4] = {XS1_PORT_1C,
                                                     XS1_PORT_1D,
                                                     XS1_PORT_1E,
                                                     XS1_PORT_1F};

on stdcore[1]: in buffered port:32 p_aud_din[4] = {XS1_PORT_1G,
                                                   XS1_PORT_1H,
                                                   XS1_PORT_1I,
                                                   XS1_PORT_1J};

on stdcore[1]: out port p_aud_rst = XS1_PORT_4C;

//***** Buttons/LEDS **********

on stdcore[0]: port p_uart_tx = PORT_UART_TXD;

media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];

media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];

int main(void)
{
  // ethernet tx channels
  chan tx_link[5];
  chan rx_link[4];
  chan connect_status;

  //ptp channels
  chan ptp_link[3];

  // avb unit control
  chan listener_ctl[AVB_NUM_LISTENER_UNITS];
  chan buf_ctl[AVB_NUM_LISTENER_UNITS];
  chan talker_ctl[AVB_NUM_TALKER_UNITS];

  // media control
  chan media_ctl[AVB_NUM_MEDIA_UNITS];
  chan clk_ctl[AVB_NUM_MEDIA_CLOCKS];
  chan media_clock_ctl;
  
  // audio channels
  streaming chan c_samples_to_codec;

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
    phy_init(clk_smi, p_mii_resetn,
             smi,
             mii);

    ethernet_server(mii, mac_address, 
                    rx_link, 4, 
                    tx_link, 5,
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
                       buf_ctl, 
                       1,
                       clk_ctl,
                       1);
  }

  // AVB - Audio
  on stdcore[1]: {
    audio_codec_CS42448_init(p_aud_rst, r_i2c, CODEC_I2S);
    audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
    init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
    configure_clock_src(b_mclk, p_aud_mclk);
    start_clock(b_mclk);
    par
      {
        audio_gen_CS2300CP_clock(p_fs, clk_ctl[0]);

        i2s_master (b_mclk,
                    b_bclk,
                    p_aud_bclk,
                    p_aud_lrclk,
                    p_aud_dout,
                    AVB_NUM_MEDIA_OUTPUTS,
                    p_aud_din,
                    AVB_NUM_MEDIA_INPUTS,
                    MASTER_TO_WORDCLOCK_RATIO,
                    c_samples_to_codec,
                    ififos,
                    media_ctl[0],
                    0);
      }
  }

  // AVB Talker - must be on the same core as the audio interface
  on stdcore[1]: avb_1722_talker(ptp_link[0],
                                 tx_link[1],
                                 talker_ctl[0],
                                 AVB_NUM_SOURCES);

  // AVB Listener
  on stdcore[1]: avb_1722_listener(rx_link[3],
                                   tx_link[3],
                                   buf_ctl[0],
                                   listener_ctl[0],
                                   AVB_NUM_SINKS);

  on stdcore[1]:
  { init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
    media_output_fifo_to_xc_channel_split_lr(media_ctl[1],
                                             c_samples_to_codec,
                                             0, // clk_ctl index
                                             ofifos,
                                             AVB_NUM_MEDIA_OUTPUTS);
  }

  // Xlog server
  on stdcore[0]: xlog_server_uart(p_uart_tx);



  // Application threads
  on stdcore[0]: demo(listener_ctl, talker_ctl, media_ctl, 
                      media_clock_ctl,   
                      ptp_link[2],
                      xtcp[0], rx_link[2], tx_link[4]);





 }

 return 0;
}




// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

/** The main application control thread **/
void demo(chanend listener_ctl[], chanend talker_ctl[],
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
  int selected_chan = 0;
  unsigned change_stream = 1;
  unsigned timeout;

  // First initialize avb higher level protocols
  avb_init(media_ctl, listener_ctl, talker_ctl, media_clock_ctl, 
           c_rx, c_tx, c_ptp);

  // Set AVB to be in "legacy" mode
  avb_set_legacy_mode(1);

  // Initialize Zeroconf
  mdns_init(tcp_svr);

  // Register all the zeroconf names
  mdns_register_canonical_name("xdk_xai_avb");

  // Initialize the media clock (a ptp derived clock)
  set_device_media_clock_type(0, PTP_DERIVED);
  set_device_media_clock_rate(0, SAMPLE_RATE);
  set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
  
  // Configure the source stream
  set_avb_source_name(0, "8 channel stream out");

  set_avb_source_channels(0, 8);
  for (int i=0;i<8;i++)
    map[i] = i;
  set_avb_source_map(0, map, 8);
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
        
        
        // Call the stream manager to check for new streams/manage
        // what is being listened to
        demo_manage_listener_stream(change_stream,
                                    selected_chan);


        break;
      }
  }
}



