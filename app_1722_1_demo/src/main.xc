#include <platform.h>
#include <print.h>
#include <assert.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include "audio_i2s.h"
#include <xscope.h>
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "simple_printf.h"
#include "media_fifo.h"
#include "ethernet_board_support.h"

// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512

// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

// Timeout for debouncing buttons
#define BUTTON_TIMEOUT_PERIOD (50000000)

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

void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl);

void ptp_server_and_gpio(chanend c_rx, chanend c_tx, chanend ptp_link[],
        int num_ptp, enum ptp_server_type server_type, chanend c);

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

on tile[0]: out port p_fs = PORT_SYNC_OUT;
on tile[0]: clock b_mclk = XS1_CLKBLK_3;
on tile[0]: clock b_bclk = XS1_CLKBLK_4;
on tile[0]: in port p_aud_mclk = PORT_MCLK;
on tile[0]: buffered out port:32 p_aud_bclk = PORT_SCLK;
on tile[0]: out buffered port:32 p_aud_lrclk = PORT_LRCLK;
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

media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];
media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];

void xscope_user_init(void)
{
    xscope_register(0, 0, "", 0, "");
    // Enable XScope printing
    xscope_config_io(XSCOPE_IO_BASIC);
}

int main(void)
{
    // ethernet tx channels
    chan c_mac_tx[3];
    chan c_mac_rx[3];

    //ptp channels
    chan c_ptp[3];

    // avb unit control
    chan c_talker_ctl[AVB_NUM_TALKER_UNITS];
    chan c_listener_ctl[AVB_NUM_LISTENER_UNITS];
    chan c_buf_ctl[AVB_NUM_LISTENER_UNITS];

    // media control
    chan c_media_ctl[AVB_NUM_MEDIA_UNITS];
    chan c_clk_ctl[AVB_NUM_MEDIA_CLOCKS];
    chan c_media_clock_ctl;

    // audio channels
    streaming chan c_samples_to_codec;

    // control channel from the GPIO buttons
    chan c_gpio_ctl;
  
    par
    {
        // AVB - Ethernet
        on tile[1]:  avb_ethernet_server(avb_ethernet_ports,
                                            c_mac_rx, 3,
                                            c_mac_tx, 3);

        // AVB - PTP
        on tile[1]:
        {
            // We need to initiate the PLL from core 1, so do it here before
            // launching  the main function of the thread
            audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);

            ptp_server_and_gpio(c_mac_rx[0], c_mac_tx[0], c_ptp, 3,
                    PTP_GRANDMASTER_CAPABLE,
                    c_gpio_ctl);
        }
    
        on tile[1]:
        {
            media_clock_server(c_media_clock_ctl,
                    c_ptp[1],
                    c_buf_ctl,
                    AVB_NUM_LISTENER_UNITS,
                    c_clk_ctl,
                    AVB_NUM_MEDIA_CLOCKS);
        }
    
        // AVB - Audio
        on tile[0]: {
            init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
            configure_clock_src(b_mclk, p_aud_mclk);
            start_clock(b_mclk);
            par
            {
                audio_gen_CS2300CP_clock(p_fs, c_clk_ctl[0]);

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
                        c_media_ctl[0],
                        0);
            }
        }

        // AVB Talker - must be on the same core as the audio interface
        on tile[0]: avb_1722_talker(c_ptp[2],
                c_mac_tx[2],
                c_talker_ctl[0],
                AVB_NUM_SOURCES);

        // AVB Listener
        on tile[0]: avb_1722_listener(c_mac_rx[1],
                c_buf_ctl[0],
                null,
                c_listener_ctl[0],
                AVB_NUM_SINKS);
    
        on tile[0]:
        {   
            init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
            media_output_fifo_to_xc_channel_split_lr(c_media_ctl[1],
                    c_samples_to_codec,
                    0, // clk_ctl index
                    ofifos,
                    AVB_NUM_MEDIA_OUTPUTS);
        }

        // Application threads
        on tile[0]:
        {
            // First initialize avb higher level protocols
            avb_init(c_media_ctl, c_listener_ctl, c_talker_ctl, c_media_clock_ctl, c_mac_rx[2], c_mac_tx[1], c_ptp[0]);

            demo(c_mac_rx[2], c_mac_tx[1], c_gpio_ctl);
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

    while (1)
    {
        select
        {
            do_ptp_server(c_rx, c_tx, ptp_link, num_ptp);

            case buttons_active =>
                p_buttons when pinsneq(button_val) :> unsigned new_button_val:
                if ((button_val & 0x1) == 1 && (new_button_val & 0x1) == 0)
                {
                    c <: STREAM_SEL;
                    buttons_active = 0;
                }
                if ((button_val & 0x2) == 2 && (new_button_val & 0x2) == 0)
                {
                    c <: REMOTE_SEL;
                    buttons_active = 0;
                }
                if ((button_val & 0x4) == 4 && (new_button_val & 0x4) == 0)
                {
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

/** The main application control thread **/
void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl)
{
    timer tmr;
    avb_status_t avb_status;
    int map[AVB_NUM_MEDIA_INPUTS];
    unsigned timeout;
    unsigned sample_rate = 48000;

    // Initialize the media clock 
    set_device_media_clock_type(0, INPUT_STREAM_DERIVED);
    set_device_media_clock_rate(0, sample_rate);
    set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);

    set_avb_source_channels(0, AVB_NUM_MEDIA_INPUTS);
    for (int i = 0; i < AVB_NUM_MEDIA_INPUTS; i++)
        map[i] = i;
    set_avb_source_map(0, map, AVB_NUM_MEDIA_INPUTS);
    set_avb_source_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);
    set_avb_source_sync(0, 0); // use the media_clock defined above
  
    tmr :> timeout;
    while (1)
    {
        unsigned int nbytes;
        unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];

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

        // Receive any events from user button presses
        case c_gpio_ctl :> int cmd:
        {
            break;
        }

        // Periodic processing
        case tmr when timerafter(timeout) :> void:
        {
            timeout += PERIODIC_POLL_TIME;

            avb_periodic(avb_status);
            break;
        }

        } // end select
    } // end while
}
