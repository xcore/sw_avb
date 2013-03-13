#include <platform.h>
#include <print.h>
#include <xccompat.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
#include "audio_i2s.h"
#include "avb_xscope.h"
#include "i2c.h"
#include "avb.h"
#include "audio_clock_CS2300CP.h"
#include "audio_clock_CS2100CP.h"
#include "audio_codec_CS4270.h"
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

on ETHERNET_DEFAULT_TILE: otp_ports_t otp_ports = OTP_PORTS_INITIALIZER;

smi_interface_t smi1 = ETHERNET_DEFAULT_SMI_INIT;

// Circle slot
mii_interface_t mii1 = ETHERNET_DEFAULT_MII_INIT;

// Square slot
on tile[1]: mii_interface_t mii2 = {
  XS1_CLKBLK_3,
  XS1_CLKBLK_4,
  XS1_PORT_1B,
  XS1_PORT_4D,
  XS1_PORT_4A,
  XS1_PORT_1C,
  XS1_PORT_1G,
  XS1_PORT_1F,
  XS1_PORT_4B      
};

//***** AVB audio ports ****
#if I2C_COMBINE_SCL_SDA
on tile[AVB_I2C_TILE]: port r_i2c = PORT_I2C;
#else
on tile[AVB_I2C_TILE]: struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };
#endif

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

#if AVB_XA_SK_AUDIO_SLICE
on tile[0]: out port p_audio_shared = PORT_AUDIO_SHARED;
#endif

// PTP sync port
on tile[0]: port ptp_sync_port = XS1_PORT_1C;

#if AVB_DEMO_ENABLE_LISTENER
media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];
#endif
#if AVB_DEMO_ENABLE_TALKER
media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];
#endif

void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl);
void gpio_task(chanend c_gpio_ctl);

void xscope_user_init(void)
{
  xscope_register_no_probes();
  // Enable XScope printing
  xscope_config_io(XSCOPE_IO_BASIC);
}

void audio_hardware_setup(void)
{
#if PLL_TYPE_CS2100
  audio_clock_CS2100CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
#elif PLL_TYPE_CS2300
  audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
#endif
#if AVB_XA_SK_AUDIO_SLICE
  audio_codec_CS4270_init(p_audio_shared, 0xff, 0x90, r_i2c);
  audio_codec_CS4270_init(p_audio_shared, 0xff, 0x92, r_i2c);
#endif
}

int main(void)
{
  // Ethernet channels
  chan c_mac_tx[2 + AVB_DEMO_ENABLE_TALKER];
  chan c_mac_rx[2 + AVB_DEMO_ENABLE_LISTENER];

  // PTP channels
  chan c_ptp[2 + AVB_DEMO_ENABLE_TALKER];

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

  chan c_gpio_ctl;

  par
  {
    on ETHERNET_DEFAULT_TILE:
    {
      char mac_address[6];
      otp_board_info_get_mac(otp_ports, 0, mac_address);
      smi_init(smi1);
      eth_phy_config(1, smi1);
      ethernet_server_full_two_port(mii1,
                                    mii2,
                                    smi1,
                                    null,
                                    mac_address,
                                    c_mac_rx, 2 + AVB_DEMO_ENABLE_LISTENER,
                                    c_mac_tx, 2 + AVB_DEMO_ENABLE_TALKER);
    }

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
                                   2 + AVB_DEMO_ENABLE_TALKER,
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

#if (AVB_I2C_TILE == 0)
      audio_hardware_setup();
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

    // on tile[AVB_GPIO_TILE]: gpio_task(c_gpio_ctl);

    // Application
    on tile[1]:
    {
#if (AVB_I2C_TILE == 1)
      audio_hardware_setup();
#endif
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

      demo(c_mac_rx[1], c_mac_tx[1], c_gpio_ctl);
    }

    on tile[0]: ptp_output_test_clock(c_ptp[2], ptp_sync_port, 100000000);

  }

    return 0;
}

void gpio_task(chanend c_gpio_ctl)
{
  int button_val;
  int buttons_active = 1;
  int toggle_remote = 0;
  unsigned buttons_timeout;
  int selected_chan = 0;
  timer button_tmr;

  p_mute_led_remote <: ~0;
  p_chan_leds <: ~(1 << selected_chan);
  p_buttons :> button_val;

  while (1)
  {
    select
    {
      case buttons_active => p_buttons when pinsneq(button_val) :> unsigned new_button_val:
        if ((button_val & STREAM_SEL) == STREAM_SEL && (new_button_val & STREAM_SEL) == 0)
        {
          c_gpio_ctl <: STREAM_SEL;
          buttons_active = 0;
        }
        if ((button_val & REMOTE_SEL) == REMOTE_SEL && (new_button_val & REMOTE_SEL) == 0)
        {
          c_gpio_ctl <: REMOTE_SEL;
          toggle_remote = !toggle_remote;
          buttons_active = 0;
          p_mute_led_remote <: (~0) & ~(toggle_remote<<1);
        }
        if ((button_val & CHAN_SEL) == CHAN_SEL && (new_button_val & CHAN_SEL) == 0)
        {
          selected_chan++;
          if (selected_chan > ((AVB_NUM_MEDIA_OUTPUTS>>1)-1))
          {
            selected_chan = 0;
          }
          p_chan_leds <: ~(1 << selected_chan);
          c_gpio_ctl <: CHAN_SEL;
          c_gpio_ctl <: selected_chan;
          buttons_active = 0;
        }
        if (!buttons_active)
        {
          button_tmr :> buttons_timeout;
          buttons_timeout += BUTTON_TIMEOUT_PERIOD;
        }
        button_val = new_button_val;
        break;
      case !buttons_active => button_tmr when timerafter(buttons_timeout) :> void:
        buttons_active = 1;
        p_buttons :> button_val;
        break;
    }
  }

}

/** The main application control task **/
void demo(chanend c_rx, chanend c_tx, chanend c_gpio_ctl)
{
  timer tmr;
#if AVB_DEMO_ENABLE_TALKER
  int map[AVB_NUM_MEDIA_INPUTS];
#endif
  unsigned periodic_timeout;
  unsigned sample_rate = 48000;
  int change_stream = 1;
  int toggle_remote = 0;

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

  tmr :> periodic_timeout;
  while (1)
  {
    unsigned int nbytes;
    unsigned int buf[(MAX_AVB_CONTROL_PACKET_SIZE+1)>>2];
    unsigned int port_num;

    select
    {
      // Receive any incoming AVB packets (802.1Qat, 1722_MAAP)
      case avb_get_control_packet(c_rx, buf, nbytes, port_num):
      {
        // Test for a control packet and process it
        avb_process_control_packet(buf, nbytes, c_tx, port_num);

        // add any special control packet handling here
        break;
      }

      // Receive any events from user button presses from the GPIO task
      case c_gpio_ctl :> int cmd:
      {
        switch (cmd)
        {
          case STREAM_SEL:
          {
            change_stream = 1;
            break;
          }
          case CHAN_SEL:
          {
            int selected_chan;
            c_gpio_ctl :> selected_chan;
#if AVB_DEMO_ENABLE_LISTENER
            if (AVB_NUM_MEDIA_OUTPUTS > 2)
            {
              enum avb_sink_state_t cur_state;
              int channel;
              int map[AVB_NUM_MEDIA_OUTPUTS];

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
#endif
            break;
          }
          case REMOTE_SEL:
          {
            toggle_remote = !toggle_remote;
            break;
          }
          break;
        }
        break;
      }

      // Periodic processing
      case tmr when timerafter(periodic_timeout) :> unsigned int time_now:
      {
        avb_periodic(time_now);

        simple_demo_controller(change_stream, toggle_remote, c_tx);

        periodic_timeout += PERIODIC_POLL_TIME;
        break;
      }

    } // end select
  } // end while
}
