#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
// #include "ethernet_server.h"
#include "miiDriver.h"
#include "miiSingleServer.h"
#include "audio_i2s.h"
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "audio_codec_CS42448.h"
#include "simple_printf.h"
#include "media_fifo.h"

// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512

// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

out port p_chan_leds = PORT_LEDS;

void demo(chanend c_rx, chanend c_tx);

//***** Ethernet Configuration ****
// on stdcore[1]: port otp_data = XS1_PORT_32B; // OTP_DATA_PORT
// on stdcore[1]: out port otp_addr = XS1_PORT_16C; // OTP_ADDR_PORT
// on stdcore[1]: port otp_ctrl = XS1_PORT_16D; // OTP_CTRL_PORT

 mii_interface_t mii = {
		XS1_CLKBLK_1,
		XS1_CLKBLK_2,
		PORT_ETH_RXCLK,
		PORT_ETH_RXER,
		PORT_ETH_RXD,
		PORT_ETH_RXDV,
		PORT_ETH_TXCLK,
		PORT_ETH_TXEN,
		PORT_ETH_TXD, 
        XS1_PORT_16A
};

 out port p_mii_resetn = PORT_SHARED_OUT;
 clock clk_smi = XS1_CLKBLK_5;

 smi_interface_t smi = { PORT_ETH_MDIO, PORT_ETH_MDC, 0 };

 struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };

//***** AVB audio ports ****
 out port p_fs = PORT_PLL_SYNC_OUT;
 clock b_mclk = XS1_CLKBLK_3;
 clock b_bclk = XS1_CLKBLK_4;
 in port p_aud_mclk = PORT_MCLK_IN;
 buffered out port:32 p_aud_bclk = PORT_SPI_CLK;
 out buffered port:32 p_aud_lrclk = PORT_SPI_MISO;
 out buffered port:32 p_aud_dout[1] = { PORT_SPI_MOSI };
 in buffered port:32 p_aud_din[1] = { XS1_PORT_16B };

media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];

unsigned get_core_id()
{
    return 0;
}

int main(void) {
	// ethernet tx channels
	chan tx_link[3];
	chan rx_link[3];
	// chan connect_status;

	//ptp channels
	chan ptp_link[2];

	// avb unit control
	chan listener_ctl[AVB_NUM_LISTENER_UNITS];
	chan buf_ctl[AVB_NUM_LISTENER_UNITS];

	// media control
	chan media_ctl[AVB_NUM_MEDIA_UNITS];
	chan clk_ctl[AVB_NUM_MEDIA_CLOCKS];
	chan media_clock_ctl;

	// audio channels
	// streaming chan c_samples_to_codec;

	par
	{
		// AVB - Ethernet
		
		{
			 miiAVBListenerServer(clk_smi, p_mii_resetn, smi, mii, rx_link, tx_link, null);
		}

		// AVB - PTP
		
		{
			ptp_server(rx_link[0], tx_link[0], ptp_link, 2, PTP_GRANDMASTER_CAPABLE);
		}

		
		{
                media_clock_server(media_clock_ctl,
					ptp_link[1],
					buf_ctl,
					AVB_NUM_LISTENER_UNITS,
					clk_ctl,
					AVB_NUM_MEDIA_CLOCKS);
		}

		// AVB - Audio
		 {
            init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
			configure_clock_src(b_mclk, p_aud_mclk);
			start_clock(b_mclk);
            audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
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
						null,
						null,
                        ofifos,
						media_ctl[0],
						0);
			}
		}

		// AVB Listener
		 avb_1722_listener(rx_link[1],
				tx_link[1],
				buf_ctl[0],
				listener_ctl[0],
				AVB_NUM_SINKS);

		// Application threads
		
		{
            // Enable XScope printing
            xscope_register(0, 0, "", 0, "");

            xscope_config_io(XSCOPE_IO_BASIC);
            
			// First initialize avb higher level protocols
			avb_init(media_ctl, listener_ctl, null, media_clock_ctl, rx_link[2], tx_link[2], ptp_link[0]);

			demo(rx_link[2], tx_link[2]);
		}
	}

	return 0;
}

/** The main application control thread **/
void demo(chanend c_rx, chanend c_tx) {

	timer tmr;
	int avb_status = 0;
	unsigned timeout;
	unsigned listener_active = 0;
	unsigned listener_ready = 0;
	unsigned sample_rate = 48000;
    int start = 0;

	// Initialize the media clock (a ptp derived clock)
	set_device_media_clock_type(0, MEDIA_FIFO_DERIVED);
	//set_device_media_clock_type(0, LOCAL_CLOCK);
	//set_device_media_clock_type(0, PTP_DERIVED);
	set_device_media_clock_rate(0, sample_rate);
	set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
    
    if (start == 0)
    {
        avb_start();
        printstrln("start");
        start = 1;
    }

	tmr	:> timeout;
	while (1) {
		unsigned char tmp;
		unsigned int streamId[2];
		unsigned int nbytes;
		unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
		int already_seen;
		unsigned char ifnum;

		select
		{
			// Check ethernet link status
            /*
			case inuchar_byref(connect_status, ifnum):
	        {
				int status;
				status = inuchar(connect_status);
				(void) inuchar(connect_status);
				(void) inct(connect_status);
				if (status != 0) avb_start();
	        }
			break;
            */

			// Receive any incoming AVB packets (802.1Qat, 1722_MAAP)
			case avb_get_control_packet(c_rx, buf, nbytes):
            
			// Process AVB control packet if it is one
			avb_status = avb_process_control_packet(buf, nbytes, c_tx);
			switch (avb_status)
			{
				default:
				break;
			}

			// add any special control packet handling here
			break;

			// Periodic processing
			case tmr when timerafter(timeout) :> void:
			timeout += PERIODIC_POLL_TIME;

			do {
			avb_status = avb_periodic();
			switch (avb_status)
			{
			default:
				break;
			}
			} while (avb_status != AVB_NO_STATUS);

			// Look for new streams
			{
			  unsigned int streamId[2];
			  unsigned vlan;
			  unsigned char addr[6];
			  int map[2] = { 0 ,  1 };

			  // check if there is a new stream
			  int res = avb_check_for_new_stream(streamId, vlan, addr);

			  if (res) {
              
				    simple_printf("Found stream %x.%x, address %x:%x:%x:%x:%x:%x, vlan %d\n",
				    		streamId[0], streamId[1],
				    		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
				    		vlan);
			  }

			  // if so, add it to the stream table
			  if (res && listener_ready==0) {
			    set_avb_sink_sync(0, 0);
			    set_avb_sink_channels(0, 2);
			    set_avb_sink_map(0, map, 2);
			    set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
			    set_avb_sink_id(0, streamId);
			    set_avb_sink_vlan(0, vlan);
			    set_avb_sink_addr(0, addr, 6);
                
                set_avb_sink_state(0, AVB_SINK_STATE_POTENTIAL);
				listener_active = 1;
                simple_printf("Listener enabled\n");

			    listener_ready = 1;
			  }
			}

			break;
		}
	}
}

