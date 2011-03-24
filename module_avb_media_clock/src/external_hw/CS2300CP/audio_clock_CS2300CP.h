
#ifndef _audio_clock_CS2300CP_h_
#define _audio_clock_CS2300CP_h_
#include "i2c.h"

void audio_clock_CS2300CP_init(struct r_i2c &r_i2c, unsigned mult);

void audio_gen_CS2300CP_clock(out port p, 
                              chanend clk_ctl,
                              unsigned mult);

#endif
