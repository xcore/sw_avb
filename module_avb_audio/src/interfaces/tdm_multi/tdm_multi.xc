#include <xs1.h>
#include <xclib.h>
#include "tdm_multi.h"
#include "media_input_fifo.h"
#include "media_fifo.h"

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
        out buffered port:32 ?p_dout[],
        int num_out,
        in buffered port:32 ?p_din[],
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
unsigned samples[AVB_NUM_MEDIA_INPUTS];
#endif

#ifdef CHECK_TEST_SIGNAL
unsigned prev_rx_samples[AVB_NUM_MEDIA_OUTPUTS];
#endif

#ifdef CHECK_HW_LOOPBACK
#define NUM_LOOPBACK_SAMPLES (AVB_NUM_MEDIA_OUTPUTS >= AVB_NUM_MEDIA_INPUTS) ? AVB_NUM_MEDIA_OUTPUTS : AVB_NUM_MEDIA_INPUTS
unsigned hw_loopback_samples[NUM_LOOPBACK_SAMPLES];
#endif

extern inline void tdm_master_multi(const clock mclk,
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
        int clk_ctl_index);  // Both in and out



