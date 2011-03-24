/*
 * @ModuleName Two Wire control interface for TLV320AIC23B Audio CODEC.
 * @Description: Two Wire control interface for TLV320AIC23B Audio CODEC.
 *
 *
 *
 */

#ifndef _AUDIO_CODEC_CONFIG_H_ 
#define _AUDIO_CODEC_CONFIG_H_ 1
#include "i2c.h"

/** This configure the Ti's TLV320AIC23B Audio Codec for followings:
 *  1. Crystal clock 12.288 MHz, 48kHz sample rate, 24bits per sample (left/right channels).
 *  2. Only ADC & DAC is enabled (all other inputs/outputs are muted).
 *
 *  \return    zero on success and non-zero on error.
 */
int audio_TI_TLV320AIC23B_init(struct r_i2c &r_i2c, int makeSlave);



#endif
