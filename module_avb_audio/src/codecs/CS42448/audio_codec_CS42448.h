#ifndef _audio_codec_CS42448_h_
#define _audio_codec_CS42448_h_
#include "avb_conf.h"
#include "i2c.h"
#define CODEC_TDM 1
#define CODEC_I2S 2

/** Configure the cirrus logic CS42448 audio codec.
 * Configures for 48kHz audio.
 *
 * \param AUD_RESET_N - the codec reset signal
 * \param r_i2c       - Reference to structure containing the I2C ports
 * \param mode        - Either CODEC_I2S for 4xI2S, 48Khz, codec as slave or
 *                      CODEC_TDM for 1xTDM (I8S), 48Khz, codec as slave
 *
 */
void audio_codec_CS42448_init(out port AUD_RESET_N,
                        #if I2C_COMBINE_SCL_SDA
                              port r_i2c
                        #else
                              struct r_i2c &r_i2c
                        #endif
	                          ,int mode);



#endif
