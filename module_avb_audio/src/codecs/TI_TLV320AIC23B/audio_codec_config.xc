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
#define LIVCTL_REG                     (0x00)
#define RIVCTL_REG                     (0x01)
#define LHPVCTL_REG                    (0x02)
#define RHPVCTL_REG                    (0x03)
#define AAPCTL_REG                     (0x04)
#define DAPCTL_REG                     (0x05)
#define PDCTL_REG                      (0x06)
#define DAIF_REG                       (0x07)
#define SRC_REG                        (0x08)
#define DIA_REG                        (0x09)
#define RESET_REG                      (0x0F)

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

/** This send simple write command to the DAC chip via 2wire interface.
 */
int swc_audio_codec_ctl_reg_wr(port AUD_SCLK, port AUD_SDIN,
                               int Adrs, int WrData)
{
   int Result;
   timer gt;
   unsigned time;
   int Temp, CtlAdrsData, i;
   // three device ACK
   int DeviceACK[3]; 
   
   // sanity checking
   // only 9bits of data.
   if ((WrData & 0xFFFFFE00) != 0)
   {
      return(0);
   }
   // only valid address.
   switch (Adrs)
   {
      case LIVCTL_REG: case RIVCTL_REG: case LHPVCTL_REG: case RHPVCTL_REG:
      case AAPCTL_REG: case DAPCTL_REG: case PDCTL_REG: case DAIF_REG:
      case SRC_REG: case DIA_REG: case RESET_REG:
         break;
      default:
         return(0);      
         break;
   }   
   // initial values.
   AUD_SCLK <: 1;
   AUD_SDIN  <: 1;
   sync(AUD_SDIN);
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS + CTL_SCLK_PERIOD_LOW_TICKS;
   gt when timerafter(time) :> time;
   // start bit on SDI
   AUD_SCLK <: 1;
   AUD_SDIN  <: 0;
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 0;   
   // shift 7bits of address and 1bit R/W (fixed to write).
   // WARNING: Assume MSB first.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (DEVICE_ADRS >> (7 - i)) & 0x1;
      AUD_SDIN <: Temp;
      gt :> time;
      time += CTL_SCLK_PERIOD_HIGH_TICKS;
      gt when timerafter(time) :> time;
      AUD_SCLK <: 1;
      gt :> time;
      time += CTL_SCLK_PERIOD_HIGH_TICKS;
      gt when timerafter(time) :> time;
      AUD_SCLK <: 0;      
   }
   // turn the data to input
   AUD_SDIN :> Temp;
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 1;
   // sample first ACK.
   AUD_SDIN :> DeviceACK[0];
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 0;      
   // this build funny TI data format
   CtlAdrsData = ((Adrs & 0x7F) << 9) | (WrData & 0x1FF);
   // shift first 8 bits.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (CtlAdrsData >> (15 - i)) & 0x1;
      AUD_SDIN <: Temp;
      gt :> time;
      time += CTL_SCLK_PERIOD_HIGH_TICKS;
      gt when timerafter(time) :> time;
      AUD_SCLK <: 1;
      gt :> time;
      time += CTL_SCLK_PERIOD_HIGH_TICKS;
      gt when timerafter(time) :> time;
      AUD_SCLK <: 0;      
   }   
   // turn the data to input
   AUD_SDIN :> Temp;
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 1;
   // sample second ACK.
   AUD_SDIN :> DeviceACK[1];
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 0;      
   


   // shift second 8 bits.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (CtlAdrsData >> (7 - i)) & 0x1;
      AUD_SDIN <: Temp;
      gt :> time;
      time += CTL_SCLK_PERIOD_HIGH_TICKS;
      gt when timerafter(time) :> time;
      AUD_SCLK <: 1;
      gt :> time;
      time += CTL_SCLK_PERIOD_HIGH_TICKS;
      gt when timerafter(time) :> time;
      AUD_SCLK <: 0;      
   }    
   // turn the data to input
   AUD_SDIN :> Temp;
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 1;
   // sample second ACK.
   AUD_SDIN :> DeviceACK[2];
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 0;  
   gt :> time;
   time += CTL_SCLK_PERIOD_HIGH_TICKS;
   gt when timerafter(time) :> time;
   AUD_SCLK <: 1;
   // put the data to a good value for next round.
   AUD_SDIN  <: 1;
   // validate all items are ACK properly.
   Result = 1;
   for (i = 0; i < 3; i += 1)
   {
      if (DeviceACK[i] != 0)
      {
         Result = 0;
      }
   }   
   
   return(Result);        
}


/** This configure the Ti's TLV320AIC23B Audio Codec for followings:
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
   
   // Initialise the ports.  
   r_i2c.scl <: 1;
   r_i2c.sda <: 1;       
 
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



