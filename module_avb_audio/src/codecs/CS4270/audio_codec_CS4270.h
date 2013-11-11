#ifndef _audio_codec_CS42448_h_
#define _audio_codec_CS42448_h_
#include "avb_conf.h"
#include "i2c.h"

void audio_codec_CS4270_init(out port p_codec_reset,
                              int mask,
                              int codec_addr,
                        #if I2C_COMBINE_SCL_SDA
                              port r_i2c
                        #else
                              struct r_i2c &r_i2c
                        #endif
                              );



#endif
