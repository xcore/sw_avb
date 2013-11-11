/*
 * @ModuleName IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 * @Description: Audio Codec I2S Ctl module.
 *
 *
 *
 */

#include <xs1.h>
#include <xclib.h>
#include <print.h>
#include "audio_i2s.h"
#include "media_fifo.h"

#define I2S_SINE_TABLE_SIZE 100

unsigned int i2s_sine[I2S_SINE_TABLE_SIZE] =
{
    0x0100da,0x0200b0,0x02fe81,0x03f94b,0x04f011,
    0x05e1da,0x06cdb2,0x07b2aa,0x088fdb,0x096466,
    0x0a2f74,0x0af037,0x0ba5ed,0x0c4fde,0x0ced5f,
    0x0d7dd1,0x0e00a1,0x0e754b,0x0edb5a,0x0f3267,
    0x0f7a18,0x0fb227,0x0fda5b,0x0ff28a,0x0ffa9c,
    0x0ff28a,0x0fda5b,0x0fb227,0x0f7a18,0x0f3267,
    0x0edb5a,0x0e754b,0x0e00a1,0x0d7dd1,0x0ced5f,
    0x0c4fde,0x0ba5ed,0x0af037,0x0a2f74,0x096466,
    0x088fdb,0x07b2aa,0x06cdb2,0x05e1da,0x04f011,
    0x03f94b,0x02fe81,0x0200b0,0x0100da,0x000000,
    0xfeff26,0xfdff50,0xfd017f,0xfc06b5,0xfb0fef,
    0xfa1e26,0xf9324e,0xf84d56,0xf77025,0xf69b9a,
    0xf5d08c,0xf50fc9,0xf45a13,0xf3b022,0xf312a1,
    0xf2822f,0xf1ff5f,0xf18ab5,0xf124a6,0xf0cd99,
    0xf085e8,0xf04dd9,0xf025a5,0xf00d76,0xf00564,
    0xf00d76,0xf025a5,0xf04dd9,0xf085e8,0xf0cd99,
    0xf124a6,0xf18ab5,0xf1ff5f,0xf2822f,0xf312a1,
    0xf3b022,0xf45a13,0xf50fc9,0xf5d08c,0xf69b9a,
    0xf77025,0xf84d56,0xf9324e,0xfa1e26,0xfb0fef,
    0xfc06b5,0xfd017f,0xfdff50,0xfeff26,0x000000,
};


void i2s_master_configure_ports(i2s_ports_t &i2s,
                                out buffered port:32 (&?p_dout)[num_out],
                                unsigned num_out,
                                in buffered port:32 (&?p_din)[num_in],
                                unsigned num_in)
{
  configure_clock_src(i2s.mclk, i2s.p_mclk);
  start_clock(i2s.mclk);

  configure_clock_src(i2s.bclk, i2s.p_bclk);

  configure_out_port_no_ready(i2s.p_bclk, i2s.mclk, 0);
  start_port(i2s.p_bclk);

  configure_out_port_no_ready(i2s.p_lrclk, i2s.bclk, 0);
  start_port(i2s.p_lrclk);

  for (int i=0;i<num_out;i++) {
    configure_out_port_no_ready(p_dout[i], i2s.bclk, 0);
    start_port(p_dout[i]);
  }

  for (int i=0;i<num_in;i++) {
    configure_in_port_no_ready(p_din[i], i2s.bclk);
    start_port(p_din[i]);
  }

  clearbuf(i2s.p_lrclk);
  clearbuf(i2s.p_bclk);

  for (int i = 0; i < num_in; i++) {
    clearbuf(p_din[i]);
  }
  for (int i = 0; i < num_out; i++) {
    clearbuf(p_dout[i]);
  }
  start_clock(i2s.bclk);
  return;
}

extern inline void i2s_master_upto_8(const clock mclk,
                                     clock bclk,
                                     out buffered port:32 p_bclk,
                                     out buffered port:32 p_lrclk,
                                     out buffered port:32 (&?p_dout)[],
                                     int num_out,
                                     in buffered port:32 (&?p_din)[],
                                     int num_in,
                                     int master_to_word_clock_ratio,
                                     streaming chanend ?c_listener,
                                     media_input_fifo_t (&?input_fifos)[]);

extern inline void i2s_master_upto_4(const clock mclk,
                                     clock bclk,
                                     out buffered port:32 p_bclk,
                                     out buffered port:32 p_lrclk,
                                     out buffered port:32 (&?p_dout)[],
                                     int num_out,
                                     in buffered port:32 (&?p_din)[],
                                     int num_in,
                                     int master_to_word_clock_ratio,
                                     media_input_fifo_t (&?input_fifos)[],
                                     media_output_fifo_t (&?output_fifos)[]);



