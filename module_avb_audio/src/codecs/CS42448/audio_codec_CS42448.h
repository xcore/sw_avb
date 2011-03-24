#ifndef _audio_codec_CS42448_h_
#define _audio_codec_CS42448_h_
#include "i2c.h"
#define CODEC_TDM 1
#define CODEC_I2S 2

/** Configure the cirrus logic CS42448 audio codec. 
 * Configures for 48kHz audio.
 * 
 * \param AUD_RESET_N - the codec reset signal
 * \param AUD_SCLK    - I2C sclk signal
 * \param AUD_SDIN    - I2C sdin signal
 * \param mode        - Either CODEC_I2S for 4xI2S, 48Khz, codec as slave or
 *                      CODEC_TDM for 1xTDM (I8S), 48Khz, codec as slave        
 *                      
 */
void audio_codec_CS42448_init(out port AUD_RESET_N, 
                              struct r_i2c &r_i2c,
	                      int mode);



#endif 
