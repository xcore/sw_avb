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
#include "simple_demo_controller.h"
#include "avb_1722_1_adp.h"
#include "app_config.h"

// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512

// Set the period inbetween periodic processing to 50us based
// on the Xcore 100Mhz timer.
#define PERIODIC_POLL_TIME 5000

// Timeout for debouncing buttons
#define BUTTON_TIMEOUT_PERIOD (20000000)

// Buttons on reference board
enum gpio_cmd
{
  STREAM_SEL=1, REMOTE_SEL=2, CHAN_SEL=4
};

// Note that this port must be at least declared to ensure it
// drives the mute low
out port p_mute_led_remote = PORT_MUTE_LED_REMOTE; // mute, led remote;
out port p_chan_leds = PORT_LEDS;
in port p_buttons = PORT_BUTTONS;

void demo(chanend c_rx, chanend c_tx);

//***** Ethernet Configuration ****
// Here are the port definitions required by ethernet
// The intializers are taken from the ethernet_board_support.h header for
// XMOS dev boards. If you are using a different board you will need to
// supply explicit port structure intializers for these values
avb_ethernet_ports_t avb_ethernet_ports =
{
  on ETHERNET_DEFAULT_TILE: OTP_PORTS_INITIALIZER,
    ETHERNET_SMI_INIT,
    ETHERNET_MII_INIT_full,
    ETHERNET_DEFAULT_RESET_INTERFACE_INIT
};

//***** AVB audio ports ****
on tile[1]: struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };

on tile[0]: out port p_fs[1] = { PORT_SYNC_OUT };
on tile[0]: i2s_ports_t i2s_ports =
{
  XS1_CLKBLK_3,
  XS1_CLKBLK_4,
  PORT_MCLK,
  PORT_SCLK,
  PORT_LRCLK
};

on tile[0]: out buffered port:32 p_aud_dout[AVB_DEMO_NUM_CHANNELS/2] = PORT_SDATA_OUT;

on tile[0]: in buffered port:32 p_aud_din[AVB_DEMO_NUM_CHANNELS/2] = PORT_SDATA_IN;

#if AVB_DEMO_ENABLE_LISTENER
media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];
#endif
#if AVB_DEMO_ENABLE_TALKER
media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];
#endif

void xscope_user_init(void)
{
  xscope_register(0, 0, "", 0, "");
  // Enable XScope printing
  xscope_config_io(XSCOPE_IO_BASIC);
}

int main(void)
{
  // Ethernet channels
  chan c_mac_tx[2 + AVB_DEMO_ENABLE_TALKER];
  chan c_mac_rx[2 + AVB_DEMO_ENABLE_LISTENER];

  // PTP channels
  chan c_ptp[1 + AVB_DEMO_ENABLE_TALKER];

  // AVB unit control
#if AVB_DEMO_ENABLE_TALKER
  chan c_talker_ctl[AVB_NUM_TALKER_UNITS];
#endif

#if AVB_DEMO_ENABLE_LISTENER
  chan c_listener_ctl[AVB_NUM_LISTENER_UNITS];
  chan c_buf_ctl[AVB_NUM_LISTENER_UNITS];
#endif

  // Media control
  chan c_media_ctl[AVB_NUM_MEDIA_UNITS];
  chan c_media_clock_ctl;

  par
  {
    // AVB - Ethernet
    on tile[1]: avb_ethernet_server(avb_ethernet_ports,
                                        c_mac_rx, 3,
                                        c_mac_tx, 3);

    on tile[0]: media_clock_server(c_media_clock_ctl,
                                   null,
                                   #if AVB_DEMO_ENABLE_LISTENER
                                   c_buf_ctl,
                                   #else
                                   null,
                                   #endif
                                   AVB_NUM_LISTENER_UNITS,
                                   p_fs,
                                   c_mac_rx[0],
                                   c_mac_tx[0],
                                   c_ptp,
                                   2,
                                   PTP_GRANDMASTER_CAPABLE);


    // AVB - Audio
    on tile[0]:
    {
#if AVB_DEMO_ENABLE_TALKER
      media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
      media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];
#endif

#if AVB_DEMO_ENABLE_LISTENER
      media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
      media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];
#endif

#if AVB_DEMO_ENABLE_TALKER
      init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
#endif
#if AVB_DEMO_ENABLE_LISTENER
      init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
#endif
      i2s_master(i2s_ports,

                 #if AVB_DEMO_ENABLE_TALKER
                 p_aud_din, AVB_NUM_MEDIA_INPUTS,
                 #else
                 null, 0,
                 #endif

                 #if AVB_DEMO_ENABLE_LISTENER
                 p_aud_dout, AVB_NUM_MEDIA_OUTPUTS,
                 #else
                 null, 0,
                 #endif

                 MASTER_TO_WORDCLOCK_RATIO,

                 #if AVB_DEMO_ENABLE_TALKER
                 ififos,
                 #else
                 null,
                 #endif

                 #if AVB_DEMO_ENABLE_LISTENER
                 ofifos,
                 #else
                 null,
                 #endif
                 c_media_ctl[0],
                 0);
    }

#if AVB_DEMO_ENABLE_TALKER
    // AVB Talker - must be on the same tile as the audio interface
    on tile[0]: avb_1722_talker(c_ptp[1],
                                c_mac_tx[2],
                                c_talker_ctl[0],
                                AVB_NUM_SOURCES);
#endif

#if AVB_DEMO_ENABLE_LISTENER
    // AVB Listener
    on tile[0]: avb_1722_listener(c_mac_rx[2],
                                  c_buf_ctl[0],
                                  null,
                                  c_listener_ctl[0],
                                  AVB_NUM_SINKS);
#endif



    // Application
    on tile[1]:
    {
      audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
      // First initialize avb higher level protocols
      avb_init(c_media_ctl,
               #if AVB_DEMO_ENABLE_LISTENER
               c_listener_ctl,
               #else
               null,
               #endif
               #if AVB_DEMO_ENABLE_TALKER
               c_talker_ctl,
               #else
               null,
               #endif
               c_media_clock_ctl,
               c_mac_rx[1], c_mac_tx[1], c_ptp[0]);

      demo(c_mac_rx[1], c_mac_tx[1]);
    }
  }

    return 0;
}

/** The main application control task **/
void demo(chanend c_rx, chanend c_tx)
{
  timer tmr;
#if AVB_DEMO_ENABLE_TALKER
  int map[AVB_NUM_MEDIA_INPUTS];
#endif
  unsigned periodic_timeout;
  unsigned sample_rate = 48000;
  int selected_chan = 0;
  int change_stream = 1;
  int buttons_active = 1;
  int toggle_remote = 0;
  unsigned buttons_timeout;
  int button_val;
  timer button_tmr;

  // Initialize the media clock
  set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED);
  set_device_media_clock_rate(0, sample_rate);
  set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);

#if AVB_DEMO_ENABLE_TALKER
  set_avb_source_channels(0, AVB_NUM_MEDIA_INPUTS);
  for (int i = 0; i < AVB_NUM_MEDIA_INPUTS; i++)
    map[i] = i;
  set_avb_source_map(0, map, AVB_NUM_MEDIA_INPUTS);
  set_avb_source_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);
  set_avb_source_sync(0, 0); // use the media_clock defined above
#endif

  set_avb_sink_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);

  p_mute_led_remote <: ~0;
  p_chan_leds <: ~(1 << selected_chan);
  p_buttons :> button_val;

  tmr :> periodic_timeout;
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
        avb_process_control_packet(buf, nbytes, c_tx);

        // add any special control packet handling here
        break;
      }

      // Receive any events from user button presses
      case buttons_active => p_buttons when pinsneq(button_val) :> unsigned new_val:
      {
        if ((button_val & STREAM_SEL) == STREAM_SEL &&
            (new_val & STREAM_SEL) == 0)
        {
          change_stream = 1;
          buttons_active = 0;
        }
        if ((button_val & REMOTE_SEL) == REMOTE_SEL &&
            (new_val & REMOTE_SEL) == 0)
        {
          toggle_remote = !toggle_remote;
          buttons_active = 0;
          p_mute_led_remote <: (~0) & ~(toggle_remote<<1);

        }

#if AVB_DEMO_ENABLE_LISTENER
        if ((button_val & CHAN_SEL) == CHAN_SEL &&
            (new_val & CHAN_SEL) == 0)
        {
          if (AVB_NUM_MEDIA_OUTPUTS > 2)
          {
            enum avb_sink_state_t cur_state;
            int channel;
            int map[AVB_NUM_MEDIA_OUTPUTS];
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
        }
#endif

        if (!buttons_active)
        {
          tmr :> buttons_timeout;
          buttons_timeout += BUTTON_TIMEOUT_PERIOD;
        }
        button_val = new_val;
        break;
      }

      case !buttons_active => button_tmr when timerafter(buttons_timeout) :> void:
      {
        buttons_active = 1;
        p_buttons :> button_val;
        break;
      }

      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> void:
      {
        avb_periodic();

        simple_demo_controller(change_stream, toggle_remote, c_tx);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }

    } // end select
  } // end while
}
