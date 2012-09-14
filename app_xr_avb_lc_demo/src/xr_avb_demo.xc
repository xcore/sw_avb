#include <platform.h>
#include <print.h>
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
#include "media_fifo.h"
#include "mdns.h"
#include "control_api_server.h"
#include "osc.h"
#include "demo_stream_manager.h"

// this is the sample rate, the frequency of the word clock
#define SAMPLE_RATE 48000

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

void demo(chanend tcp_svr, chanend c_rx, chanend c_tx, chanend c_gpio_ctl);

void ptp_server_and_gpio(chanend c_rx, chanend c_tx, chanend ptp_link[],
		int num_ptp, enum ptp_server_type server_type, chanend c);

//***** Ethernet Configuration ****
avb_ethernet_ports_t avb_ethernet_ports = AVB_ETHERNET_DEFAULT_PORTS_INIT;

//***** AVB audio ports ****
on stdcore[1]: struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };

on stdcore[0]: out port p_fs = PORT_SYNC_OUT;
on stdcore[0]: clock b_mclk = XS1_CLKBLK_3;
on stdcore[0]: clock b_bclk = XS1_CLKBLK_4;
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

media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];

media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];




int main(void) {
	// ethernet tx channels
	chan c_mac_tx[4];
	chan c_mac_rx[4];

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
	streaming
	chan c_samples_to_codec;

	// tcp/ip channels
	chan c_xtcp[1];

	// control channel from the GPIO buttons
	chan c_gpio_ctl;

	par
	{
          on stdcore[1]:  avb_ethernet_server(avb_ethernet_ports,
                                              c_mac_rx, 4,
                                              c_mac_tx, 4);
          // TCP/IP stack
          on stdcore[1]:
          {
            xtcp_server_uip(c_mac_rx[1],
                            c_mac_tx[2],
                            c_xtcp, 1,
                            null);
          }

		// AVB - PTP
		on stdcore[1]:
		{
			// We need to initiate the PLL from core 1, so do it here before
			// launching  the main function of the thread
			audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);

			ptp_server_and_gpio(c_mac_rx[0], c_mac_tx[0], ptp_link, 3,
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
		on stdcore[0]: avb_1722_talker(ptp_link[0],
				c_mac_tx[1],
				talker_ctl[0],
				AVB_NUM_SOURCES);

		// AVB Listener
		on stdcore[0]: avb_1722_listener(c_mac_rx[3],
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

		// Xlog server
		on stdcore[0]:
		{
			xlog_server_uart(p_uart_tx);
		}

		// Application threads
		on stdcore[0]:
		{
			// First initialize avb higher level protocols
			avb_init(media_ctl, listener_ctl, talker_ctl, media_clock_ctl, c_mac_rx[2], c_mac_tx[3], ptp_link[2]);

			demo(c_xtcp[0], c_mac_rx[2], c_mac_tx[3], c_gpio_ctl);
		}
	}

	return 0;
}

void ptp_server_and_gpio(chanend c_rx, chanend c_tx, chanend ptp_link[],
		int num_ptp, enum ptp_server_type server_type, chanend c) {

	static unsigned buttons_active = 1;
	static unsigned buttons_timeout;
	static unsigned remote = 0;
	static unsigned selected_chan = 0;

	unsigned button_val;
	timer tmr;
    p_mute_led_remote <: ~(remote << 1) | 1;
	p_chan_leds <: ~(1 << selected_chan);
	p_buttons :> button_val;

	ptp_server_init(c_rx, c_tx, server_type);

	while (1) {
		select
		{
			do_ptp_server(c_rx, c_tx, ptp_link, num_ptp);

			case buttons_active =>
			p_buttons when pinsneq(button_val) :> unsigned new_button_val:
			if ((button_val & 0x1) == 1 &&
					(new_button_val & 0x1) == 0) {
				c <: STREAM_SEL;
				buttons_active = 0;
			}
			if ((button_val & 0x2) == 2 &&
					(new_button_val & 0x2) == 0) {
				remote = 1-remote;
				p_mute_led_remote <: ~(remote << 1) | 1;

				/* Currently we do not do anything with the remote select
				 mode. So there is no need to signal the demo thread. */
				//c <: REMOTE_SEL;
				//c <: remote;
				buttons_active = 0;
			}
			if ((button_val & 0x4) == 4 &&
					(new_button_val & 0x4) == 0) {
				selected_chan++;
				if (selected_chan > ((AVB_NUM_MEDIA_OUTPUTS>>1)-1))
                {
                    selected_chan = 0;
                }
				p_chan_leds <: ~(1 << selected_chan);
				c <: CHAN_SEL;
				c <: selected_chan;
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

/** The main application control thread **/
void demo(chanend tcp_svr, chanend c_rx, chanend c_tx, chanend c_gpio_ctl) {

	timer tmr;
	int avb_status = 0;
	int map[AVB_NUM_MEDIA_INPUTS];
	unsigned char macaddr[6];
	int selected_chan = 0;
	unsigned change_stream = 1;
	unsigned timeout;

	// Set AVB to be in "legacy" mode
	//  avb_set_legacy_mode(1);

	// Initialize Zeroconf
	mdns_init(tcp_svr);

	// Register all the zeroconf names
	mdns_register_canonical_name("xmos_attero_endpoint");
	mdns_register_service("XMOS/Attero AVB", "_attero-cfg._udp",
			ATTERO_CFG_PORT, "");

	// Initialize the control api server
	c_api_server_init(tcp_svr);

	// Initialize the media clock (a ptp derived clock)
	//printstr("Media clock: LOCAL\n");
	//set_device_media_clock_type(0, MEDIA_FIFO_DERIVED);
	set_device_media_clock_type(0, LOCAL_CLOCK);
	//set_device_media_clock_type(0, PTP_DERIVED);
	set_device_media_clock_rate(0, SAMPLE_RATE);
	set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);

	// Configure the source stream
	set_avb_source_name(0, "multi channel stream out");

	set_avb_source_channels(0, AVB_NUM_MEDIA_INPUTS);
	for (int i = 0; i < AVB_NUM_MEDIA_INPUTS; i++)
		map[i] = i;
	set_avb_source_map(0, map, AVB_NUM_MEDIA_INPUTS);
	set_avb_source_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, SAMPLE_RATE);
	set_avb_source_sync(0, 0); // use the media_clock defined above

	// Main loop
	tmr	:> timeout;
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

			// Process TCP/IP events
			case xtcp_event(tcp_svr, conn):
			{
				if (conn.event == XTCP_IFUP)
				{
					avb_start();

					// Request a multicast addresses for stream transmission
					avb_1722_maap_request_addresses(AVB_NUM_SOURCES, null);
				}
				else if (conn.event == XTCP_IFDOWN)
				{
					for(int i=0; i<AVB_NUM_SOURCES; i++)
					{
						set_avb_source_state(i, AVB_SOURCE_STATE_DISABLED);
					}
				}

				{
					mdns_event res;
					res = mdns_xtcp_handler(tcp_svr, conn);
					if (res & mdns_entry_lost)
					{
						printstr("Media clock: FIFO\n");
						set_device_media_clock_type(0, MEDIA_FIFO_DERIVED);
					}
				}

				c_api_xtcp_handler(tcp_svr, conn);

				// add any special tcp/ip packet handling here
			}
			break;

			// Receive any events from user button presses
			case c_gpio_ctl :> int cmd:
			{
				switch (cmd)
				{
					case STREAM_SEL:
					change_stream = 1;
					break;
					case CHAN_SEL:
					{
						enum avb_sink_state_t cur_state;
						int channel;

						c_gpio_ctl :> selected_chan;
						channel = selected_chan*2;
						get_avb_sink_state(0, cur_state);
						set_avb_sink_state(0, AVB_SINK_STATE_DISABLED);
						for (int j=0;j<AVB_NUM_MEDIA_OUTPUTS;j++)
						{
							map[j] = channel;
							channel++;
							if (channel > AVB_NUM_MEDIA_OUTPUTS-1)
							{
								channel = 0;
							}
						}
						set_avb_sink_map(0, map, AVB_NUM_MEDIA_OUTPUTS);
						if (cur_state != AVB_SINK_STATE_DISABLED)
						set_avb_sink_state(0, AVB_SINK_STATE_POTENTIAL);
					}
					break;
				}
			}
			break;

			// Periodic processing
			case tmr when timerafter(timeout) :> void:
			timeout += PERIODIC_POLL_TIME;

			do
			{
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

			// Call the stream manager to check for new streams/manage
			// what is being listened to
			demo_manage_listener_stream(change_stream, selected_chan);

			break;
		}
	}
}

