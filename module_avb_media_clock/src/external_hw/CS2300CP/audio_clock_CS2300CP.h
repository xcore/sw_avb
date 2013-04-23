
#ifndef _audio_clock_CS2300CP_h_
#define _audio_clock_CS2300CP_h_
#include "i2c.h"

void audio_clock_CS2300CP_init(struct r_i2c &r_i2c, unsigned mclks_per_wordclk);

#ifdef AVB_PTP_GEN_DEBUG_CLK_IN_PLL_DRIVER
void audio_gen_CS2300CP_clock(out port p, chanend clk_ctl, chanend ptp_svr);
#else
void audio_gen_CS2300CP_clock(out port p, chanend clk_ctl);
#endif

#endif
