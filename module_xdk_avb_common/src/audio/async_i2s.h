#ifndef _async_i2s_h_
#define _async_i2s_h_
#include "media_fifo.h"

struct i2s_resources {
  clock bclk;
  out buffered port:32 AUD_BCLK;
  out port AUD_LRCOUT;
  out port AUD_LRCIN;
  out buffered port:32 p_aud_dout[1];
  in buffered port:32 p_aud_din[1];
};


/** Runs an I2S output with the XCore as master and the clocks derived 
 *  from the XCore internal clock (i.e. asynchronously to the CODEC 
 *  master clock). This will cause noise in the CODEC (and in some CODECs this
 *  will simply not work).
 *  Note that the bitclock is not distributed evenly over the word clock period.
 * 
 */
void i2s_master_dyn_wordclk(struct i2s_resources &r_i2s,
                            chanend media_ctl,
                            chanend clk_ctl,
                            chanend monitor_ctl,
                            int clk_ctl_index,
                            streaming chanend samples_to_output,
                            media_input_fifo_t ififos[],
                            int num_in,
                            int num_out,
                            streaming chanend ?audio_monitor);

#endif
