#include <platform.h>
#include <print.h>
#include "simple_printf.h"
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
#include "xtcp.h"
#include "audio_i2s.h"
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "media_fifo.h"
#include "mdns.h"
#include "control_api_server.h"
#include "demo_stream_manager.h"
#include "ethernet_board_support.h"

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
enum gpio_cmd
{
    STREAM_SEL, CHAN_SEL, REMOTE_SEL
};

// Note that this port must be at least declared to ensure it
// drives the mute low
out port p_mute_led_remote = PORT_SHARED_OUT; // mute, led remote;
out port p_chan_leds = PORT_LEDS;
in port p_buttons = PORT_SHARED_IN;

void demo(chanend tcp_svr, chanend c_rx, chanend c_tx);

//***** Ethernet Configuration ****

// Here are the port definitions required by ethernet
// The intializers are taken from the ethernet_board_support.h header for
// XMOS dev boards. If you are using a different board you will need to
// supply explicit port structure intializers for these values
avb_ethernet_ports_t avb_ethernet_ports =
  {on ETHERNET_DEFAULT_TILE: OTP_PORTS_INITIALIZER,
   ETHERNET_DEFAULT_SMI_INIT,
   ETHERNET_DEFAULT_MII_INIT_full,
   ETHERNET_DEFAULT_RESET_INTERFACE_INIT};

//***** AVB audio ports ****
on tile[1]: struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };

on tile[0]: out port p_fs[1] = { PORT_SYNC_OUT };

on tile[0]: i2s_ports_t i2s_ports = {
  XS1_CLKBLK_3,
  XS1_CLKBLK_4,
  PORT_MCLK,
  PORT_SCLK,
  PORT_LRCLK
};

on tile[0]: out buffered port:32 p_aud_dout[4] =
{
        PORT_SDATA_OUT0,
        PORT_SDATA_OUT1,
        PORT_SDATA_OUT2,
        PORT_SDATA_OUT3
};

on tile[0]: in buffered port:32 p_aud_din[4] =
{
        PORT_SDATA_IN0,
        PORT_SDATA_IN1,
        PORT_SDATA_IN2,
        PORT_SDATA_IN3
};

on tile[0]: port p_uart_tx = PORT_UART_TX;


void xscope_user_init(void)
{
    xscope_register(0, 0, "", 0, "");
    // Enable XScope printing
    xscope_config_io(XSCOPE_IO_BASIC);
}


int main(void)
{
    // Ethernet channels
    chan c_mac_tx[4];
    chan c_mac_rx[4];

    // PTP channels
    chan c_ptp[2];

    // AVB unit control
    chan c_listener_ctl[AVB_NUM_LISTENER_UNITS];
    chan c_buf_ctl[AVB_NUM_LISTENER_UNITS];
    chan c_talker_ctl[AVB_NUM_TALKER_UNITS];

    // Media control
    chan c_media_ctl[AVB_NUM_MEDIA_UNITS];
    chan c_media_clock_ctl;



    // TCP/IP channels
    chan c_xtcp[1];

    par
    {
        // AVB - Ethernet
        on tile[1]:  avb_ethernet_server(avb_ethernet_ports,
                                            c_mac_rx, 4,
                                            c_mac_tx, 4);

        on tile[0]:
        {
          media_clock_server(c_media_clock_ctl,
                             null,
                             c_buf_ctl,
                             AVB_NUM_LISTENER_UNITS,
                             p_fs,
                             c_mac_rx[0],
                             c_mac_tx[0],
                             c_ptp,
                             2,
                             PTP_GRANDMASTER_CAPABLE);
        }

        // AVB - Audio
        on tile[0]: {
          media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
          media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];

          media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
          media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];

          init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
          init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
          i2s_master(i2s_ports,
                     p_aud_din,
                     AVB_NUM_MEDIA_INPUTS,
                     p_aud_dout,
                     AVB_NUM_MEDIA_OUTPUTS,
                     MASTER_TO_WORDCLOCK_RATIO,
                     ififos,
                     ofifos,
                     c_media_ctl[0],
                     0);
        }

        // AVB 1722 packet processing
        // Must be on the same tile as the audio interface
        on tile[0]: avb_1722_talkerlistener(c_ptp[0],
                                            c_mac_rx[3],
                                            c_mac_tx[1],
                                            c_listener_ctl[0],
                                            c_talker_ctl[0],
                                            c_buf_ctl[0],
                                            AVB_NUM_SINKS,
                                            AVB_NUM_SOURCES);


        // TCP/IP stack
        on tile[0]:  xtcp_server_uip(c_mac_rx[1],
                                     c_mac_tx[2],
                                     c_xtcp, 1,
                                     null);

        // Application threads
        on tile[1]:
        {
            // First initialize avb higher level protocols
            audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);

            avb_init(c_media_ctl, c_listener_ctl, c_talker_ctl,
                     c_media_clock_ctl, c_mac_rx[2], c_mac_tx[3], c_ptp[1]);

            demo(c_xtcp[0], c_mac_rx[2], c_mac_tx[3]);
        }

        on tile[0]: par(int i=0;i<3;i++) while(1);
        on tile[1]: par(int i=0;i<2;i++) while(1);
    }

    return 0;
}


/** The main application control thread **/
void demo(chanend tcp_svr, chanend c_rx, chanend c_tx)
{

    timer tmr;
    avb_status_t avb_status;
    int map[AVB_NUM_MEDIA_INPUTS];
    int selected_chan = 0;
    unsigned change_stream = 1;
    unsigned timeout;
    unsigned buttons_active = 1;
    unsigned buttons_timeout;
    unsigned button_val;
    timer button_tmr;

    // Set AVB to be in "legacy" mode
    //  avb_set_legacy_mode(1);

    // Initialize Zeroconf
    mdns_init(tcp_svr);

    // Register all the zeroconf names
    mdns_register_canonical_name("xmos_attero_endpoint");
    mdns_register_service("XMOS/Attero AVB", "_attero-cfg._udp", ATTERO_CFG_PORT, "");

    // Initialize the control api server
    c_api_server_init(tcp_svr);

    // Initialize the media clock (a ptp derived clock)
    //set_device_media_clock_type(0, INPUT_STREAM_DERIVED);
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

    p_mute_led_remote <: ~0;
    p_chan_leds <: ~(1 << selected_chan);
    p_buttons :> button_val;

    // Main loop
    tmr :> timeout;
    while (1) {
        int ret;
        xtcp_connection_t conn;
        unsigned int streamId[2];
        unsigned int nbytes;
        unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
        int already_seen;

        select
        {
        // Receive any incoming AVB packets (802.1Qat, 1722_MAAP)
        case avb_get_control_packet(c_rx, buf, nbytes):
        {
            // Test for a control packet and process it
            avb_process_control_packet(avb_status, buf, nbytes, c_tx);

            // add any special control packet handling here
            break;
        }

        // Process TCP/IP events
        case xtcp_event(tcp_svr, conn):
        {
            mdns_event res;
            res = mdns_xtcp_handler(tcp_svr, conn);
            if (res & mdns_entry_lost)
            {
                printstr("Media clock: Input stream\n");
                set_device_media_clock_type(0, INPUT_STREAM_DERIVED);
            }

            c_api_xtcp_handler(tcp_svr, conn, tmr);

            // add any special tcp/ip packet handling here
            break;
        }

        // Receive any events from user button presses
        case buttons_active => p_buttons when pinsneq(button_val) :>
                   unsigned new_button_val:

            if ((button_val & 0x1) == 1 && (new_button_val & 0x1) == 0)
              {
                change_stream = 1;
                buttons_active = 0;
              }
            if ((button_val & 0x4) == 4 && (new_button_val & 0x4) == 0)
            {
              enum avb_sink_state_t cur_state;
              int channel;
              selected_chan++;
              if (selected_chan > ((AVB_NUM_MEDIA_OUTPUTS>>1)-1))
                {
                  selected_chan = 0;
                }
              p_chan_leds <: ~(1 << selected_chan);
              buttons_active = 0;
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
            if (!buttons_active) {
                tmr :> buttons_timeout;
                buttons_timeout += BUTTON_TIMEOUT_PERIOD;
            }
            button_val = new_button_val;
            break;
        case !buttons_active => button_tmr when timerafter(buttons_timeout) :> void:
          buttons_active = 1;
          p_buttons :> button_val;
          break;
        // Periodic processing
        case tmr when timerafter(timeout) :> void:
        {
            timeout += PERIODIC_POLL_TIME;

            avb_periodic(avb_status);

            // Call the stream manager to check for new streams/manage
            // what is being listened to
            demo_manage_listener_stream(change_stream, selected_chan);

            break;
        }

        } // end select
    } // end while
}

