#include <xs1.h>
#include <xclib.h>
#include <print.h>
#include "media_fifo.h"
#include "media_clock_client.h"
#include "async_i2s.h"

int i2s_ts=0;
static int i2s_diff=0;
#pragma unsafe arrays
static void inline i2s_master(clock bclk,
                              out buffered port:32 AUD_BCLK,
                              out port AUD_LRCOUT,
                              out port AUD_LRCIN,
                              out buffered port:32 p_aud_dout[],
                              in buffered port:32 p_aud_din[],
                              chanend media_ctl,
                              chanend clk_ctl,
                              chanend monitor_ctl,
                              int clk_ctl_index,
                              streaming chanend samples_in, 
                              media_input_fifo_t input_fifos[],
                              int num_in, 
                              int num_out,
                              streaming chanend ?audio_monitor)
{
  int lrc = 0x0;
  unsigned int wordTime;
  int wordLength;
  unsigned int baseLength;
  unsigned int lowBits = 0;
  unsigned int prevLowBits = 0;
  unsigned int bitMask = (1 << WC_FRACTIONAL_BITS) - 1;
  unsigned int val = 0;
  unsigned clk_cmd, clk_arg;
  timer tmr;
  unsigned int timestamp;
  int monitor_val = 0;
  
  media_ctl_register(media_ctl, num_in, input_fifos, 0, null, clk_ctl_index);

  clk_cmd = -1;
  while (clk_cmd != CLK_CTL_SET_RATE) 
    {
      slave {clk_ctl :> clk_cmd; clk_ctl :> clk_arg; };
      switch (clk_cmd)
        {
        case CLK_CTL_SET_RATE:  
          wordLength = clk_arg >> 1;
          baseLength = wordLength >> WC_FRACTIONAL_BITS;
          break;
        }
      break;
    }

  configure_clock_src(bclk, AUD_BCLK);

  for (int i=0;i<num_out>>1;i++) {
    configure_out_port_no_ready(p_aud_dout[i], bclk, 0);
    start_port(p_aud_dout[i]);
  }

  for (int i=0;i<num_in>>1;i++) {
    configure_in_port_no_ready(p_aud_din[i], bclk);
    start_port(p_aud_din[i]);
  }

  start_clock(bclk);

  samples_in <: 0;
  for (int i=0;i<num_out;i++) {
    samples_in :> int;
  }
  samples_in <: 0;

  for(int i=0;i<num_out>>1;i++) {
  	p_aud_dout[i] <: 0;
  }

  AUD_BCLK <: 0b00111111000000111111000000111111;
  AUD_BCLK <: 0b11110000001111110000001111110000;
  AUD_BCLK <: 0b00000011111100000011111100000011;

  AUD_BCLK <: 0b00111111000000111111000000111111;
  AUD_BCLK <: 0b11110000001111110000001111110000;
  AUD_BCLK <: 0b00000011111100000011111100000011;

  AUD_BCLK <: 0b00111111000000111111000000111111;
  AUD_BCLK <: 0b11110000001111110000001111110000;
  AUD_BCLK <: 0b00000011111100000011111100000011;

  AUD_BCLK <: 0b00111111000000111111000000111111;
  AUD_BCLK <: 0b11110000001111110000001111110000;
  AUD_BCLK <: 0b00000011111100000011111100000011;

  AUD_BCLK <: 0b11111111111111111111111111111111;


  AUD_LRCOUT <: 0 @ wordTime;
  wordTime += 100;
  AUD_LRCOUT @ wordTime <: lrc;
  wordTime -= baseLength-30;
  //  lrc = !lrc;
  while (1) {
	unsigned int active_fifos = media_input_fifo_enable_req_state();
    tmr :> timestamp;
    if (!lrc) {
      samples_in <: timestamp;
      i2s_diff = timestamp-i2s_ts;
      i2s_ts = timestamp;
    }
    wordTime += baseLength;

    for(int i=0;i<num_out>>1;i++) {
      val = 0;
      samples_in :> val;
      if (monitor_val == 2+lrc && !isnull(audio_monitor)) 
        audio_monitor <: val;
      val = val << 8;
      val = val << 2;
      val = bitrev(val);
      p_aud_dout[i] <: val;
    }
    
    AUD_BCLK @ (wordTime-6) <: 0b00111111000000111111000000111111;
    AUD_LRCOUT @ (wordTime) <: lrc;
    AUD_LRCIN @ (wordTime) <: lrc;

    AUD_BCLK <: 0b11110000001111110000001111110000;
    AUD_BCLK <: 0b00000011111100000011111100000011;

    AUD_BCLK <: 0b00111111000000111111000000111111;
    AUD_BCLK <: 0b11110000001111110000001111110000;
    AUD_BCLK <: 0b00000011111100000011111100000011;

    AUD_BCLK <: 0b00111111000000111111000000111111;
    AUD_BCLK <: 0b11110000001111110000001111110000;
    AUD_BCLK <: 0b00000011111100000011111100000011;

    AUD_BCLK <: 0b00111111000000111111000000111111;
    AUD_BCLK <: 0b11110000001111110000001111110000;
    AUD_BCLK <: 0b00000011111100000011111100000011;

    AUD_BCLK <: 0b11111111111111111111111111111111;

    for(int i=0;i<num_in>>1;i++) {
    	p_aud_din[i] :> val;
    	val = bitrev(val);
    	val <<= 3;
    	val >>= 8;
    	val &= 0xffffff;

    	if (active_fifos & (1 << (i+lrc))) {
          media_input_fifo_push_sample(input_fifos[i+lrc], val, timestamp);
    	} else {
          media_input_fifo_flush(input_fifos[i+lrc]);
    	}
        if (monitor_val == lrc && !isnull(audio_monitor)) 
          audio_monitor <: val;
    }


    select {
    case slave { clk_ctl :> clk_cmd; clk_ctl :> clk_arg; }:
      switch (clk_cmd)
        {
        case CLK_CTL_SET_RATE:
          wordLength = clk_arg >> 1;
          baseLength = wordLength >> WC_FRACTIONAL_BITS;
          break;
        }
      break;
    case monitor_ctl :> monitor_val:
        break;
    default:
      break;
    }

    lowBits = (lowBits + wordLength) & bitMask;
    if (lowBits <  prevLowBits) {
      wordTime += 1;
    }
    prevLowBits = lowBits;


    lrc = 1-lrc;

    media_input_fifo_update_enable_ind_state(active_fifos, 0xFFFFFFFF);
  }
}

void i2s_master_dyn_wordclk(struct i2s_resources &r_i2s,
                            chanend media_ctl,
                            chanend clk_ctl,
                            chanend monitor_ctl,
                            int clk_ctl_index,
                            streaming chanend samples_to_output,
                            media_input_fifo_t input_fifos[],
                            int num_in,
                            int num_out,
                            streaming chanend ?audio_monitor)
{
  i2s_master(r_i2s.bclk, r_i2s.AUD_BCLK, r_i2s.AUD_LRCOUT, r_i2s.AUD_LRCIN,
             r_i2s.p_aud_dout, r_i2s.p_aud_din,
             media_ctl, clk_ctl, monitor_ctl, clk_ctl_index, samples_to_output, 
             input_fifos, num_in,
             num_out, audio_monitor);
}
