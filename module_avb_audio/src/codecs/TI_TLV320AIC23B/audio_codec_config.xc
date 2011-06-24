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
#include "i2c_tlv320.h"

// Device address and R/W fixed to write only.
// 0011_0100
#define DEVICE_ADRS                    (0x34)

// Register indexes
#define CODEC_LEFT_LINE_IN_CTL_REG     (0x00) // Left line input channel volume
#define CODEC_RIGHT_LINE_IN_CTL_REG    (0x01) // Right line input channel volume
#define CODEC_LEFT_HPONE_CTL_REG       (0x02) // Left channel headphone volume
#define CODEC_RIGHT_HPONE_CTL_REG      (0x03) // Right channel headphone volume
#define CODEC_ANALOGE_CTL_REG          (0x04) // Analogue audio path
#define CODEC_DIGITAL_CTL_REG          (0x05) // Digital audio path
#define CODEC_POWER_DOWN_CTL_REG       (0x06) // Power down
#define CODEC_DIG_IF_FORMAT_CTL_REG    (0x07) // Digital audio interface format
#define CODEC_SAMPLE_RATE_CTL_REG      (0x08) // Sample rate
#define CODEC_DIF_IF_ACT_CTL_REG       (0x09) // Digital interface activation
#define CODEC_RESET_REG                (0x0f) // Reset

static unsigned i2c_tx(unsigned int reg, unsigned int data, struct r_i2c &r_i2c)
{
	return i2c_tlv320_tx(r_i2c, DEVICE_ADRS, reg, data);
}

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

   i2c_master_init(r_i2c);

   error = 0;

   // write to codec with reset register.
   Result = i2c_tx(CODEC_RESET_REG, 0, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: Reset successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: Reset FAIL.\n");            
      error ++;
   }
   
   // Left Line input ctl
   // a. Simultaneous update    : Enable.
   // b. Left line input        : Normal.
   // c. Left line input volume : b10111 : 0db Default
   Result = i2c_tx(CODEC_LEFT_LINE_IN_CTL_REG, 0b100010111, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_LEFT_LINE_IN_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_LEFT_LINE_IN_CTL_REG FAIL.\n");
      error ++;                  
   }
   
   // Right Line input ctl
   // a. Simultaneous update    : Enable.
   // b. Right line input        : Normal.
   // c. Right line input volume : b10111 : 0db Default
   Result = i2c_tx(CODEC_RIGHT_LINE_IN_CTL_REG, 0b100010111, r_i2c);
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
   Result = i2c_tx(CODEC_ANALOGE_CTL_REG, 0b000010010, r_i2c);
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
   Result = i2c_tx(CODEC_DIGITAL_CTL_REG, 0, r_i2c);
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
   Result = i2c_tx(CODEC_POWER_DOWN_CTL_REG, 0b000000010, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_POWER_DOWN_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_POWER_DOWN_CTL_REG FAIL.\n");
      error ++;
   }   
   
   // Digital Audio Interface Format control.
   // a. Master/Slave            : MASTER for talker/SLAVE for listener
   // b. LRSWAP                  : Disabled.
   // c. LRP                     : 0
   // d. IWL[1:0] InputBitLength : 24bits
   // e. FOR[1:0] DataFormat     : I2S
   Result = i2c_tx(CODEC_DIG_IF_FORMAT_CTL_REG, (makeSlave ? 0b000001010 : 0b001001010), r_i2c);
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
   Result = i2c_tx(CODEC_SAMPLE_RATE_CTL_REG, 0b00000000, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_SAMPLE_RATE_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_SAMPLE_RATE_CTL_REG FAIL.\n");
      error ++;      
   }              
   
   // Digital Interface Activation reg.
   // a. ACT                     : Enable
   Result = i2c_tx(CODEC_DIF_IF_ACT_CTL_REG, 0x1, r_i2c);
   if (Result == 1) {
      printstr("AudioCodec: CODEC_DIF_IF_ACT_CTL_REG successful.\n");      
   } else {
      printstr("AudioCodec: ERROR: CODEC_DIF_IF_ACT_CTL_REG FAIL.\n");
      error ++;
   }            
   
   return (error);
}



