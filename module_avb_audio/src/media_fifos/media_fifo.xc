#include <xs1.h>
#include "media_fifo.h"

void media_ctl_register(chanend media_ctl,
                        media_input_fifo_t (&?input_fifos)[num_in],
                        unsigned num_in,
                        media_output_fifo_t (&?output_fifos)[num_out],
                        unsigned num_out,
                        int clk_ctl_index)
{
  unsigned tile_id;
  tile_id = get_local_tile_id();
  media_ctl <: tile_id;
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
}

