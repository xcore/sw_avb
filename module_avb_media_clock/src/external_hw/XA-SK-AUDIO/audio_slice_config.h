
#ifndef _audio_clock_CS2100CP_h_
#define _audio_clock_CS2100CP_h_
#include "i2c.h"

void init_slicekit_audio(port p_i2c, out port p_audio_shared, unsigned mclks_per_wordclk);

void audio_gen_slicekit_clock(out port p, chanend clk_ctl);

#endif
