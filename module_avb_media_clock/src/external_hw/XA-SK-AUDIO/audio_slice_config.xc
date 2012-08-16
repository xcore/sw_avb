#include <xs1.h>
#include <xclib.h>
#include "media_clock_client.h"
#include "print.h"
#include "i2c.h"
#include <stdlib.h>

#define CODEC1_I2C_DEVICE_ADDR       0x90
#define CODEC2_I2C_DEVICE_ADDR       0x92

#define CODEC_DEV_ID_ADDR           0x01
#define CODEC_PWR_CTRL_ADDR         0x02
#define CODEC_MODE_CTRL_ADDR        0x03
#define CODEC_ADC_DAC_CTRL_ADDR     0x04
#define CODEC_TRAN_CTRL_ADDR        0x05
#define CODEC_MUTE_CTRL_ADDR        0x06
#define CODEC_DACA_VOL_ADDR         0x07
#define CODEC_DACB_VOL_ADDR         0x08

/*
 *  This module takes the output from the media clock server and transforms it into a
 *  reference clock to be sent to a PLL multiplier device - the CS2100
 *
 *  There are four important clocks in the system:
 *
 *    100MHz reference clock
 *    The word clock
 *    The master clock (MCLK)
 *    The i2s bit clock (BCLK)
 *
 *  The media clock server outputs the length of the sample period in 100MHz clocks.
 *  This module converts that into a reference clock by waiting for a multiple of this
 *  period (the multiple is PLL_TO_WORD_MULTIPLIER) and outputting a clock based on that
 *  multiplier period.   The PLL device then takes this low frequency clock and multiplies
 *  the frequency up to the MCLK frequency, which is the product of PLL_TO_WORD_MULTIPLIER
 *  and mclks_per_wordclk
 */

// This is the number of word clocks per cycle of the PLL output clock
#define PLL_TO_WORD_MULTIPLIER 100

// outputs a clock of media clock
void audio_gen_slicekit_clock(out port p, chanend clk_ctl)
{
  int bit = 0x0;
  unsigned int wordTime;
  int wordLength = 0;
  int clk_cmd, clk_arg;
  unsigned int baseLength = 0;
  unsigned int lowBits = 0;
  unsigned int prevLowBits = 0;
  unsigned int bitMask = (1 << WC_FRACTIONAL_BITS) - 1;
  int count=0;
  int stop=0;
  
  // this is the number of word clocks in one PLL output
  unsigned mult = PLL_TO_WORD_MULTIPLIER;

  // we need 2 ticks per sample
  mult = mult/2;

  while (1)
  {
   clk_cmd = -1;
   while (clk_cmd != CLK_CTL_SET_RATE)
    {
      slave {clk_ctl :> clk_cmd; clk_ctl :> clk_arg; };
      switch (clk_cmd)
        {
        case CLK_CTL_SET_RATE:  
          wordLength = clk_arg >> 1;
          baseLength = wordLength >> WC_FRACTIONAL_BITS;
          break;
        }
      break;
    }

  stop = 0;

  p <: 0 @ wordTime;
  while (!stop) {
    wordTime += baseLength;

    count++;
    if (count==mult) {
      bit = ~bit;
      count = 0;
    }

    p @ wordTime <: bit;

    lowBits = (lowBits + wordLength) & bitMask;
    if (lowBits <  prevLowBits) {
      wordTime += 1;
    }
    prevLowBits = lowBits;


   
    select {
    case slave {clk_ctl :> clk_cmd; clk_ctl :> clk_arg; }:
      switch (clk_cmd)
        {
        case CLK_CTL_SET_RATE:  
          wordLength = clk_arg >> 1; 
          baseLength = wordLength >> WC_FRACTIONAL_BITS;
          break;
        case CLK_CTL_STOP:
        	stop = 1;
        	break;
        }
      break;
    default:
      break;
    }
   }
  }
}

void config_codec(port p_i2c, out port p_gpo)
{
    timer t;
    unsigned time;
    unsigned tmp;
    int codec_dev_id;
    char data[1];
    int codec_addrs[2] = {CODEC1_I2C_DEVICE_ADDR, CODEC2_I2C_DEVICE_ADDR};

    // Codec out of reset
    p_gpo <: 0xF;
    
    /* Give CODEC some reset recovery, CS4270 datasheet says 500ns min so lets do 1us */
    t :> time;
    time += 100;
    t when timerafter(time) :> int _;
    
    /* Set power down bit in the CODEC over I2C */
    
    data[0] = 0x01;
    i2c_master_write_reg(CODEC1_I2C_DEVICE_ADDR, CODEC_PWR_CTRL_ADDR, data,1, p_i2c);
    data[0] = 0x01;
    i2c_master_write_reg(CODEC2_I2C_DEVICE_ADDR, CODEC_PWR_CTRL_ADDR, data,1, p_i2c);
    
    /* Now set all registers as we want them :    
    
    Mode Control Reg:
    Set FM[1:0] as 11. This sets Slave mode.
    Set MCLK_FREQ[2:0] as 010. This sets MCLK to 512Fs in Single, 256Fs in Double and 128Fs in Quad Speed Modes.
    This means 24.576MHz for 48k and 22.5792MHz for 44.1k.
    Set Popguard Transient Control.
    So, write 0x35.

    ADC & DAC Control Reg:
    Leave HPF for ADC inputs continuously running.
    Digital Loopback: OFF
    DAC Digital Interface Format: I2S
    ADC Digital Interface Format: I2S
    So, write 0x09.

    Transition Control Reg:
    No De-emphasis. Don't invert any channels. Independent vol controls. Soft Ramp and Zero Cross enabled.
    So, write 0x60.

    Mute Control Reg:
    Turn off AUTO_MUTE
    So, write 0x00.

    DAC Chan A Volume Reg:
    We don't require vol control so write 0x00 (0dB)

    DAC Chan B Volume Reg:
    We don't require vol control so write 0x00 (0dB)  */

    for (int i=0; i < 2; i++)
    {
      i2c_master_read_reg(codec_addrs[i], CODEC_DEV_ID_ADDR, data, 0x01, p_i2c);
      codec_dev_id = data[0];
    
      if (((codec_dev_id & 0xF0) >> 4) != 0xC) {
          printstr("Unexpected CODEC Device ID, expected 0xC, got ");
          printhexln(codec_dev_id);
          while(1);
      }

      data[0] = 0x35;
      i2c_master_write_reg(codec_addrs[i], CODEC_MODE_CTRL_ADDR, data, 1, p_i2c);
      data[0] = 0x09;
      i2c_master_write_reg(codec_addrs[i], CODEC_ADC_DAC_CTRL_ADDR, data, 1, p_i2c);
      data[0] = 0x60;
      i2c_master_write_reg(codec_addrs[i], CODEC_TRAN_CTRL_ADDR, data,1, p_i2c);
      data[0] = 0x00;
      i2c_master_write_reg(codec_addrs[i], CODEC_MUTE_CTRL_ADDR, data,1, p_i2c);
      data[0] = 0x00;
      i2c_master_write_reg(codec_addrs[i], CODEC_DACA_VOL_ADDR, data,1, p_i2c);
      data[0] = 0x00;
      i2c_master_write_reg(codec_addrs[i], CODEC_DACB_VOL_ADDR, data,1, p_i2c);

      // Power down off
      data[0] = 0x00;
      i2c_master_write_reg(codec_addrs[i], CODEC_PWR_CTRL_ADDR, data,1, p_i2c);
    }

}



static unsigned char regaddr[9] = {0x09,0x08,0x07,0x06,0x17,0x16,0x05,0x03,0x1E};
static unsigned char regdata[9] = {0x00,0x00,0x00,0x00,0x00,0x08,0x01,0x01,0x00};

void init_slicekit_audio(port p_i2c, out port p_audio_shared, unsigned mclks_per_wordclk)
{
  int deviceAddr = 0x9C;
  unsigned char data[1];

  // this is the muiltiplier in the PLL, which takes the PLL reference clock and
  // multiplies it up to the MCLK frequency.
  (regdata,unsigned int[])[0] = ((PLL_TO_WORD_MULTIPLIER << 11) * mclks_per_wordclk);

  i2c_master_init(p_i2c);

  config_codec(p_i2c, p_audio_shared);

  #pragma unsafe arrays
  for(int i = 8; i >= 0; i--) {
    data[0] = (regdata,unsigned char[])[i];
    i2c_master_write_reg(deviceAddr, regaddr[i], data, 1, p_i2c);
  }
}

