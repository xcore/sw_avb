#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include "ethernet_server.h"
#include "ethernet_tx_client.h"
#include "audio_i2s.h"
#include <xscope.h>
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "simple_printf.h"
#include "media_fifo.h"
#include "avb_1722_1_adp.h"
#include "avb_1722_1_acmp.h"
#include "flash.h"

// Ethernet link status
#define ETHERNET_LINK_IS_DOWN 0
#define ETHERNET_LINK_IS_UP   1

// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512

// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

// Timeout for debouncing buttons
#define BUTTON_TIMEOUT_PERIOD (50000000)

// Commands sent from the GPIO to the main demo app
enum gpio_cmd {
  STREAM_SEL, CHAN_SEL, REMOTE_SEL
};

// Note that this port must be at least declared to ensure it
// drives the mute low
out port p_mute_led_remote = PORT_SHARED_OUT; // mute, led remote;
out port p_chan_leds = PORT_LEDS;
in port p_buttons = PORT_SHARED_IN;


fl_SPIPorts fl_ports =
  {
    PORT_SPI_MISO,
    PORT_SPI_SS,
    PORT_SPI_CLK,
    PORT_SPI_MOSI,
    XS1_CLKBLK_4
  };


void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl, chanend connect_status);

void ptp_server_and_gpio(chanend c_rx, chanend c_tx, chanend ptp_link[],
             int num_ptp, enum ptp_server_type server_type, chanend c);

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

on stdcore[1]: struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };

//***** AVB audio ports ****
on stdcore[0]: out port p_fs = PORT_SYNC_OUT;
on stdcore[0]: clock b_mclk = XS1_CLKBLK_1;
on stdcore[0]: clock b_bclk = XS1_CLKBLK_2;
on stdcore[0]: in port p_aud_mclk = PORT_MCLK;
on stdcore[0]: buffered out port:32 p_aud_bclk = PORT_SCLK;
on stdcore[0]: out buffered port:32 p_aud_lrclk = PORT_LRCLK;
on stdcore[0]: out buffered port:32 p_aud_dout[4] = {
  PORT_SDATA_OUT0,
  PORT_SDATA_OUT1,
  PORT_SDATA_OUT2,
  PORT_SDATA_OUT3
};

on stdcore[0]: in buffered port:32 p_aud_din[4] = {
  PORT_SDATA_IN0,
  PORT_SDATA_IN1,
  PORT_SDATA_IN2,
  PORT_SDATA_IN3
};

on stdcore[0]: port p_uart_tx = PORT_UART_TX;

media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];
media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];


void xscope_user_init(void)
{
	#ifdef USE_XSCOPE
	/*
	xscope_register(12,
	  XSCOPE_CONTINUOUS, "PTP Filtered Adjust", XSCOPE_INT, "PTP",
	  XSCOPE_CONTINUOUS, "PTP Raw Adjust", XSCOPE_INT, "PTP",
	  XSCOPE_CONTINUOUS, "ierror", XSCOPE_INT, "WC",
	  XSCOPE_CONTINUOUS, "perror", XSCOPE_INT, "WC",
	  XSCOPE_CONTINUOUS, "wordlen", XSCOPE_INT, "WC",
	  XSCOPE_CONTINUOUS, "mediadiff", XSCOPE_INT, "Buf",
	  XSCOPE_CONTINUOUS, "lock status", XSCOPE_INT, "Buf",
	  XSCOPE_CONTINUOUS, "fill", XSCOPE_INT, "Buf",
	  XSCOPE_CONTINUOUS, "samplediff", XSCOPE_INT, "Buf",
	  XSCOPE_CONTINUOUS, "stabcount", XSCOPE_INT, "Buf",
	  XSCOPE_CONTINUOUS, "mcvalid", XSCOPE_INT, "Buf",
	);
	*/
	xscope_register(3,
	  XSCOPE_CONTINUOUS, "prestime", XSCOPE_INT, "Value",
	  XSCOPE_CONTINUOUS, "wlen", XSCOPE_INT, "Value",
	  XSCOPE_CONTINUOUS, "1722time", XSCOPE_INT, "Value"
	  );
	#else
	xscope_register(0, 0, "", 0, "");
	#endif
}


int main(void) {
  // ethernet tx channels
  chan tx_link[3];
  chan rx_link[3];
  chan connect_status;
  
  //ptp channels
  chan ptp_link[3];
  
  // avb unit control
  chan talker_ctl[AVB_NUM_TALKER_UNITS];
  chan listener_ctl[AVB_NUM_LISTENER_UNITS];
  chan buf_ctl[AVB_NUM_LISTENER_UNITS];
  
  // media control
  chan media_ctl[AVB_NUM_MEDIA_UNITS];
  chan clk_ctl[AVB_NUM_MEDIA_CLOCKS];
  chan media_clock_ctl;
  
  // audio channels
  streaming
  chan c_samples_to_codec;
  
  // control channel from the GPIO buttons
  chan c_gpio_ctl;
  
  par
  {
    // AVB - Ethernet
    on stdcore[1]:
    {
      int mac_address[2];
      ethernet_getmac_otp(otp_data, otp_addr, otp_ctrl, (mac_address, char[]));
      phy_init(clk_smi, p_mii_resetn,
           smi,
           mii);
      
      ethernet_server(mii, mac_address,
              rx_link, 3,
              tx_link, 3,
              smi, connect_status);
    }
    
    // AVB - PTP
    on stdcore[1]:
    {
      // We need to initiate the PLL from core 1, so do it here before
      // launching  the main function of the thread
      audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
      
      ptp_server_and_gpio(rx_link[0], tx_link[0], ptp_link, 3,
                PTP_GRANDMASTER_CAPABLE,
                c_gpio_ctl);
    }
    
    on stdcore[1]:
    {
      // Enable XScope printing
      xscope_config_io(XSCOPE_IO_BASIC);

      media_clock_server(media_clock_ctl,
                 ptp_link[1],
                 buf_ctl,
                 AVB_NUM_LISTENER_UNITS,
                 clk_ctl,
                 AVB_NUM_MEDIA_CLOCKS);
    }
    
    // AVB - Audio
    on stdcore[0]: {
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
    
    // AVB Listener
    on stdcore[0]: avb_1722_listener(rx_link[1],
                     buf_ctl[0],
                     null,
                     listener_ctl[0],
                     AVB_NUM_SINKS);
    
    // AVB Talker - must be on the same core as the audio interface
    on stdcore[0]: avb_1722_talker(ptp_link[2],
                     tx_link[2],
                     talker_ctl[0],
                     AVB_NUM_SOURCES);
    
    on stdcore[0]:
    { init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
      media_output_fifo_to_xc_channel_split_lr(media_ctl[1],
                           c_samples_to_codec,
                           0, // clk_ctl index
                           ofifos,
                           AVB_NUM_MEDIA_OUTPUTS);
    }
    
    // Application threads
    on stdcore[0]:
    {
      // Enable XScope printing
      // xscope_register(0, 0, "", 0, "");
      xscope_config_io(XSCOPE_IO_BASIC);

      // First initialize avb higher level protocols
      avb_init(media_ctl, listener_ctl, talker_ctl, media_clock_ctl, rx_link[2], tx_link[1], ptp_link[0]);
      
      demo(rx_link[2], tx_link[1], c_gpio_ctl, connect_status);
    }
  }
  
  return 0;
}

void ptp_server_and_gpio(chanend c_rx, chanend c_tx, chanend ptp_link[],
             int num_ptp, enum ptp_server_type server_type, chanend c) {
  
  static unsigned buttons_active = 1;
  static unsigned buttons_timeout;
  
  unsigned button_val;
  timer tmr;
  p_buttons :> button_val;
  
  ptp_server_init(c_rx, c_tx, server_type);
  
  while (1) {
    select
    {
      do_ptp_server(c_rx, c_tx, ptp_link, num_ptp);
      
    case buttons_active =>
      p_buttons when pinsneq(button_val) :> unsigned new_button_val:
      if ((button_val & 0x1) == 1 && (new_button_val & 0x1) == 0) {
        c <: STREAM_SEL;
        buttons_active = 0;
      }
      if ((button_val & 0x2) == 2 && (new_button_val & 0x2) == 0) {
        c <: REMOTE_SEL;
        buttons_active = 0;
      }
      if ((button_val & 0x4) == 4 && (new_button_val & 0x4) == 0) {
        c <: CHAN_SEL;
        buttons_active = 0;
      }
      if (!buttons_active) {
        tmr :> buttons_timeout;
        buttons_timeout += BUTTON_TIMEOUT_PERIOD;
      }
      button_val = new_button_val;
      break;
    case !buttons_active => tmr when timerafter(buttons_timeout) :> void:
      buttons_active = 1;
      p_buttons :> button_val;
      break;
      
    }
  }
}

enum avb_media_clock_list
{
  MEDIA_CLOCK_0=0,
} avb_media_clock_list;

enum avb_stream_source_list
{
  STREAM_SOURCE_0=0,
} avb_stream_source_list;

enum avb_stream_sink_list
{
  STREAM_SINK_0=0,
} avb_stream_sink_list;

void app_handle_maap_indication(avb_status_t &status)
{
  switch (status.info.maap.msg)
  {
    case AVB_MAAP_ADDRESSES_LOST:
    {
      // oh dear, someone else is using our multicast address
      for (int i=0;i<AVB_NUM_SOURCES;i++)
        set_avb_source_state(i, AVB_SOURCE_STATE_DISABLED);
      
      // request a different address
      avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);
      break;
    }
    case AVB_MAAP_ADDRESSES_RESERVED:
    {
      unsigned char macaddr[6];
      avb_1722_maap_get_offset_address(macaddr, 0);
      set_avb_source_dest(0, macaddr, 6);
      avb_1722_1_talker_set_mac_address(0, macaddr);
      simple_printf("Stream multicast address acquired (%x:%x:%x:%x:%x:%x)\n", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
      break;
    }
  }
}

void app_handle_srp_indication(avb_status_t &status)
{
  switch (status.info.srp.msg)
  {
    case AVB_SRP_TALKER_ROUTE_FAILED:
    {
      // old: avb_srp_get_failed_stream(streamId);
      // new: status.info.srp.msg.streamId

      // handle a routing failure here
      break;
    }
    case AVB_SRP_LISTENER_ROUTE_FAILED:
    {
      // old: avb_srp_get_failed_stream(streamId);
      // new: status.info.srp.msg.streamId

      // handle a routing failure here
      break;
    }
  }
}

void app_handle_1722_1_indication(avb_status_t &status, chanend c_tx)
{
  switch (status.info.a1722_1.msg)
  {
    case AVB_1722_1_CONNECT_TALKER:
    {
      unsigned streamId[2];
      get_avb_source_id(0, streamId);
      avb_1722_1_talker_set_stream_id(0, streamId);
      simple_printf("1722.1 request to advertise %x.%x\n", streamId[0], streamId[1]);
      //set_avb_source_state(0, AVB_SOURCE_STATE_DISABLED);
      set_avb_source_state(0, AVB_SOURCE_STATE_POTENTIAL);
      avb_1722_1_acmp_talker_connection_complete(0, c_tx);
      break;
    }
    
    case AVB_1722_1_DISCONNECT_TALKER:
    {
      simple_printf("1722.1 request to disconnect talker\n");
      set_avb_source_state(0, AVB_SOURCE_STATE_DISABLED);
      avb_1722_1_acmp_talker_connection_complete(0, c_tx);
      break;
    }

    case AVB_1722_1_CONNECT_LISTENER:
    {
      short listener;
      unsigned int streamId[2];
      unsigned vlan;
      unsigned char addr[6];
      int map[AVB_NUM_MEDIA_INPUTS];
      for (int i = 0; i < AVB_NUM_MEDIA_INPUTS; i++)
        map[i] = i;

      avb_1722_1_acmp_get_listener_connection_info(listener, addr, streamId, vlan);
      simple_printf("1722.1 request to connect to stream %x.%x, address %x:%x:%x:%x:%x:%x, vlan %d\n",
          streamId[0], streamId[1],
          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
          vlan);

      set_avb_sink_sync(0, 0);
      set_avb_sink_channels(0, AVB_NUM_MEDIA_INPUTS);
      set_avb_sink_map(0, map, AVB_NUM_MEDIA_INPUTS);
      set_avb_sink_id(0, streamId);
      set_avb_sink_vlan(0, vlan);
      set_avb_sink_addr(0, addr, 6);
      set_avb_source_state(0, AVB_SOURCE_STATE_DISABLED);
      set_avb_sink_state(0, AVB_SINK_STATE_POTENTIAL);

      avb_1722_1_acmp_listener_connection_complete(listener, c_tx);
      break;
    }

    case AVB_1722_1_DISCONNECT_LISTENER:
    {
      simple_printf("1722.1 request to disconnect listener\n");
      set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
      avb_1722_1_acmp_listener_connection_complete(0, c_tx);
      break;
    }
  }

}

void avb_1722_1_store(int offset,
                      const char x[],
                      int size)
{
  char scratch[4096];
  printstrln("Storing 1722.1 command to flash");
  fl_connect(fl_ports);
  fl_writeData(offset,size,x,scratch);
  fl_disconnect();
}

void avb_1722_1_load(int offset,
                     char x[],
                     int size)
{
  printstrln("Loading 1722.1 command from flash");
  fl_connect(fl_ports);
  fl_readData(offset,size,x);
  fl_disconnect();
}

/** The main application control thread **/
void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl, chanend c_eth_link_status)
{
  timer tmr;
  avb_status_t avb_status;
  int map[AVB_NUM_MEDIA_INPUTS];
  unsigned timeout;
  unsigned sample_rate = 48000;



  // Initialize the media clock (a ptp derived clock)
  set_device_media_clock_type(MEDIA_CLOCK_0, MEDIA_FIFO_DERIVED);
  //set_device_media_clock_type(0, LOCAL_CLOCK);
  //set_device_media_clock_type(0, PTP_DERIVED);
  set_device_media_clock_rate(MEDIA_CLOCK_0, sample_rate);
  set_device_media_clock_state(MEDIA_CLOCK_0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
  
  // Configure the source stream
  set_avb_source_name(STREAM_SOURCE_0, "2 channel testing stream");



  //  set_avb_source_channels(STREAM_SOURCE_0, AVB_NUM_MEDIA_INPUTS);
  set_avb_source_channels(STREAM_SOURCE_0, 1);
  for (int i = 0; i < AVB_NUM_MEDIA_INPUTS; i++)
    map[i] = i;
  //  set_avb_source_map(STREAM_SOURCE_0, map, AVB_NUM_MEDIA_INPUTS);
  set_avb_source_map(STREAM_SOURCE_0, map, 1);
  set_avb_source_format(STREAM_SOURCE_0, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);
  set_avb_source_sync(STREAM_SOURCE_0, MEDIA_CLOCK_0); // use the media_clock defined above
  
  // Request a multicast addresses for stream transmission
  avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);
  
  tmr :> timeout;
  while (1)
  {
    unsigned char tmp;
    unsigned int streamId[2];
    unsigned int nbytes;
    unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
    int already_seen;
    unsigned char ifnum;
    int link_status;

    select
    {
      // Check ethernet link status
      case mac_check_link_client(c_eth_link_status, ifnum, link_status):
      {
        if (link_status != ETHERNET_LINK_IS_DOWN)
        {
          printstrln("Up");
          avb_start();
          avb_1722_1_adp_announce();
        }
        else
        {
          printstrln("Down");
        }
        break;
      }

      // Receive any incoming AVB packets (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_rx, buf, nbytes):
      {
        // Test for a control packet and process it
        avb_process_control_packet(avb_status, buf, nbytes, c_tx);

        switch (avb_status.type)
        {
          case AVB_SRP: app_handle_srp_indication(avb_status); break;
          case AVB_MAAP: app_handle_maap_indication(avb_status); break;
          case AVB_1722_1: app_handle_1722_1_indication(avb_status, c_tx); break;
        }
        break;
      }

      // Receive any events from user button presses
      case c_gpio_ctl :> int cmd:
      {
        break;
      }

      // Periodic processing
      case tmr when timerafter(timeout) :> void:
      {
        timeout += PERIODIC_POLL_TIME;

        do
        {
          avb_periodic(avb_status);

          switch (avb_status.type)
          {
            case AVB_SRP: app_handle_srp_indication(avb_status); break;
            case AVB_MAAP: app_handle_maap_indication(avb_status); break;
            case AVB_1722_1: app_handle_1722_1_indication(avb_status, c_tx); break;
          }

        } while (avb_status.type != AVB_NO_STATUS);

        break;

      }
    }
    

  }
}

