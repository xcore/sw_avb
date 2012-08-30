#include <xs1.h>
#include "media_fifo.h"
#include "get_core_id_from_chanend.h"

void media_ctl_register(chanend media_ctl,
                        int num_in, 
                        media_input_fifo_t ?input_fifos[], 
                        int num_out, 
                        media_output_fifo_t ?output_fifos[],
                        int clk_ctl_index)
{
  unsigned core_id;
  core_id = get_core_id_from_chanend(media_ctl);
  media_ctl <: core_id;
  media_ctl <: clk_ctl_index;
  media_ctl <: num_in;
  for (int i=0;i<num_in;i++) {
    int stream_num;
    media_ctl :> stream_num;
    media_ctl <: input_fifos[i];
    media_input_fifo_init(input_fifos[i], stream_num);
  }
  media_ctl <: num_out;
  for (int i=0;i<num_out;i++) {
    int stream_num;
    media_ctl :> stream_num;
    media_ctl <: output_fifos[i];

    media_output_fifo_init(output_fifos[i], stream_num);
  }

  return;
}

