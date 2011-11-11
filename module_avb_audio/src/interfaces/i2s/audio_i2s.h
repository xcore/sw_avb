#ifndef __AUDIO_I2S_CTL_H__
#define __AUDIO_I2S_CTL_H__ 1

/*
  This unit provides functionality to communicate from AVB media fifos to 
  an audio codec using the I2S digital audio interface format.
*/

#include "avb_conf.h"
#include "media_fifo.h"
#include <xclib.h>
#ifdef __XC__

// By defining this, all channels are filled with an increasing counter
// value instead of the samples themselves.  Useful to check the channel
// synchronization using a network monitor
//#define SAMPLE_COUNTER_TEST

#define I2S_SINE_TABLE_SIZE 100

extern unsigned int i2s_sine[I2S_SINE_TABLE_SIZE];

void i2s_master_configure_ports(const clock mclk,
                                clock bclk,
                                out buffered port:32 p_bclk,
                                out buffered port:32 p_lrclk,
                                out buffered port:32 p_dout[],
                                int num_out,
                                in buffered port:32 p_din[],             
                                int num_in);

/** Input and output audio data using I2S format with the XCore acting 
 as master.

 This function implements a thread that can handle several synchronous 
 I2S interfaces. It inputs and outputs 24-bit data packed into 32 bits.

 The function will take input from the I2S interface and put the samples 
 directly into shared memory media input FIFOs. The output samples are 
 received over a channel. Every two word clock periods (i.e. once a
 sample) a timestamp is sent from this thread over the channel 
 and num_out samples are taken from the channel.

 The master clock is generated externally by the PLL. A clock block clocks
 the Bit clock port (aka serial clock or sclk), from the master clock, and we
 write out a clock pattern on this port to generate the correct divided
 BCLK.  Likewise, a clock block puts the BCLK as the clock for the LRCLK port.
 We also write out data to the LRCLK port to generate a correct LRCLK pattern.


 This function can handle up to 8in and 8out at 48KHz. 

  \param mclk      clock block that clocks the system clock of the codec;  
                   needs to be configured before the function call
  \param bclk      clock block that clocks the bit clock; configured
                   within the i2s_master function
  \param p_bclk    the port to output the bit clock to
  \param p_lrclk   the port to output the word clock to
  \param p_dout    array of ports to output data to
  \param num_out   number of output ports
  \param p_din     array of ports to input data from
  \param num_in    number of input ports
  \param master_to_word_clock_ratio  the ratio of the master clock
                                     to the word clock; must be one
                                     of 128, 256 or 512
  \param c_listener  chanend connector to a listener component
  \param input_fifos           a map from the inputs to local talker streams.
                               The channels of the inputs are interleaved,
							   for example, if you have two input ports, the map
                               {0,1,0,1} would map to the two stereo local
                               talker streams 0 and 1.
  \param media_ctl the media fifo control channel
  \param clk_ctl_index the index of the clk_ctl channel array that
                       controls the master clock fo the codec
 */
#pragma unsafe arrays
inline void i2s_master(const clock mclk,
                       clock bclk,
                       out buffered port:32 p_bclk,
                       out buffered port:32 p_lrclk,
                       out buffered port:32 p_dout[],
                       int num_out,
                       in buffered port:32 p_din[],
                       int num_in,
                       int master_to_word_clock_ratio,
                       streaming chanend ?c_listener,
                       media_input_fifo_t ?input_fifos[],
                       chanend media_ctl,
                       int clk_ctl_index)
{
  int mclk_to_bclk_ratio = master_to_word_clock_ratio / 64;
  unsigned int bclk_val;
  unsigned int lrclk_val = 0;

#ifdef SAMPLE_COUNTER_TEST
  unsigned int sample_counter=0;
#endif

  // This is the master timing clock for the audio system.  Its value is sent
  // to the input and output fifos and is converted into presentation time for
  // clock recovery.
  timer tmr;

#ifdef I2S_SYNTH_FROM
  int sine_count[8] = {0};
  int sine_inc[8] = {0x080, 0x100, 0x180, 0x200, 0x100, 0x100, 0x100, 0x100};
#endif
  media_ctl_register(media_ctl, num_in, input_fifos, 0, null, clk_ctl_index);

  // You can output 32 mclk ticks worth of bitclock at a time.
  // So the ratio between the master clock and the word clock will affect 
  // how many bitclocks outputs you have to do per word and also the 
  // length of the bitclock w.r.t the master clock.
  // In every case you will end up with 32 bit clocks per word.
  switch (mclk_to_bclk_ratio)
    {
    case 2:
      bclk_val = 0xaaaaaaaa; // 10
      break;
    case 4: 
      bclk_val = 0xcccccccc; // 1100
      break;
    case 8:
      bclk_val = 0xf0f0f0f0; // 11110000
      break;
    default:
      // error - unknown master clock/word clock ratio
      return;
    }

  i2s_master_configure_ports(mclk,
                             bclk,
                             p_bclk,
                             p_lrclk,
                             p_dout,
                             num_out>>1,
                             p_din,
                             num_in>>1);

  

  // This sections aligns the ports so that the dout/din ports are 
  // inputting and outputting in sync,
  // setting the t variable at the end sets when the lrclk will change
  // w.r.t to the bitclock.

  for (int i=0;i<num_out>>1;i++) 
    p_dout[i] @ 32 <: 0;

  for (int i=0;i<num_in>>1;i++) 
    asm ("setpt res[%0], %1" : : "r"(p_din[i]), "r"(63));

  p_lrclk @ 31 <: 0;

  for (int j=0;j<2;j++) {
    for (int i=0;i<mclk_to_bclk_ratio;i++)  {
      p_bclk <: bclk_val;
    }  
  }


  for (int i=0;i<num_out>>1;i++) 
    p_dout[i] <: 0;

  // the unroll directives in the following loops only make sense if this
  // function is inlined into a more specific version
  while (1) {

	  unsigned int timestamp;

	  unsigned int active_fifos = media_input_fifo_enable_req_state();

#pragma xta label "i2s_master_loop"

#ifdef I2S_SYNTH_FROM
    for (int k=I2S_SYNTH_FROM;k<num_in>>1;k++) {
      sine_count[k] += sine_inc[k];
      if (sine_count[k] > I2S_SINE_TABLE_SIZE * 256)
        sine_count[k] -= I2S_SINE_TABLE_SIZE * 256;
    }
#endif

    tmr :> timestamp;
    if (num_out > 0)
    {
    	c_listener <: timestamp;
    }

    for (int j=0;j<2;j++) {
#pragma xta endpoint "i2s_master_lrclk_output"
    	// This assumes that there are 32 BCLKs in one half of an LRCLK
    	p_lrclk <: lrclk_val;
    	lrclk_val = ~lrclk_val;

#pragma loop unroll    
      for (int k=0;k<mclk_to_bclk_ratio;k++)  {

#pragma xta endpoint "i2s_master_bclk_output"
        p_bclk <: bclk_val;

        if (k < num_in>>1) {
#ifdef SAMPLE_COUNTER_TEST
        	if (active_fifos & (1 << (j+k*2))) {
			  media_input_fifo_push_sample(input_fifos[j+k*2], sample_counter, timestamp);
        	} else {
        	  media_input_fifo_flush(input_fifos[j+k*2]);
        	}
#else
            unsigned int sample_in;
#pragma xta endpoint "i2s_master_sample_input"
            asm("in %0, res[%1]":"=r"(sample_in):"r"(p_din[k]));

            sample_in = (bitrev(sample_in) >> 8);
#ifdef I2S_SYNTH_FROM
            if (k >= I2S_SYNTH_FROM) {
              sample_in = i2s_sine[sine_count[k]>>8];
            }
#endif
            if (active_fifos & (1 << (j+k*2))) {
			  media_input_fifo_push_sample(input_fifos[j+k*2], sample_in, timestamp);
            } else {
              media_input_fifo_flush(input_fifos[j+k*2]);
            }
#endif
        }
        
        if (k < num_out>>1) {
          unsigned int sample_out;
          c_listener :> sample_out;
          sample_out = bitrev(sample_out << 8);
#pragma xta endpoint "i2s_master_sample_output"
          p_dout[k] <: sample_out;
        }
        
      }
    }

#ifdef SAMPLE_COUNTER_TEST
    sample_counter++;
#endif

    media_input_fifo_update_enable_ind_state(active_fifos, 0xFFFFFFFF);
  }
}




#endif

#endif

