#include <xs1.h>
#include <xclib.h>
#include "tdm.h"
#include "media_input_fifo.h"
#include "media_fifo.h"
#include <print.h>

// Must be a multiple of 4
#define CLOCKS_PER_CHANNEL 32

// Each sample will be right aligned, with left 32-RESOLUTION bits zeroed out
// This exactly matches the 1722 packet format
#define RESOLUTION 24

// How many channels there are in the TDM window
#define TDM_NUM_CHANNELS 8


static void tdm_loopback_aux(
  clock b_mck, in port p_mck, out port p_bck,
  out buffered port:4 p_wck,
  in buffered port:32 p_din, out buffered port:32 p_dout,
  streaming chanend c_adc, streaming chanend c_dac)
{
  unsigned t;
  int first = 2;

  // Master clock is slaved from external clock chip
  set_clock_src(b_mck, p_mck);
  set_port_clock(p_bck, b_mck);
  set_port_mode_clock(p_bck);
  set_port_clock(p_wck, b_mck);
  set_port_clock(p_din, b_mck);
  set_port_clock(p_dout, b_mck);
  start_clock(b_mck);


  p_din :> void @ t;
  t += 256;


  asm("setpt res[%0], %1" : : "r"(p_din), "r"(t - CLOCKS_PER_CHANNEL));

  p_dout @ (t - CLOCKS_PER_CHANNEL) <: 0;

  while (1)
  {
    for (int n = 0; n < 8; n++)
    {
      unsigned xin, xout;
      c_dac :> xout;
      p_dout <: bitrev(xout);
      p_din :> xin;
      if (first == 0)
        c_adc <: bitrev(xin);
      else
        first--;
    }
    t += 256;
    p_wck @ (t - 1) <: 0b0001;
  }
}


void tdm_loopback(  clock b_mck, in port p_mck, out port p_bck,
                out buffered port:4 p_wck,
                in buffered port:32 p_din, out buffered port:32 p_dout)
{
  streaming chan c_adc, c_dac;
  par {
    tdm_loopback_aux(b_mck, p_mck, p_bck, p_wck, p_din, p_dout, c_adc, c_dac);
    {
      unsigned x[6];
      int padc = 5;
      int pdac = 0;
      c_dac <: 0;
      c_dac <: 0;
      c_dac <: 0;
      while (1)
      {
        c_adc :> x[padc];
        c_dac <: x[pdac];
        padc = (padc + 1) * (padc != 5);
        pdac = (pdac + 1) * (pdac != 5);
      }
    }
  }

}


#pragma unsafe arrays
void tdm_master(
  clock b_mck, in port p_mck, out port p_bck, out buffered port:4 p_wck,
  in buffered port:32 p_din, out buffered port:32 p_dout,
  streaming chanend c_listener,
  int input_fifos[],
  int num_channels,
  chanend media_ctl,
  int clk_ctl_index)  // Both in and out
{
  unsigned t;
  unsigned timestamp;
  timer tmr;

  media_ctl_register(media_ctl, num_channels, input_fifos, 0, null, clk_ctl_index);

  c_listener <: 0;
  for (int n=0;n<num_channels;n++) {
    int x;
    c_listener :> x;
  }

  // Master clock is slaved from external clock chip
  set_clock_src(b_mck, p_mck);
  set_port_clock(p_bck, b_mck);
  set_port_mode_clock(p_bck);

  set_port_clock(p_wck, b_mck);
  set_port_clock(p_din, b_mck);
  set_port_clock(p_dout, b_mck);
  start_clock(b_mck);

  p_din :> void @ t;

  t += 64;

  asm("setpt res[%0], %1" : : "r"(p_din), "r"(t + CLOCKS_PER_CHANNEL + 2));

  p_dout @ (t + CLOCKS_PER_CHANNEL) <: 0;
  p_dout <: 0;

  t += 2 * CLOCKS_PER_CHANNEL + TDM_NUM_CHANNELS * CLOCKS_PER_CHANNEL;

  while (1)
  {
	unsigned int active_fifos = media_input_fifo_enable_req_state();
    tmr :> timestamp;
    c_listener <: timestamp;
    for (int n = 0; n < num_channels; n++)
    {
      unsigned x;
      c_listener :> x;
      p_dout <: bitrev(x << (32 - RESOLUTION)) & 0xffffff;
      p_din :> x;
      x = (bitrev(x) >> (32-RESOLUTION)) & 0xffffff;
      if (active_fifos & (1 << n)) {
        media_input_fifo_push_sample(input_fifos[n], x, timestamp);
      } else {
    	  media_input_fifo_flush(input_fifos[n]);
      }
    }
    t += num_channels * CLOCKS_PER_CHANNEL;
    p_wck @ t <: 0b0001;
    media_input_fifo_update_enable_ind_state(active_fifos, 0xFFFFFFFF);
  }
}


