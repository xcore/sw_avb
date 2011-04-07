#include <xs1.h>
#include <xclib.h>
#include "media_clock_client.h"
#include "print.h"
#include "i2c.h"
#include <stdlib.h>

/*** Private Definitions ***/
#define CTL_SCLK_PERIOD_LOW_TICKS      (1000)
#define CTL_SCLK_PERIOD_HIGH_TICKS     (1000)

#define DEVICE_ADRS                    (0x90)


// outputs a clock of media clock (e.g. 48khz) * mult
void audio_gen_CS2300CP_clock(out port p, 
                              chanend clk_ctl, 
                              unsigned mult)
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
  
  // we need 2 ticks per sample
  mult = mult/2;

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



void audio_clock_CS2300CP_init(struct r_i2c &r_i2c,
                               unsigned mult) 
{
   int deviceAddr = 0x9C;

   mult = mult/2;
   mult = mult << 12;

  // Configure PLL
  i2c_wr(0x03, 0x01,deviceAddr,r_i2c);
  i2c_wr(0x05, 0x01,deviceAddr,r_i2c);
  i2c_wr(0x16, 0x10,deviceAddr,r_i2c);
  i2c_wr(0x17, 0x00,deviceAddr,r_i2c);

  i2c_wr( 0x06, (mult >> 24) & 0xFF,deviceAddr,r_i2c);
  i2c_wr( 0x07, (mult >> 16) & 0xFF,deviceAddr,r_i2c);
  i2c_wr( 0x08, (mult >> 8) & 0xFF, deviceAddr,r_i2c);
  i2c_wr( 0x09, (mult) & 0xFF,      deviceAddr,r_i2c);

  // Check configuration
  if (i2c_rd(0x03, deviceAddr, r_i2c) != 0x01 || i2c_rd(0x09, deviceAddr, r_i2c) != (mult & 0xFF))
  {
    printstr("PLL chip configuration failed\n");
    return;
  }

}

