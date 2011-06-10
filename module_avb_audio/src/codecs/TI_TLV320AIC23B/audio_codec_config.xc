/*
 * @ModuleName Two Wire control interface for TLV320AIC23B Audio CODEC.
 * @Description: Two Wire control interface for TLV320AIC23B Audio CODEC.
 *
 *
 *
 */

#include <xs1.h>
#include "audio_codec_config.h"
#include "print.h"
#include "i2c.h"

/*** Private Definations ***/

#define CTL_SCLK_PERIOD_LOW_TICKS      (1000)
#define CTL_SCLK_PERIOD_HIGH_TICKS     (1000)

// Device address and R/W fixed to write only.
// 0011_0100
#define DEVICE_ADRS                    (0x34)

// Valid register address.
#define LIVCTL_REG                     (0x00) // Left line input channel volume
#define RIVCTL_REG                     (0x01) // Right line input channel volume
#define LHPVCTL_REG                    (0x02) // Left channel headphone volume
#define RHPVCTL_REG                    (0x03) // Right channel headphone volume
#define AAPCTL_REG                     (0x04) // Analogue audio path
#define DAPCTL_REG                     (0x05) // Digital audio path
#define PDCTL_REG                      (0x06) // Power down
#define DAIF_REG                       (0x07) // Digital audio interface format
#define SRC_REG                        (0x08) // Sample rate
#define DIA_REG                        (0x09) // Digital interface activation
#define RESET_REG                      (0x0F) // Reset

// Register indexes
#define CODEC_LEFT_LINE_IN_CTL_REG     (0x0)
#define CODEC_RIGHT_LINE_IN_CTL_REG    (0x1)
#define CODEC_LEFT_HPONE_CTL_REG       (0x2)
#define CODEC_RIGHT_HPONE_CTL_REG      (0x3)
#define CODEC_ANALOGE_CTL_REG          (0x4)
#define CODEC_DIGITAL_CTL_REG          (0x5)
#define CODEC_POWER_DOWN_CTL_REG       (0x6)
#define CODEC_DIG_IF_FORMAT_CTL_REG    (0x7)
#define CODEC_SAMPLE_RATE_CTL_REG      (0x8)
#define CODEC_DIF_IF_ACT_CTL_REG       (0x9)
#define CODEC_RESET_REG                (0x0f)


/** This configures the Ti's TLV320AIC23B Audio Codec for followings:
 *  1. Crystal clock 12.288 MHz, 48kHz sample rate, 24bits per sample (left/right channels).
 *  2. Only ADC & DAC is enabled (all other inputs/outputs are muted).
 *
 *  \return    zero on success and non-zero on error.
 */
int audio_TI_TLV320AIC23B_init(struct r_i2c &r_i2c, int makeSlave)
{
   int Result;
   int error;

   struct i2c_data_info data;
   data.master_num = 0;
   data.data_len = 1;
   data.clock_mul = 1;

   i2c_master_init(r_i2c);

   error = 0;

   // write to codec with reset register.
   data.data[0] = 0;
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_RESET_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: Reset successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: Reset FAIL.\n");            
      error ++;
   }
   
   // Left Line input ctl
   // a. Simultaneous update    : Enable.
   // b. Left line input        : Normal.
   // c. Left line input volume : b1011 : 0db Default
   
   data.data[0] = 0b100010010;
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_LEFT_LINE_IN_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_LEFT_LINE_IN_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_LEFT_LINE_IN_CTL_REG FAIL.\n");
      error ++;                  
   }
   
   // Right Line input ctl
   // a. Simultaneous update    : Enable.
   // b. Right line input        : Normal.
   // c. Right line input volume : b1011 : 0db Default

   data.data[0] = 0b100010010;
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_RIGHT_LINE_IN_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_RIGHT_LINE_IN_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_RIGHT_LINE_IN_CTL_REG FAIL.\n");
      error ++;      
   }   
   
   // ***** NOTE *****
   // Left/Right HeadPhone interface is disabled.
   
   // Analoge Audio path contorl .
   // a. STA[2:0] & STE          : Side Tone disabled
   // b. DAC                     : DAC selected
   // c. Bypass                  : Disabled.
   // d. INSEL                   : Line Input.
   // e. MICM                    : Microphone muted.
   // f. MICB                    : 0db    
   data.data[0] = 0x12;
   //data.data[0] = 0x1A;    //Debugging bypass
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_ANALOGE_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_ANALOGE_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_ANALOGE_CTL_REG FAIL.\n");
      error ++;      
   }      
   
   // Digital Audio Path Control Reg.
   // a. DAC Soft Mute           : Disable
   // b. DEEMP[1:0] De-emphasis  : Disabled
   // c. ADC high pass filter    : Enable.
   data.data[0] = 0x0;
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_DIGITAL_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_DIGITAL_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_DIGITAL_CTL_REG FAIL.\n");
      error ++;      
   }      
      
   // Power Down Control register.
   // a. Device power            : ON
   // b. Clock                   : ON
   // c. Oscillator              : ON
   // d. Outputs                 : ON
   // e. DAC                     : ON
   // f. ADC                     : ON
   // g. MIC                     : OFF
   // h. LINE                    : ON
   data.data[0] = 0x2;
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_POWER_DOWN_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_POWER_DOWN_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_POWER_DOWN_CTL_REG FAIL.\n");
      error ++;
   }   
   
   // Digital Audio Interface Format control.s
   // a. Master/Slave            : MASTER for talker/SLAVE for listener
   // b. LRSWAP                  : Disabled.
   // c. LRP                     : 0
   // d. IWL[1:0] InputBitLength : 24bits
   // e. FOR[1:0] DataFormat     : I2S


   if (makeSlave)
	   data.data[0] = 0b0001010;
   else
	   data.data[0] = 0x4A;

   Result = i2c_master_tx(DEVICE_ADRS, CODEC_DIG_IF_FORMAT_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_DIG_IF_FORMAT_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_DIG_IF_FORMAT_CTL_REG FAIL.\n");
      error ++;      
   }         
      
   // Sample Rate Control register. (MCLK = 12.288 MHz and 48KHz for both ADC/DAC)
   // a. CLKOUT                  : MCLK
   // b. CLKIN                   : MCLK
   // c. SR[3:0]                 : 4'b0000
   // d. BOSR                    : 1'b0
   // e. USB/Normal              : Normal

   data.data[0] = 0b00000000; // 48KHz
   //data.data[0] = 0b000011100; // 96KHz
   //data.data[0] = 0b000011000; // 32KHz
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_SAMPLE_RATE_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_SAMPLE_RATE_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_SAMPLE_RATE_CTL_REG FAIL.\n");
      error ++;      
   }              
   
   // Digital Interface Activation reg.
   // a. ACT                     : Enable
   data.data[0] = 0x1;
   Result = i2c_master_tx(DEVICE_ADRS, CODEC_DIF_IF_ACT_CTL_REG, data, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_DIF_IF_ACT_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_DIF_IF_ACT_CTL_REG FAIL.\n");
      error ++;
   }            
   
   return (error);
}



