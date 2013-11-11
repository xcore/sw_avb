#include <platform.h>
#include <print.h>
#include <xccompat.h>
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
#include "avb_ethernet.h"
#include "avb_1722.h"
#include "gptp.h"
#include "media_clock_server.h"
#include "avb_1722_1.h"
#include "avb_srp.h"
#include "aem_descriptor_types.h"

on tile[0]: otp_ports_t otp_ports0 = OTP_PORTS_INITIALIZER;
on ETHERNET_DEFAULT_TILE: otp_ports_t otp_ports1 = OTP_PORTS_INITIALIZER;

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

on tile[0]: out port p_leds = XS1_PORT_4F;

//***** AVB audio ports ****
#if I2C_COMBINE_SCL_SDA
on tile[AVB_I2C_TILE]: port r_i2c = PORT_I2C;
#else
on tile[AVB_I2C_TILE]: struct r_i2c r_i2c = { PORT_I2C_SCL, PORT_I2C_SDA };
#endif

on tile[0]: out buffered port:32 p_fs[1] = { PORT_SYNC_OUT };
on tile[0]: i2s_ports_t i2s_ports =
{
  XS1_CLKBLK_3,
  XS1_CLKBLK_4,
  PORT_MCLK,
  PORT_SCLK,
  PORT_LRCLK
};

#if AVB_DEMO_ENABLE_LISTENER
on tile[0]: out buffered port:32 p_aud_dout[AVB_DEMO_NUM_CHANNELS/2] = PORT_SDATA_OUT;
#else
  #define p_aud_dout null
#endif

#if AVB_DEMO_ENABLE_TALKER
on tile[0]: in buffered port:32 p_aud_din[AVB_DEMO_NUM_CHANNELS/2] = PORT_SDATA_IN;
#else
  #define p_aud_din null
#endif

#if AVB_XA_SK_AUDIO_PLL_SLICE
on tile[0]: out port p_audio_shared = PORT_AUDIO_SHARED;
#endif

// PTP sync port
on tile[0]: port ptp_sync_port = XS1_PORT_1G;

#if AVB_DEMO_ENABLE_LISTENER
media_output_fifo_data_t ofifo_data[AVB_NUM_MEDIA_OUTPUTS];
media_output_fifo_t ofifos[AVB_NUM_MEDIA_OUTPUTS];
#else
  #define ofifos null
#endif

#if AVB_DEMO_ENABLE_TALKER
media_input_fifo_data_t ififo_data[AVB_NUM_MEDIA_INPUTS];
media_input_fifo_t ififos[AVB_NUM_MEDIA_INPUTS];
#else
  #define ififos null
#endif

[[combinable]] void application_task(client interface avb_interface avb, server interface avb_1722_1_control_callbacks i_1722_1_entity);

[[distributable]] void audio_hardware_setup(void)
{
#if PLL_TYPE_CS2100
  audio_clock_CS2100CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
#elif PLL_TYPE_CS2300
  audio_clock_CS2300CP_init(r_i2c, MASTER_TO_WORDCLOCK_RATIO);
#endif
#if AVB_XA_SK_AUDIO_PLL_SLICE
  audio_codec_CS4270_init(p_audio_shared, 0xff, 0x48, r_i2c);
  audio_codec_CS4270_init(p_audio_shared, 0xff, 0x49, r_i2c);
#endif

  while (1) {
    select {
    }
  }
}

enum mac_rx_chans {
  MAC_RX_TO_MEDIA_CLOCK = 0,
#if AVB_DEMO_ENABLE_LISTENER
  MAC_RX_TO_LISTENER,
#endif
  MAC_RX_TO_SRP,
  MAC_RX_TO_1722_1,
  NUM_MAC_RX_CHANS
};

enum mac_tx_chans {
  MAC_TX_TO_MEDIA_CLOCK = 0,
#if AVB_DEMO_ENABLE_TALKER
  MAC_TX_TO_TALKER,
#endif
  MAC_TX_TO_SRP,
  MAC_TX_TO_1722_1,
  MAC_TX_TO_AVB_MANAGER,
  NUM_MAC_TX_CHANS
};

enum avb_manager_chans {
  AVB_MANAGER_TO_SRP = 0,
  AVB_MANAGER_TO_1722_1,
  AVB_MANAGER_TO_DEMO,
  NUM_AVB_MANAGER_CHANS
};

enum ptp_chans {
  PTP_TO_AVB_MANAGER = 0,
#if AVB_DEMO_ENABLE_TALKER
  PTP_TO_TALKER,
#endif
  PTP_TO_1722_1,
  PTP_TO_TEST_CLOCK,
  NUM_PTP_CHANS
};

int main(void)
{
  // Ethernet channels
  chan c_mac_tx[NUM_MAC_TX_CHANS];
  chan c_mac_rx[NUM_MAC_RX_CHANS];

  // PTP channels
  chan c_ptp[NUM_PTP_CHANS];

  // AVB unit control
#if AVB_DEMO_ENABLE_TALKER
  chan c_talker_ctl[AVB_NUM_TALKER_UNITS];
#else
  #define c_talker_ctl null
#endif

#if AVB_DEMO_ENABLE_LISTENER
  chan c_listener_ctl[AVB_NUM_LISTENER_UNITS];
  chan c_buf_ctl[AVB_NUM_LISTENER_UNITS];
#else
  #define c_listener_ctl null
  #define c_buf_ctl null
#endif

  // Media control
  chan c_media_ctl[AVB_NUM_MEDIA_UNITS];
  chan c_media_clock_ctl;

  interface avb_interface i_avb[NUM_AVB_MANAGER_CHANS];
  interface srp_interface i_srp;
  interface avb_1722_1_control_callbacks i_1722_1_entity;
  interface spi_interface i_spi;

  par
  {
    on ETHERNET_DEFAULT_TILE:
    {
      char mac_address[6];
      otp_board_info_get_mac(otp_ports1, 0, mac_address);
      smi_init(smi1);
      eth_phy_config(1, smi1);
      ethernet_server_full_two_port(mii1,
                                    mii2,
                                    smi1,
                                    null,
                                    mac_address,
                                    c_mac_rx, NUM_MAC_RX_CHANS,
                                    c_mac_tx, NUM_MAC_TX_CHANS);
    }

    on tile[0]: media_clock_server(c_media_clock_ctl,
                                   null,
                                   c_buf_ctl,
                                   AVB_NUM_LISTENER_UNITS,
                                   p_fs,
                                   c_mac_rx[MAC_RX_TO_MEDIA_CLOCK],
                                   c_mac_tx[MAC_TX_TO_MEDIA_CLOCK],
                                   c_ptp, NUM_PTP_CHANS,
                                   PTP_GRANDMASTER_CAPABLE);

    on tile[AVB_I2C_TILE]: [[distribute]] audio_hardware_setup();

    // AVB - Audio
    on tile[0]:
    {
#if AVB_DEMO_ENABLE_TALKER
      init_media_input_fifos(ififos, ififo_data, AVB_NUM_MEDIA_INPUTS);
#endif

#if AVB_DEMO_ENABLE_LISTENER
      init_media_output_fifos(ofifos, ofifo_data, AVB_NUM_MEDIA_OUTPUTS);
#endif

      i2s_master(i2s_ports,
                 p_aud_din, AVB_NUM_MEDIA_INPUTS,
                 p_aud_dout, AVB_NUM_MEDIA_OUTPUTS,
                 MASTER_TO_WORDCLOCK_RATIO,
                 ififos,
                 ofifos,
                 c_media_ctl[0],
                 0);
    }

#if AVB_DEMO_ENABLE_TALKER
    // AVB Talker - must be on the same tile as the audio interface
    on tile[0]: avb_1722_talker(c_ptp[PTP_TO_TALKER],
                                c_mac_tx[MAC_TX_TO_TALKER],
                                c_talker_ctl[0],
                                AVB_NUM_SOURCES);
#endif

#if AVB_DEMO_ENABLE_LISTENER
    // AVB Listener
    on tile[0]: avb_1722_listener(c_mac_rx[MAC_RX_TO_LISTENER],
                                  c_buf_ctl[0],
                                  null,
                                  c_listener_ctl[0],
                                  AVB_NUM_SINKS);
#endif

    on tile[1]: [[combine]] par {
      avb_manager(i_avb, NUM_AVB_MANAGER_CHANS,
                  i_srp,
                  c_media_ctl,
                  c_listener_ctl,
                  c_talker_ctl,
                  c_mac_tx[MAC_TX_TO_AVB_MANAGER],
                  c_media_clock_ctl,
                  c_ptp[PTP_TO_AVB_MANAGER]);
      avb_srp_task(i_avb[AVB_MANAGER_TO_SRP],
                   i_srp,
                   c_mac_rx[MAC_RX_TO_SRP],
                   c_mac_tx[MAC_TX_TO_SRP]);
    }

    on tile[0].core[0]: application_task(i_avb[AVB_MANAGER_TO_DEMO], i_1722_1_entity);
    on tile[0].core[0]: avb_1722_1_task(otp_ports0,
                                        i_avb[AVB_MANAGER_TO_1722_1],
                                        i_1722_1_entity,
                                        i_spi,
                                        c_mac_rx[MAC_RX_TO_1722_1],
                                        c_mac_tx[MAC_TX_TO_1722_1],
                                        c_ptp[PTP_TO_1722_1]);
    on tile[0].core[0]: spi_task(i_spi);

    on tile[0]: ptp_output_test_clock(c_ptp[PTP_TO_TEST_CLOCK],
                                      ptp_sync_port, 100000000);

  }

    return 0;
}

/** The main application control task **/
[[combinable]]
void application_task(client interface avb_interface avb, server interface avb_1722_1_control_callbacks i_1722_1_entity)
{
#if AVB_DEMO_ENABLE_TALKER
  const int channels_per_stream = AVB_NUM_MEDIA_INPUTS/AVB_NUM_SOURCES;
  int map[AVB_NUM_MEDIA_INPUTS/AVB_NUM_SOURCES];
#endif
  unsigned sample_rate = 48000;
  unsigned char aem_identify_control_value = 0;

  // Initialize the media clock
  avb.set_device_media_clock_type(0, DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED);
  avb.set_device_media_clock_rate(0, sample_rate);
  avb.set_device_media_clock_state(0, DEVICE_MEDIA_CLOCK_STATE_ENABLED);

#if AVB_DEMO_ENABLE_TALKER
  for (int j=0; j < AVB_NUM_SOURCES; j++)
  {
    avb.set_source_channels(j, channels_per_stream);
    for (int i = 0; i < channels_per_stream; i++)
      map[i] = j ? j*(channels_per_stream)+i  : j+i;
    avb.set_source_map(j, map, channels_per_stream);
    avb.set_source_format(j, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);
    avb.set_source_sync(j, 0); // use the media_clock defined above
  }
#endif

  avb.set_sink_format(0, AVB_SOURCE_FORMAT_MBLA_24BIT, sample_rate);

  while (1)
  {
    select
    {
      case i_1722_1_entity.get_control_value(unsigned short control_type,
                                            unsigned short control_index,
                                            unsigned short &values_length,
                                            unsigned char values[508]) -> unsigned char return_status:
      {
        return_status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;

        if (control_type == AEM_CONTROL_TYPE)
        {
          switch (control_index)
          {
            case DESCRIPTOR_INDEX_CONTROL_IDENTIFY:
                values[0] = aem_identify_control_value;
                values_length = 1;
                return_status = AECP_AEM_STATUS_SUCCESS;
              break;
          }
        }

        break;
      }

      case i_1722_1_entity.set_control_value(unsigned short control_type,
                                            unsigned short control_index,
                                            unsigned short values_length,
                                            unsigned char values[508]) -> unsigned char return_status:
      {        
        return_status = AECP_AEM_STATUS_NO_SUCH_DESCRIPTOR;

        if (control_type == AEM_CONTROL_TYPE)
        {
          switch (control_index) {
            case DESCRIPTOR_INDEX_CONTROL_IDENTIFY: {
              if (values_length == 1) {
                aem_identify_control_value = values[0];
                p_leds <: aem_identify_control_value;
                return_status = AECP_AEM_STATUS_SUCCESS;
              }
              else
              {
                return_status = AECP_AEM_STATUS_BAD_ARGUMENTS;
              }
              break;
            }
          }
          
        }
        

        break;
      }
    }
  }
}
