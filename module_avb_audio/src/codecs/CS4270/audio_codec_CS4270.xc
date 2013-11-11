
#include <xs1.h>
#include <xclib.h>
#include <stdlib.h>
#include <print.h>
#include "audio_codec_CS4270.h"
#include "print.h"
#include "i2c.h"

#define CODEC_DEV_ID_ADDR           0x01
#define CODEC_PWR_CTRL_ADDR         0x02
#define CODEC_MODE_CTRL_ADDR        0x03
#define CODEC_ADC_DAC_CTRL_ADDR     0x04
#define CODEC_TRAN_CTRL_ADDR        0x05
#define CODEC_MUTE_CTRL_ADDR        0x06
#define CODEC_DACA_VOL_ADDR         0x07
#define CODEC_DACB_VOL_ADDR         0x08


static unsigned char regaddr[8] = { CODEC_PWR_CTRL_ADDR,
                                    CODEC_MODE_CTRL_ADDR,
                                    CODEC_ADC_DAC_CTRL_ADDR,
                                    CODEC_TRAN_CTRL_ADDR,
                                    CODEC_MUTE_CTRL_ADDR,
                                    CODEC_DACA_VOL_ADDR,
                                    CODEC_DACB_VOL_ADDR,
                                    CODEC_PWR_CTRL_ADDR
                                   };
static unsigned char regdata[8] = {0x01,0x35,0x09,0x60,0x00,0x00,0x00,0x00};

void audio_codec_CS4270_init(out port p_codec_reset,
                              int mask,
                              int codec_addr,
                        #if I2C_COMBINE_SCL_SDA
                              port r_i2c
                        #else
                              struct r_i2c &r_i2c
                        #endif
                              )
{
  timer tmr;
  unsigned time;
  char data[1];

#if I2C_COMBINE_SCL_SDA
  // Unfortunately the single port and simple I2C APIs do not currently match
  // with regards the device address
  deviceAddr <<= 1;
#endif

  // Bring codec out of reset
  p_codec_reset <: 0xF;

  tmr :> time;
  time += 100;
  tmr when timerafter(time) :> int _;

  for(int i = 0; i < 8; i++) {
    data[0] = regdata[i];
    i2c_master_write_reg(codec_addr, regaddr[i], data, 1, r_i2c);
  }

}
