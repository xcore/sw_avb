#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
#include "ethernet_server.h"
#include "tsi_input.h"
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "simple_printf.h"
#include "media_fifo.h"

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

//***** AVB TS-SPI ports ****
on stdcore[0]: out port p_fs = PORT_SYNC_OUT;
on stdcore[0]: in port p_ts_clk = PORT_SDATA_OUT2;
on stdcore[0]: in port p_ts_valid = PORT_SDATA_OUT1;
on stdcore[0]: in buffered port:4 p_ts_sync = PORT_SDATA_OUT3;
on stdcore[0]: in buffered port:32 p_ts_data = XS1_PORT_8B;
on stdcore[0]: clock clk_ts = XS1_CLKBLK_1;

on stdcore[0]: port p_uart_tx = PORT_UART_TX;

media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];


int main(void) {
	// ethernet tx channels
	chan tx_link[3];
	chan rx_link[2];
	chan connect_status;

	//ptp channels
	chan ptp_link[2];

	// avb unit control
	chan talker_ctl[AVB_NUM_TALKER_UNITS];

	// media control
	chan media_ctl[AVB_NUM_MEDIA_UNITS];

	// control channel from the GPIO buttons
	chan c_gpio_ctl;

	par
	{
		// AVB - Ethernet
		on stdcore[1]:
		{
			int mac_address[2];
			ethernet_getmac_otp(otp_data, otp_addr, otp_ctrl, (mac_address, char[]));

			// Override with a specific prefix
			mac_address[0] = 0x00049700;

			phy_init(clk_smi, p_mii_resetn,
					smi,
					mii);

			ethernet_server(mii, mac_address,
					rx_link, 2,
					tx_link, 3,
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
            // Enable XScope printing
            //xscope_register(0, 0, "", 0, "");
            //xscope_config_io(XSCOPE_IO_BASIC);
		}

		// AVB - Audio
		on stdcore[0]: {
			init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
			media_ctl_register(media_ctl[0], 1, ififos, 0, null, 0);
			tsi_input(clk_ts, p_ts_data, p_ts_clk, p_ts_sync, p_ts_valid, ififo_data[0]);
		}

		// AVB Talker - must be on the same core as the audio interface
		on stdcore[0]: avb_1722_talker(ptp_link[0],
				tx_link[1],
				talker_ctl[0],
				AVB_NUM_SOURCES);

		// Application threads
		on stdcore[0]:
		{
            // Enable XScope printing
            //xscope_register(0, 0, "", 0, "");
            //xscope_config_io(XSCOPE_IO_BASIC);
            
			// First initialize avb higher level protocols
			avb_init(media_ctl, null, talker_ctl, null, rx_link[1], tx_link[2], ptp_link[1]);

			demo(rx_link[1], tx_link[2], c_gpio_ctl, connect_status);
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
	int map[1];
	unsigned char macaddr[6];
	unsigned timeout;
	unsigned talker_active = 0;
	unsigned talker_ok_to_start = 0;

	// Configure the source stream
	set_avb_source_name(0, "MPEG transport stream");

	set_avb_source_channels(0, 1);
	map[0] = 0;
	set_avb_source_map(0, map, 1);
	set_avb_source_sync(0, 0); // use the media_clock defined above

	// Request a multicast addresses for stream transmission
	avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);

	avb_start();

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

			// Receive any events from user button presses
			case c_gpio_ctl :> int cmd:
			switch (cmd)
			{
			case CHAN_SEL:
			{
				// The channel select button starts and stops listening
				if (talker_active)
				{
					set_avb_source_state(0, AVB_SOURCE_STATE_DISABLED);
					talker_active = 0;
					simple_printf("Talker disabled\n");
					c_gpio_ctl <: 0;
				}
				else if (talker_ok_to_start)
				{
					unsigned id[2];
					get_avb_source_id(0, id);
					set_avb_source_state(0, AVB_SOURCE_STATE_POTENTIAL);
					talker_active = 1;
					c_gpio_ctl <: 1;
					simple_printf("Talker enabled (%x.%x)\n", id[0], id[1]);
				}
			}
			break;
			case STREAM_SEL:
			{
			}
			break;
			default:
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
				case AVB_MAAP_ADDRESSES_RESERVED:
				avb_1722_maap_get_offset_address(macaddr, 0);
				// activate the source
				set_avb_source_dest(0, macaddr, 6);
				talker_ok_to_start = 1;
				simple_printf("Stream multicast address acquired (%x:%x:%x:%x:%x:%x)\nPress Channel Select to advertise stream.\n", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
				set_avb_source_state(0, AVB_SOURCE_STATE_POTENTIAL);
				break;
			}
			} while (avb_status != AVB_NO_STATUS);

			break;
		}
	}
}

