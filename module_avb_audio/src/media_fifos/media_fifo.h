#ifndef __media_fifo_h__
#define __media_fifo_h__
#include "media_output_fifo.h"
#include "media_input_fifo.h"

#ifdef __XC__
void media_ctl_register(chanend media_ctl,
                        int num_in,
                        media_input_fifo_t ?input_fifos[],
                        int num_out,
                        media_output_fifo_t ?output_fifos[],
                        int clk_ctl_index);
#endif

#endif // __media_fifo_h__
