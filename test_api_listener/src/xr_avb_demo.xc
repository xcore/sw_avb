#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
#include "ethernet_server.h"
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

// Timeout for debouncing buttons
#define BUTTON_TIMEOUT_PERIOD (20000000)

// Commands sent from the GPIO to the main demo app
enum gpio_cmd {
	STREAM_SEL, CHAN_SEL, REMOTE_SEL
};

// Note that this port must be at least declared to ensure it
// drives the mute low
out port p_mute_led_remote = PORT_SHARED_OUT; // mute, led remote;
out port p_chan_leds = PORT_LEDS;
in port p_buttons = PORT_SHARED_IN;

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


void xscope_user_init()
{
	xscope_register(2,
		XSCOPE_CONTINUOUS, "Samples 1", XSCOPE_UINT, "val",
		XSCOPE_CONTINUOUS, "Samples 2", XSCOPE_UINT, "val"
		);
	xscope_config_io(XSCOPE_IO_BASIC);
	// xscope_register(0, 0, "", 0, "");
}


int main(void) {
	// ethernet tx channels
	chan tx_link[2];
	chan rx_link[3];
	chan connect_status;

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
					tx_link, 2,
					smi, connect_status);
		}

		// AVB - PTP
		on stdcore[1]:
		{
			// We need to initiate the PLL from core 1, so do it here before
			// launching  the main function of the thread
			audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);

			ptp_server_and_gpio(rx_link[0], tx_link[0], ptp_link, 2,
					PTP_GRANDMASTER_CAPABLE,
					c_gpio_ctl);
		}

		on stdcore[1]:
		{
			media_clock_server(media_clock_ctl,
					ptp_link[1],
					buf_ctl,
					AVB_NUM_LISTENER_UNITS,
					clk_ctl,
					AVB_NUM_MEDIA_CLOCKS);
		}

		// AVB - Audio
		on stdcore[0]: {
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
						null,
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

		on stdcore[0]:
		{	init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
			media_output_fifo_to_xc_channel_split_lr(media_ctl[1],
					c_samples_to_codec,
					0, // clk_ctl index
					ofifos,
					AVB_NUM_MEDIA_OUTPUTS);
		}

		// Application threads
		on stdcore[0]:
		{
            
			// First initialize avb higher level protocols
			avb_init(media_ctl, listener_ctl, null, media_clock_ctl, rx_link[2], tx_link[1], ptp_link[0]);

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

			case buttons_active => p_buttons when pinsneq(button_val) :> unsigned new_button_val:
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

			case c :> unsigned led_command:
				p_chan_leds <: (~led_command);
				break;

		}
	}
}

/** The main application control thread **/
void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl, chanend connect_status) {

	timer tmr;
	int avb_status = 0;
	unsigned timeout;
	unsigned listener_active = 0;
	unsigned listener_ready = 0;
	unsigned sample_rate = 48000;

	// Initialize the media clock (a ptp derived clock)
	set_device_media_clock_type(0, MEDIA_FIFO_DERIVED);
	//set_device_media_clock_type(0, LOCAL_CLOCK);
	//set_device_media_clock_type(0, PTP_DERIVED);
	set_device_media_clock_rate(0, sample_rate);
	set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);

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
			case inuchar_byref(connect_status, ifnum):
	        {
				int status;
				status = inuchar(connect_status);
				(void) inuchar(connect_status);
				(void) inct(connect_status);
				if (status != 0) avb_start();
	        }
			break;

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

			// Receive any events from user button presses
			case c_gpio_ctl :> int cmd:
			switch (cmd)
			{
				case CHAN_SEL:
				{
					if (listener_active)
					{
						set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
						listener_active = 0;
						simple_printf("Listener disabled\n");
						c_gpio_ctl <: 1;
					}
					else if (listener_ready)
					{
						set_avb_sink_state(0, AVB_SINK_STATE_POTENTIAL);
						listener_active = 1;
						simple_printf("Listener enabled\n");
						c_gpio_ctl <: 2;
					}
				}
				break;
				case STREAM_SEL:
				{
					// Channel select switches the sample frequency
					// The stream sel button cycles through frequency settings
					switch (sample_rate)
					{
					case 8000:
						sample_rate = 96000;
						break;
					case 16000:
						sample_rate = 8000;
						break;
					case 32000:
						sample_rate = 16000;
						break;
					case 44100:
						sample_rate = 32000;
						break;
					case 48000:
						sample_rate = 44100;
						break;
					case 88200:
						sample_rate = 48000;
						break;
					case 96000:
						sample_rate = 88200;
						break;
					}
					simple_printf("Frequency set to %d Hz\n", sample_rate);

					set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_DISABLED);
					set_device_media_clock_rate(0, sample_rate);
					set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);
				}
				break;
			}
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
			  // int map[8] = { -1, -1, -1, -1, -1, -1, 0, 1 };
			  int map[8] = {0, 1, 2, 3, 4, 5, 6, 7};

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
			    set_avb_sink_channels(0, 8);
			    set_avb_sink_map(0, map, 8);
			    set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
			    set_avb_sink_id(0, streamId);
			    set_avb_sink_vlan(0, vlan);
			    set_avb_sink_addr(0, addr, 6);

			    listener_ready = 1;
			    c_gpio_ctl <: 1;
			  }
			}

			break;
		}
	}
}

