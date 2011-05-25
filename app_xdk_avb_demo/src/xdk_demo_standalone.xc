#include <platform.h>
#include <print.h>
#include "avb.h"
#include "audio_codec_config.h"
#include "async_i2s.h"
#include "fft.h"
#include "filter.h"
#include "touch.h"
#include "LcdDriver.h"
#include "LCD_Comp_Def.h"
#include "LCD_Switch.h"
#include "LCD_TXT_Client.h"
#include "xdk_demo_stream_manager.h"
#include "xdk_buttons.h"
#include "xdk_lcd_debug.h"
#include "gptp.h"
#include "i2c.h"
#include "xtcp_client.h"
#include "uip_server.h"
#include "mdns.h"
#include "osc.h"
#include "monitor.h"
#include "xlog_server.h"

void xdk_demo(chanend listener_ctl[], chanend talker_ctl[],
              chanend media_ctl[], chanend media_clock_ctl,
              chanend ptp_link,
              chanend monitor_ctl,
              chanend txt_svr, 
              chanend debug_stream, 
              chanend lcd_ctl,
              chanend xtcp, 
              chanend c_mac_rx,
              chanend c_mac_tx);

static void output_test_clock(chanend ptp_link);
on stdcore[1]: port test_clock_port = XS1_PORT_1P;

//***** Ethernet Configuration ****

on stdcore[2]: port otp_data = XS1_PORT_32B; 		// OTP_DATA_PORT
on stdcore[2]: out port otp_addr = XS1_PORT_16C;	// OTP_ADDR_PORT
on stdcore[2]: port otp_ctrl = XS1_PORT_16D;		// OTP_CTRL_PORT


on stdcore[2]: mii_interface_t mii = 
  { XS1_CLKBLK_1, XS1_CLKBLK_2,    
    PORT_ETH_RXCLK, PORT_ETH_RXER, 
    PORT_ETH_RXD, PORT_ETH_RXDV,                                       
    PORT_ETH_TXCLK, PORT_ETH_TXEN, PORT_ETH_TXD, };

on stdcore[2]: out port p_mii_resetn = PORT_ETH_RST_N;
on stdcore[2]:clock clk_smi = XS1_CLKBLK_5;

on stdcore[2]: smi_interface_t smi = { PORT_ETH_MDIO, PORT_ETH_MDC, 0 };

//***** AVB Audio Port Configuration ****

// Externally declared ports.
// 2-wire configuration interface.
on stdcore[3]:struct r_i2c r_i2c = {XS1_PORT_1K, XS1_PORT_1L};

// I2S based two channel Audio in/out
on stdcore[3]:struct i2s_resources r_i2s = 
  {
    XS1_CLKBLK_3,
    PORT_AUDIO_BCLK,
    PORT_AUDIO_LRCOUT,
    PORT_AUDIO_LRCIN,
    { XS1_PORT_1I },
    { XS1_PORT_1J }    
  };

#define CLK_LCD          XS1_CLKBLK_1

on stdcore[3] : struct lcd_resources r_lcd = 
  {
    PORT_LCD_HSYNC,
    PORT_LCD_DTMG,
    PORT_LCD_DCLK,
    PORT_LCD_RGB,
    CLK_LCD
  };

media_input_fifo_data_t ififo_data[2];
media_input_fifo_t ififos[2];

media_output_fifo_data_t ofifo_data[2];
media_output_fifo_t ofifos[2];

int main(void) {
  // ethernet
  chan tx_link[5];
  chan rx_link[4];
  chan connect_status;
  
  //ptp channels
  chan ptp_link[5];
  
  // avb unit control
  chan listener_ctl[AVB_NUM_LISTENER_UNITS];
  chan buf_ctl[AVB_NUM_LISTENER_UNITS];
  chan talker_ctl[AVB_NUM_TALKER_UNITS];

  // media control
  chan media_ctl[AVB_NUM_MEDIA_UNITS];
  chan clk_ctl[AVB_NUM_MEDIA_CLOCKS];
  chan filter_ctl;
  streaming chan audio_monitor;
  chan media_clock_ctl;

  // display
  chan lcd_data[2];
  chan equalizer;
  chan txt_data;
  chan lcd_ctl;

  // Configuration and debugging
  chan debug_stream;
  chan xtcp[1];

  streaming chan c_samples, c_filtered_samples;

  chan monitor_ctl;
  
  par
    {
      // AVB - Ethernet
      on stdcore[2]:
      {
        int mac_address[2];
        ethernet_getmac_otp(otp_data, otp_addr, otp_ctrl, (mac_address, char[]));
        if ((mac_address, char[])[0] == 0xff)
          (mac_address, char[])[0] = 0;

        phy_init(clk_smi, p_mii_resetn, smi, mii);
        ethernet_server(mii, mac_address, rx_link,
                        4, tx_link, 5, smi,
                        connect_status);
      }
                
      
      // AVB - AUDIO

      on stdcore[3]:
      {
        init_media_input_fifos(ififos, ififo_data, 2);
        audio_TI_TLV320AIC23B_init(r_i2c, 1);
        i2s_master_dyn_wordclk(r_i2s, 
                               media_ctl[1], 
                               clk_ctl[0],  
                               monitor_ctl,
                               0, // clk_ctl index
                               c_filtered_samples, // samples to output
                               ififos,
                               2,  // # channels in
                               2,  // # channels out
                               audio_monitor);      
      }

      on stdcore[3]: stereo_biquad_filter(filter_ctl, 
                                          c_samples, 
                                          c_filtered_samples);           

      on stdcore[1]: media_clock_server(media_clock_ctl,
                                        ptp_link[2], 
                                        buf_ctl, 
                                        1,
                                        clk_ctl,
                                        1);
     
      on stdcore[0]: avb_1722_listener(rx_link[1], tx_link[3],    
                                       buf_ctl[0], listener_ctl[0], 1);
      
      on stdcore[0]: 
      { 
        init_media_output_fifos(ofifos, ofifo_data, 2);
        media_output_fifo_to_xc_channel_split_lr(media_ctl[0], 
                                                 c_samples, 
                                                 0, // clk_ctl index 
                                                 ofifos,
                                                 2);  // # channels 
      }

      on stdcore[3]: avb_1722_talker(ptp_link[0], tx_link[1], talker_ctl[0], 1);
            
      // AVB - PTP
      on stdcore[1]: ptp_server(rx_link[0], tx_link[0], 
                                ptp_link,
                                5,
                                PTP_GRANDMASTER_CAPABLE);

      // Debugging
      on stdcore[0]: xlog_server_chan(debug_stream);
      // DEMO
      
      // LCD output
      on stdcore[3]: lcd_driver(r_lcd, lcd_data, lcd_ctl);

      // Text output to LCD
      on stdcore[3]: lcd_text_display(txt_data, lcd_data[1]);
      
      // Frequency spectrum analyser      
      on stdcore[2]: freq(equalizer, audio_monitor, lcd_data[0], ptp_link[1]);

      // Touchscreen interface
      on stdcore[3]: touch(equalizer, filter_ctl);
    
      // A TCP/IP server
      on stdcore[1]: uip_server(rx_link[2], tx_link[2],
				xtcp, 1, null, connect_status);
      

      // Main thread that runs the demo - see below
      on stdcore[0]: xdk_demo(listener_ctl, talker_ctl, media_ctl, media_clock_ctl,
                              ptp_link[3],
                              monitor_ctl,
                              txt_data, debug_stream, lcd_ctl,
                              xtcp[0], rx_link[3], tx_link[4]);

      on stdcore[1]: output_test_clock(ptp_link[4]);
    }
  
  return 0;
  
}

#define DEBUG_TXT 0
#define STREAM_TXT 1

/**
 * Manage button presses
 */
void button_manager(chanend lcd_ctl, chanend media_ctl, 
                    chanend txt_svr, unsigned int &changeStream,
                    unsigned int &txtDisplay,
                    int is_talker,
                    int is_listener) 
{
  int button_value;
  static int lcd_choice = 1;

  // Check on button presses
  button_value = poll_xdk_buttons();
  switch (button_value)
    {
    case 1:
      if (lcd_choice == 0)        
        {
          int msource;
          get_device_monitor_source(msource);
          if (msource < 2)
            set_device_monitor_source(2);
          else
            set_device_monitor_source(1);                     
        }
      lcd_choice = 0;
      lcd_ctl <: 0;
      break;
    case 3:
      lcd_choice = 1;
      lcd_ctl <: 1;
      if (txtDisplay != DEBUG_TXT) {
        txtDisplay = DEBUG_TXT;
        xdk_lcd_init_debug_screen(txt_svr);
      }
      break;
    case 5:
      if (is_listener) {
        lcd_ctl <: 1;
        if (txtDisplay != STREAM_TXT) {
          txtDisplay = STREAM_TXT;
          LCD_TXT_ClrScreen(txt_svr);
          xdk_stream_manager_display_table(txt_svr);
        }
        else if (lcd_choice == 1)
          changeStream = 1;
        lcd_choice = 1;
      }
      break;
    }
}


int AVB1722_AddToStreamTable(unsigned int id_hi,
                             unsigned int id_lo);


/** Top level configuration thread
 */
void xdk_demo(chanend listener_ctl[], chanend talker_ctl[],
              chanend media_ctl[], chanend media_clock_ctl,
              chanend c_ptp,
              chanend monitor_ctl,
              chanend txt_svr, 
              chanend debug_stream, 
              chanend lcd_ctl,
              chanend tcp_svr, 
              chanend c_rx,
              chanend c_tx)
 {
  unsigned int changeStream = 1;
  int avb_status = 0;
  unsigned int txtDisplay = DEBUG_TXT;
  int map[2];
  unsigned char macaddr[6];
  int button_value;
  int chosen=0;
  int is_talker, is_listener;

  // First initalize avb higher level protocols
  avb_init(media_ctl, listener_ctl, talker_ctl, media_clock_ctl, 
           c_rx, c_tx,c_ptp);

  // Set avb to be in "legacy" mode 
  avb_set_legacy_mode(1);

  mdns_init(tcp_svr);

  // Initialize the monitor control (a special control just for this demo)
  register_monitor_ctl(monitor_ctl);

  // Register all the zeroconf names
  mdns_register_canonical_name("xdk_avb");

  LCD_TXT_ClrScreen(txt_svr);
  LCD_TXT_ClrLine(txt_svr, 0);
  LCD_TXT_PutStringColour(txt_svr, "XDK AVB Demo\n\n\n\n", LCD_RED);
  LCD_TXT_PutStringColour(txt_svr,"  Red - Talker Only\n  Yellow - Listener Only\n  Black - Talker/Listener (PTP Clock)\n", LCD_BLUE);
  lcd_ctl <: 1;
  while(!chosen) {
    xdk_xlog_to_lcd(debug_stream, txt_svr, 0);
    button_value = poll_xdk_buttons();    
    switch (button_value)
      {
      case 1:
        // TALKER
        // initialize the media_clock (a local clock)
        set_device_media_clock_type(0, LOCAL_CLOCK);
        set_device_media_clock_rate(0, 48000);
        set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
        
        // configure the single avb source
        set_avb_source_name(0, "line in 1/2");
        set_avb_source_channels(0, 2);
        map[0] = 0; map[1] = 1; 
        set_avb_source_map(0, map, 2);
        set_avb_source_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, 48000);
        set_avb_source_sync(0, 0); // use the media_clock defined above
        
        // request a single multicast address for stream transmission
        avb_1722_maap_request_addresses(1, null);
        chosen = 1;
        is_talker = 1;
        is_listener = 0;
        break;
      case 2:
        // LISTENER
        // initialize the media_clock (a stream derived clock))
        set_device_media_clock_type(0, MEDIA_FIFO_DERIVED);
        set_device_media_clock_rate(0, 48000);
        set_device_media_clock_source(0, 0);
        set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
        chosen = 1;
        is_talker = 0;
        is_listener = 1;
        break;
      case 3:
        // TALKER/LISTENER
        // initialize the media_clock (a ptp derived clock)
        set_device_media_clock_type(0, PTP_DERIVED);
        set_device_media_clock_rate(0, 48000);
        set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
        
        // configure the single avb source
        set_avb_source_name(0, "line in 1/2");
        set_avb_source_channels(0, 2);
        map[0] = 0; map[1] = 1; 
        set_avb_source_map(0, map, 2);
        set_avb_source_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, 48000);
        set_avb_source_sync(0, 0); // use the media_clock defined above
        
        // request a single multicast address for stream transmission
        avb_1722_maap_request_addresses(1, null);

        is_talker = 1;
        is_listener = 1;
        chosen = 1;
        break;
      }
  }


  xdk_lcd_init_debug_screen(txt_svr);
  lcd_ctl <: 1;


 
  while (1) {
    unsigned char tmp;
    xtcp_connection_t conn;
    unsigned int streamId[2];
    unsigned int nbytes;
    unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];

    select
      {
      // Process TCP/IP events
      case xtcp_event(tcp_svr, conn):
        if (conn.event == XTCP_IFUP) 
          avb_start();

        mdns_xtcp_handler(tcp_svr, conn);

        // add any special tcp/ip packet handling here
        break;
        
      // receive and incoming avb packet (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_rx,
                                  buf,
                                  nbytes):

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
            if (is_talker) {
              set_avb_source_state(0, AVB_SOURCE_STATE_DISABLED);    
              
              // request a different address
              avb_1722_maap_request_addresses(1, null);
            }
            break;
        }

        // add any special control packet handling here
        break;
                                     
      default:
        // Periodic avb config processing
    	do {
        avb_status = avb_periodic();

        switch (avb_status) 
          {
          case AVB_MAAP_ADDRESSES_RESERVED:
            if (is_talker) {
              avb_1722_maap_get_base_address(macaddr);
              // activate the source
              set_avb_source_dest(0, macaddr, 6);
              set_avb_source_state(0, AVB_SOURCE_STATE_POTENTIAL);            
            }
            break;
          }
    	} while (avb_status != AVB_NO_STATUS);

        // Handle any button events
        button_manager(lcd_ctl, media_ctl[0], txt_svr, 
                       changeStream, txtDisplay,
                       is_talker, is_listener);


        // This call handles the LCD screen stream selection 
        if (is_listener) {
          xdk_manage_streams(listener_ctl[0],
                           txt_svr,
                             (txtDisplay == STREAM_TXT),
                             changeStream);
        }

        // Transfer messages from the xlog debug server to the 
        // LCD text buffer
        xdk_xlog_to_lcd(debug_stream, txt_svr, (txtDisplay == DEBUG_TXT));
        break;
      }
  }
}


static void output_test_clock(chanend ptp_link)
{ int x = 0;
        timer tmr;
        int t;
        ptp_timestamp ptp_ts;
        ptp_time_info ptp_info;

#if 0   

       tmr :> t;
        while(1) {
          tmr when timerafter(t) :> void;
          test_clock_port <: x;
          x = ~x;
          t += 1000000;
        }
#else

        ptp_get_time_info(ptp_link, ptp_info);
        


        while(1) {
          tmr :> t;
          t += 200000000;
          tmr when timerafter(t) :> void;
          
        tmr :> t;
          //printstr("ref_local:   ");printuintln(ptp_info.local_ts);
        
          //        printstr("ref_ptp:   ");print_ptp_ts(ptp_info.ptp_ts);

          //        printstr("t:   ");printuintln(t);
        local_timestamp_to_ptp(ptp_ts, t, ptp_info);
        //        printstr("t(ptp):   ");print_ptp_ts(ptp_ts);

        ptp_ts.seconds[0] += 2;
        ptp_ts.nanoseconds = 0;

        t = ptp_timestamp_to_local(ptp_ts, ptp_info);
        //        printstr("t:   ");printuintln(t);

        x = ptp_ts.seconds[0] & 1;

        while (1) {
          tmr when timerafter(t) :> void;
          test_clock_port <: x;
          x = ~x;
          ptp_get_time_info(ptp_link, ptp_info);
          ptp_timestamp_offset(ptp_ts, 1000000000);
          t = ptp_timestamp_to_local(ptp_ts, ptp_info);
          //        printstr("ref_local:   ");printuintln(ptp_info.local_ts);
        
          //        printstr("ref_ptp:   ");print_ptp_ts(ptp_info.ptp_ts);

          //        printstr("t:   ");printuintln(t);

        } 
        }

#endif        

      }
