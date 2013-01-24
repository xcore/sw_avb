
#ifndef _audio_clock_CS2100CP_h_
#define _audio_clock_CS2100CP_h_
#include "i2c.h"

void audio_clock_CS2100CP_init(
                        #if I2C_COMBINE_SCL_SDA
                            port r_i2c
                        #else
                            struct r_i2c &r_i2c
                        #endif
                        ,unsigned mclks_per_wordclk);

#endif
