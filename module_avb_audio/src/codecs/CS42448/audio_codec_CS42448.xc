
#include <xs1.h>
#include <xclib.h>
#include <stdlib.h>
#include <print.h>
#include "audio_codec_CS42448.h"
#include "print.h"
#include "i2c.h"

/*** Private Definitions ***/
#define CTL_SCLK_PERIOD_LOW_TICKS      	(1000)
#define CTL_SCLK_PERIOD_HIGH_TICKS     	(1000)

#define DEVICE_ADRS                    	(0x90)

#define CODEC_ID						(0x01)
#define CODEC_POWER_CONTROL				(0x02)
#define CODEC_FUNCTIONAL_MODE          	(0x03)
#define CODEC_INTERFACE_FORMATS        	(0x04)
#define CODEC_ADC_CONTROL              	(0x05)
#define CODEC_TRANSITION_CONTROL		(0x06)
#define CODEC_CHANNEL_MUTE				(0x07)
#define CODEC_VOL_AOUT_1				(0x08)
#define CODEC_VOL_AOUT_2				(0x09)
#define CODEC_VOL_AOUT_3				(0x0a)
#define CODEC_VOL_AOUT_4				(0x0b)
#define CODEC_VOL_AOUT_5				(0x0c)
#define CODEC_VOL_AOUT_6				(0x0d)
#define CODEC_VOL_AOUT_7				(0x0e)
#define CODEC_VOL_AOUT_8				(0x0f)
#define CODEC_DAC_INVERT_CHANNEL		(0x10)
#define CODEC_VOL_AIN_1					(0x11)
#define CODEC_VOL_AIN_2					(0x12)
#define CODEC_VOL_AIN_3					(0x13)
#define CODEC_VOL_AIN_4					(0x14)
#define CODEC_VOL_AIN_5					(0x15)
#define CODEC_VOL_AIN_6					(0x16)
#define CODEC_ADC_INVERT_CHANNEL		(0x17)
#define CODEC_STATUS_CONTROL			(0x18)
#define CODEC_STATUS					(0x19)
#define CODEC_STATUS_MASK				(0x1a)
#define CODEC_MUTEC						(0x1b)


static unsigned REGWR(unsigned reg, unsigned val,
                  #if I2C_COMBINE_SCL_SDA
                     port r_i2c
                  #else
                     struct r_i2c &r_i2c
                  #endif
                     )
{
  unsigned char data[1];
	data[0] = val;

	return i2c_master_write_reg(DEVICE_ADRS, reg, data, 1, r_i2c);
}

static const char error_msg[] = "CS42448 Config Failed";

void audio_codec_CS42448_init(out port p_reset,
                           #if I2C_COMBINE_SCL_SDA
                               port r_i2c
                           #else
                               struct r_i2c &r_i2c
                           #endif
                              ,int mode)
{
   timer t;
   unsigned int time;
   unsigned res = 1;

   i2c_master_init(r_i2c);

   // Reset the codec
   p_reset <: 0;
   t :> time;
   t when timerafter(time + 100000) :> time;
   p_reset <: 1;

   if (mode == CODEC_TDM) {
	   // DAC_FM = 0 (single speed)
	   // ADC_FM = 0 (single speed)
	   // MFREQ = 2 (MCLK = /512 - 48Khz)
	   res = REGWR(CODEC_FUNCTIONAL_MODE, 0b11111000, r_i2c);
   } else {
	   // Default to I2S
	   res = REGWR(CODEC_FUNCTIONAL_MODE, 0b00000100, r_i2c);
   }

   if (res == 0) {
	   printstr(error_msg);
	   return;
   }

   if (mode == CODEC_TDM) {
	   // FREEZE = 0
	   // AUX_DIF = 0 - left justified
	   // DAC_DIF=1 (I2S)
	   // ADC_DIF = 1 (I2S)
	   res = REGWR(CODEC_INTERFACE_FORMATS, 0b00110110, r_i2c);
   } else  {
	   // Left justified for DAC and ADC
	   res = REGWR(CODEC_INTERFACE_FORMATS, 0b00000000, r_i2c);
   }

   if (res == 0) {
	   printstr(error_msg);
	   return;
   }

   // ADC1-2_HPF FREEZE = 0
   // ADC3_HPF FREEZE = 0
   // DAC_DEM = 0
   // ADC1_SINGLE = 1(single ended)
   // ADC2_SINGLE = 1
   // ADC3_SINGLE = 1
   // AIN5_MUX = 0
   // AIN6_MUX = 0
   res = REGWR(CODEC_ADC_CONTROL, 0b00011100, r_i2c);
   if (res == 0) {
	   printstr(error_msg);
	   return;
   }

   // -10dB attenuation for input
   res = REGWR(CODEC_ADC_CONTROL, 0b11101100, r_i2c);
   if (res == 0) {
	   printstr(error_msg);
	   return;
   }

   // Use same volume control for all inputs and all outputs
   res = REGWR(CODEC_TRANSITION_CONTROL, 0b10110101, r_i2c);
   if (res == 0) {
	   printstr(error_msg);
	   return;
   }

   // -3dB attenuation on inputs
   res = REGWR(CODEC_VOL_AIN_1, 0b11111010, r_i2c);
   if (res == 0) {
	   printstr(error_msg);
	   return;
   }
}
