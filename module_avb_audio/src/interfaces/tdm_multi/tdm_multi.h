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
#include <simple_printf.h>
#include <assert.h>
#include <xscope.h>

// Each sample will be right aligned, with left 32-RESOLUTION bits zeroed out
// This exactly matches the 1722 packet format
#define RESOLUTION 24

#ifdef GEN_TEST_SIGNAL
extern unsigned samples[];
#endif

#ifdef CHECK_TEST_SIGNAL
extern unsigned prev_rx_samples[];
#endif

#ifdef CHECK_HW_LOOPBACK
extern unsigned hw_loopback_samples[];
#endif

void tdm_master_multi_configure_ports(const clock mclk,
        clock bclk,
        out port p_bclk,
        out buffered port:4 p_wclk,
        out buffered port:32 ?p_dout[],
        int num_out,
        in buffered port:32 ?p_din[],
        int num_in);

/** Input and output audio data using TDM format with the XCore acting 
 as master.

 This function implements a thread that can handle up to 8 channels on
 per wire. It inputs and outpus 24-bit data.

 The function will take input from the TDM interface and put the samples 
 directly into shared memory media input FIFOs. The output samples are 
 received over a channel. Every two word clock periods (i.e. once a
 sample) a timestamp is sent from this thread over the channel 
 and num_out samples are taken from the channel.

 This function can handle up to 8in and 8out at 48KHz. 

 TODO: correct params list
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
#pragma unsafe arrays
inline void tdm_master_multi(const clock mclk,
        clock bclk,
        out port p_bclk,
        out buffered port:4 p_wclk,
        out buffered port:32 ?p_dout[],
        int  num_chan_out,
        in buffered port:32 ?p_din[],
        int num_chan_in,
        int master_to_word_clock_ratio,
        streaming chanend ?c_listener,
        media_input_fifo_t ?input_fifos[],
        chanend media_ctl,
        int clk_ctl_index)  // Both in and out
{
    unsigned t;
    unsigned timestamp;
    timer tmr;
    unsigned num_dout = num_chan_out/TDM_NUM_CHANNELS;
    unsigned num_din = num_chan_in/TDM_NUM_CHANNELS;

#ifdef CHECK_TEST_SIGNAL
    unsigned check_active=0;
#endif

    //tmr :> t;
    // arbitrary delay to avoid ET_ILLEGAL_RESOURCE caused by xlog grabbing all the chanends
    //tmr when timerafter(t+30000) :> void;
    media_ctl_register(media_ctl, num_chan_in, input_fifos, 0, null, clk_ctl_index);

    c_listener <: 0;
    for (int n=0;n<num_chan_out;n++) {
        int x;
        c_listener :> x;
    }

    tdm_master_multi_configure_ports(mclk,
            bclk,
            p_bclk,
            p_wclk,
            p_dout,
            num_dout,
            p_din,
            num_din);


    if(num_din>0) {
        p_din[0] :> void @ t;
    } else if(num_dout>0) {
        p_dout[0] <: 0 @ t;
    } else {
        assert(0);
    }

    t += 64;

    for(int i=0; i<num_din; i++)
        asm("setpt res[%0], %1" : : "r"(p_din[i]), "r"(t + CLOCKS_PER_CHANNEL + 2));

    for(int i=0; i<num_dout; i++) {
        p_dout[i] @ (t + CLOCKS_PER_CHANNEL) <: 0;
        p_dout[i] <: 0;
    }

    t += 2 * CLOCKS_PER_CHANNEL + TDM_NUM_CHANNELS * CLOCKS_PER_CHANNEL;

    while (1)
    {
        unsigned int active_fifos = media_input_fifo_enable_req_state();
        tmr :> timestamp;
        c_listener <: timestamp;
        for (int n = 0; n < TDM_NUM_CHANNELS; n++)
        {
            unsigned x;
            for(int i=0; i<num_dout; i++) {
                c_listener :> x;
#ifdef USE_XSCOPE_PROBES
                xscope_probe_data(22, x);
#endif
#ifdef CHECK_TEST_SIGNAL
                {
                    unsigned chan_idx = i*TDM_NUM_CHANNELS+n;
                    if(check_active) {
                        if(x != prev_rx_samples[chan_idx]+1) {
                            simple_printf("ERROR: CHECK_TEST_SIGNAL received invalid sample. expected %x, actual %x, previous %x\n",prev_rx_samples[chan_idx]+1,x,prev_rx_samples[chan_idx]);
                            assert(0);
                        }
                    } else {
                        if(prev_rx_samples[chan_idx] != x) {
                            check_active=1;
                            simple_printf("Activated Sample Stream Loopback Selfcheck....\n");
                        }
                    }
                    prev_rx_samples[chan_idx] = x;
                }
#endif
#ifdef CHECK_HW_LOOPBACK
                {
                    unsigned chan_idx = i*TDM_NUM_CHANNELS+n;
                    hw_loopback_samples[chan_idx] = x;
                }
#endif

                p_dout[i] <: bitrev(x << (32 - RESOLUTION)) & 0xffffff;
            }
            for(int i=0; i<num_din; i++) {
                unsigned chan_idx = i*TDM_NUM_CHANNELS+n;

                p_din[i] :> x;

#ifdef GEN_TEST_SIGNAL
                x = samples[chan_idx]++;
#endif
#ifdef CHECK_HW_LOOPBACK
                {
                    unsigned chan_idx = i*TDM_NUM_CHANNELS+n;
                    if(x != hw_loopback_samples[chan_idx]) {
                        simple_printf("ERROR: Invalid hardware loopback sample on line %d. expected %x, actual %x\n",i,hw_loopback_samples[chan_idx],x);
                        assert(0);
                    }
                }
#endif

                x = (bitrev(x) >> (32-RESOLUTION)) & 0xffffff;

                if (active_fifos & (1 << chan_idx)) {
                    media_input_fifo_push_sample(input_fifos[chan_idx], x, timestamp);
                } else {
                    media_input_fifo_flush(input_fifos[chan_idx]);
                }

            }
        }
        t += TDM_NUM_CHANNELS * CLOCKS_PER_CHANNEL;
        p_wclk @ t <: 0b0001;
        media_input_fifo_update_enable_ind_state(active_fifos, 0xFFFFFFFF);
    }
}




// loop back inputs to outputs - for testing
void tdm_loopback(clock b_mck, in port p_mck, out port p_bck, 
                  out buffered port:4 p_wck,
                  in buffered port:32 p_din, out buffered port:32 p_dout);


#endif
