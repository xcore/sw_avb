#include <xs1.h>
#include <xclib.h>
#include "tdm_multi.h"
#include "media_input_fifo.h"
#include "media_fifo.h"
#include <simple_printf.h>
#include <assert.h>
#include <xscope.h>

// Must be a multiple of 4
#define CLOCKS_PER_CHANNEL 32

// Each sample will be right aligned, with left 32-RESOLUTION bits zeroed out
// This exactly matches the 1722 packet format
#define RESOLUTION 24

static void tdm_loopback_aux(
        clock mclk, in port p_mck, out port p_bclk,
        out buffered port:4 p_wclk,
        in buffered port:32 p_din, out buffered port:32 p_dout,
        streaming chanend c_adc, streaming chanend c_dac)
{
    unsigned t;
    int first = 2;

    // Master clock is slaved from external clock chip
    set_clock_src(mclk, p_mck);
    set_port_clock(p_bclk, mclk);
    set_port_mode_clock(p_bclk);
    set_port_clock(p_wclk, mclk);
    set_port_clock(p_din, mclk);
    set_port_clock(p_dout, mclk);
    start_clock(mclk);


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
        p_wclk @ (t - 1) <: 0b0001;
    }
}


void tdm_loopback(  clock mclk, in port p_mck, out port p_bclk,
        out buffered port:4 p_wclk,
        in buffered port:32 p_din, out buffered port:32 p_dout)
{
    streaming chan c_adc, c_dac;
    par {
        tdm_loopback_aux(mclk, p_mck, p_bclk, p_wclk, p_din, p_dout, c_adc, c_dac);
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

void tdm_master_multi_configure_ports(const clock mclk,
        clock bclk,
        out port p_bclk,
        out buffered port:4 p_wclk,
        out buffered port:32 p_dout[],
        int num_out,
        in buffered port:32 p_din[],
        int num_in)
{

    // Master clock is slaved from external clock chip
    // done upstairs: set_clock_src(mclk, p_mck);

    // generate p_bclk from mclk
    set_port_clock(p_bclk, mclk);
    set_port_mode_clock(p_bclk);

    set_port_clock(p_wclk, mclk);
    //set_port_clock(p_din, mclk);
    //set_port_clock(p_dout, mclk);
    // done upstairs: start_clock(mclk);

    configure_out_port_no_ready(p_wclk, mclk, 0);
    start_port(p_wclk);

    for (int i=0;i<num_out;i++) {
        configure_out_port_no_ready(p_dout[i], mclk, 0);
        start_port(p_dout[i]);
    }

    for (int i=0;i<num_in;i++) {
        configure_in_port_no_ready(p_din[i], mclk);
        start_port(p_din[i]);
    }

    clearbuf(p_wclk);
    clearbuf(p_bclk);

    for (int i = 0; i < num_in; i++) {
        clearbuf(p_din[i]);
    }
    for (int i = 0; i < num_out; i++) {
        clearbuf(p_dout[i]);
    }
    // done upstairs start_clock(bclk);
    return;
}

#ifdef GEN_TEST_SIGNAL
static unsigned samples[AVB_NUM_MEDIA_INPUTS];
#endif

#ifdef CHECK_TEST_SIGNAL
static unsigned prev_rx_samples[AVB_NUM_MEDIA_OUTPUTS];
#endif

#pragma unsafe arrays
void tdm_master_multi(const clock mclk,
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


    p_din[0] :> void @ t;

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
#ifdef USE_XSCOPE
                xscope_probe_data(22, x);
#endif
#ifdef CHECK_TEST_SIGNAL
                {
                    unsigned chan_idx = i*TDM_NUM_CHANNELS+n;
                    if(check_active) {
                        if(x != prev_rx_samples[chan_idx]+1) {
                            simple_printf("ERROR: Sample expected %x, actual %x, previous %x\n",prev_rx_samples[chan_idx]+1,x,prev_rx_samples[chan_idx]);
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
                p_dout[i] <: bitrev(x << (32 - RESOLUTION)) & 0xffffff;
            }
            for(int i=0; i<num_din; i++) {
                unsigned chan_idx = i*TDM_NUM_CHANNELS+n;

                p_din[i] :> x;

#ifdef GEN_TEST_SIGNAL
                x = samples[chan_idx]++;
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


