
/* This file provides an audio interface that outputs a synthesized
   sine wave a particular frequency */
#include <xs1.h>
#include <print.h>

#include "media_clock_client.h"
#include "media_fifo.h"
#include "print.h"
#define SINE_TABLE_SIZE 100






static unsigned int sine[100] = {
    0x0100da,0x0200b0,0x02fe81,0x03f94b,0x04f011,
    0x05e1da,0x06cdb2,0x07b2aa,0x088fdb,0x096466,
    0x0a2f74,0x0af037,0x0ba5ed,0x0c4fde,0x0ced5f,
    0x0d7dd1,0x0e00a1,0x0e754b,0x0edb5a,0x0f3267,
    0x0f7a18,0x0fb227,0x0fda5b,0x0ff28a,0x0ffa9c,
    0x0ff28a,0x0fda5b,0x0fb227,0x0f7a18,0x0f3267,
    0x0edb5a,0x0e754b,0x0e00a1,0x0d7dd1,0x0ced5f,
    0x0c4fde,0x0ba5ed,0x0af037,0x0a2f74,0x096466,
    0x088fdb,0x07b2aa,0x06cdb2,0x05e1da,0x04f011,
    0x03f94b,0x02fe81,0x0200b0,0x0100da,0x000000,
    0xfeff26,0xfdff50,0xfd017f,0xfc06b5,0xfb0fef,
    0xfa1e26,0xf9324e,0xf84d56,0xf77025,0xf69b9a,
    0xf5d08c,0xf50fc9,0xf45a13,0xf3b022,0xf312a1,
    0xf2822f,0xf1ff5f,0xf18ab5,0xf124a6,0xf0cd99,
    0xf085e8,0xf04dd9,0xf025a5,0xf00d76,0xf00564,
    0xf00d76,0xf025a5,0xf04dd9,0xf085e8,0xf0cd99,
    0xf124a6,0xf18ab5,0xf1ff5f,0xf2822f,0xf312a1,
    0xf3b022,0xf45a13,0xf50fc9,0xf5d08c,0xf69b9a,
    0xf77025,0xf84d56,0xf9324e,0xfa1e26,0xfb0fef,
    0xfc06b5,0xfd017f,0xfdff50,0xfeff26,0x000000,
    };


void synth(int period, 
           chanend media_ctl,
           chanend clk_ctl, 
           int clk_ctl_index,
           media_input_fifo_t input_fifos[],
           int num_channels)
{
  timer tmr;
  int i=0,index=0;
  unsigned int wordTime;
  int wordLength; 
  unsigned int baseLength;
  unsigned int lowBits = 0;
  unsigned int prevLowBits = 0;
  unsigned int bitMask = (1 << WC_FRACTIONAL_BITS) - 1;
  int step;
  unsigned clk_cmd, clk_arg;
  unsigned int rand = 646226;
  unsigned int a=1664525;
  unsigned int c=1013904223;
  int sample;
  int count = 0;

  media_ctl_register(media_ctl, num_channels, 
                     input_fifos, 0, null, clk_ctl_index);

  clk_cmd = -1;
  while (clk_cmd != CLK_CTL_SET_RATE) 
    {
      slave {clk_ctl :> clk_cmd; clk_ctl :> clk_arg; };
      switch (clk_cmd)
        {
        case CLK_CTL_SET_RATE:  
          wordLength = clk_arg;
          baseLength = wordLength >> WC_FRACTIONAL_BITS;
          break;
        }
      break;
    }


  step = ((2*period)/SINE_TABLE_SIZE);

  tmr :> wordTime;
  wordTime += baseLength;
  while (1) {
    int outData;
    select {
    case tmr when timerafter(wordTime) :> int _:
      rand = a*rand+c;
      wordTime += baseLength;

      // scale it down
      sample = (((signed) (sine[index] << 8)) >> 10) & 0xffffff;
            
      if (count < 48000)        
        outData = sample;
        //   outData = (sample+(rand>>30));     
      else
        outData = 0;

      for (int i=0;i<num_channels;i++)
        media_input_fifo_push_sample(input_fifos[i], outData, wordTime);
        
      i++;
      if (i==step) {
        index++;
        if (index >= SINE_TABLE_SIZE)
          index=0;
        i=0;
      }

      lowBits = (lowBits + wordLength) & bitMask;
      if (lowBits <  prevLowBits) {
        wordTime += 1;
      }
      prevLowBits = lowBits;
      count++;
      if (count == 96000)
        count = 0;
      break;
    case slave { clk_ctl :> clk_cmd; clk_ctl :> clk_arg; }:
      switch (clk_cmd)
        {
        case CLK_CTL_SET_RATE:
          wordLength = clk_arg;
          baseLength = wordLength >> WC_FRACTIONAL_BITS;
          break;
        }
      break;
    }
  }
}
