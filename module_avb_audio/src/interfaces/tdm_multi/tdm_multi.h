/*
 * @ModuleName IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 * @Description: Audio Codec TDM Ctl module.
 *
 */
 
#ifndef __tdm_h__
#define __tdm_h__

#include "avb_conf.h"
#include "media_fifo.h"
#include <xclib.h>

/** Input and output audio data using TDM format with the XCore acting 
 as master.

 This function implements a thread that can handle up to 8 channels on
 a single wire. It inputs and outpus 24-bit data.

 The function will take input from the TDM interface and put the samples 
 directly into shared memory media input FIFOs. The output samples are 
 received over a channel. Every two word clock periods (i.e. once a
 sample) a timestamp is sent from this thread over the channel 
 and num_out samples are taken from the channel.

 This function can handle up to 8in and 8out at 48KHz. 

  \param b_mck      clock block that clocks the system clock of the codec
  \param p_mck      the input system clock port
  \param p_bck      clock block that clocks the bit clock; configured 
                    within the i2s_master function
  \param p_bck      the port to output the bit clock to
  \param p_wck      the port to output the word clock to
  \param p_dout     array of ports to output data to
  \param p_din      array of ports to input data from
  \param num_channels     number of input and output ports
  \param c_listener  chanend connector to a listener component
  \param input_fifos           a map from the inputs to local talker streams.
                               The channels of the inputs are interleaved,
							   for example, if you have two input ports, the map
                               {0,1,0,1} would map to the two stereo local
                               talker streams 0 and 1.
  \param media_ctl the media control channel
  \param clk_ctl_index the index of the clk_ctl channel array that
                       controls the master clock fo the codec
 */
void tdm_master_multi(const clock b_mck,
        clock bclk,
        out port p_bck,
        out buffered port:4 p_wck,
        out buffered port:32 ?p_dout[],
        int  num_chan_out,
        in buffered port:32 ?p_din[],
        int num_chan_in,
        int master_to_word_clock_ratio,
        streaming chanend ?c_listener,
        media_input_fifo_t ?input_fifos[],
        chanend media_ctl,
        int clk_ctl_index);  // Both in and out

// loop back inputs to outputs - for testing
void tdm_loopback(clock b_mck, in port p_mck, out port p_bck, 
                  out buffered port:4 p_wck,
                  in buffered port:32 p_din, out buffered port:32 p_dout);


#endif
