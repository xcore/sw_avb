/*
 * @ModuleName Audio DAC/ADC FIFO Module.
 * @Description: Implements the Audio DAC controller.
 *
 *
 */

#include <xs1.h>
#include <xclib.h>
#include <print.h>
#include <stdlib.h>
#include <syscall.h>
#include <xscope.h>
#include "media_fifo.h"

void media_output_fifo_to_xc_channel(chanend media_ctl,
                                     streaming chanend samples_out,    
                                     int clk_ctl_index,
                                     media_output_fifo_t output_fifos[],
                                     int num_channels)
{
  media_ctl_register(media_ctl, 0, null, num_channels, output_fifos,
                     clk_ctl_index);  

  while (1) {
    unsigned int size;
    unsigned timestamp;
    samples_out :> timestamp;
    for (int i=0;i<num_channels;i++) {
      unsigned sample;
      sample = media_output_fifo_pull_sample(output_fifos[i],
                                             timestamp);
      samples_out <: sample;
      
    }
  }
}


int mo_ts;
#pragma unsafe arrays
void 
media_output_fifo_to_xc_channel_split_lr(chanend media_ctl,
                                         streaming chanend samples_out,    
                                         int clk_ctl_index,
                                         media_output_fifo_t output_fifos[],
                                         int num_channels)
{
  mo_ts = 0xbadf00d;
  
  xscope_register(1, XSCOPE_DISCRETE, "Media Output FIFO", XSCOPE_UINT, "Samples");
  
  media_ctl_register(media_ctl, 0, null, num_channels, output_fifos,
                     clk_ctl_index);  

  while (1) {
    unsigned timestamp;
    samples_out :> timestamp;
    mo_ts = timestamp;
    for (int i=0;i<num_channels;i+=2) {
      unsigned sample;
      sample = media_output_fifo_pull_sample(output_fifos[i],
                                             timestamp);
      samples_out <: sample;
    }
    for (int i=1;i<num_channels;i+=2) {
      unsigned sample;
      sample = media_output_fifo_pull_sample(output_fifos[i],
                                             timestamp);
      samples_out <: sample;
    }
  }
}
