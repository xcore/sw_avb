
#include <xs1.h>
#include <xclib.h>
#include <stdlib.h>
#include <print.h>
#include "audio_codec_CS42448.h"
#include "print.h"
#include "i2c.h"

/*** Private Definitions ***/
#define CTL_SCLK_PERIOD_LOW_TICKS      (1000)
#define CTL_SCLK_PERIOD_HIGH_TICKS     (1000)

#define DEVICE_ADRS                    (0x90)




int regrd(int addr, int device, port scl, port sda)
{
    //int Result;
   timer gt;
   unsigned time;
   int Temp, CtlAdrsData, i;
   // three device ACK
   int ack[3];
   int rdData;

   // initial values.
   scl <: 1;
   sda  <: 1;
   sync(sda);
   gt :> time;
   time += 1000 + 1000;
   gt when timerafter(time) :> int _;
   // start bit on SDI
   scl <: 1;
   sda  <: 0;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;
   // shift 7bits of address and 1bit R/W (fixed to write).
   // WARNING: Assume MSB first.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (device >> (7 - i)) & 0x1;
      sda <: Temp;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 1;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 0;
   }
   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 1;
   // sample first ACK.
   sda :> ack[0];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;

   CtlAdrsData = (addr & 0xFF);
   // shift first 8 bits.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (CtlAdrsData >> (7 - i)) & 0x1;
      sda <: Temp;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 1;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 0;
   }
   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 1;
   // sample second ACK.
   sda :> ack[1];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;


   // stop bit
   gt :> time;
   time += 1000 + 1000;
   gt when timerafter(time) :> int _;
   // start bit on SDI
   scl <: 1;
   sda  <: 1;
   time += 1000 + 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;
   time += 1000 + 1000;
   gt when timerafter(time) :> int _;


   // send address and read
   scl <: 1;
   sda  <: 1;
   sync(sda);
   gt :> time;
   time += 1000 + 1000;
   gt when timerafter(time) :> int _;
   // start bit on SDI
   scl <: 1;
   sda  <: 0;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;
   // shift 7bits of address and 1bit R/W (fixed to write).
   // WARNING: Assume MSB first.
   for (i = 0; i < 8; i += 1)
   {
      int deviceAddr = device | 1;
      Temp = (deviceAddr >> (7 - i)) & 0x1;
      sda <: Temp;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 1;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 0;
   }
   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 1;
   // sample first ACK.
   sda :> ack[0];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;


   rdData = 0;
   // shift second 8 bits.
   for (i = 0; i < 8; i += 1)
   {

      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 1;

      sda :> Temp;
      rdData = (rdData << 1) | (Temp & 1);

      gt :> time;
      time += 1000;
      gt when timerafter(time) :> int _;
      scl <: 0;
   }

   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 1;
   // sample second ACK.
   sda :> ack[2];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 0;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> int _;
   scl <: 1;
   // put the data to a good value for next round.
   sda  <: 1;
   // validate all items are ACK properly.
   //Result = 0;
   //for (i = 0; i < 3; i += 1)
   //{
      //if ((ack[i]&1) != 0)
      //{
         //Result = Result | (1 << i);
      //}
   //}

   return rdData;
}

void regwr(int addr, int data, int device, port scl, port sda)
{
   //int Result;
   timer gt;
   unsigned time;
   int Temp, CtlAdrsData, i;
   // three device ACK
   int ack[3];

   // initial values.
   scl <: 1;
   sda  <: 1;
   sync(sda);

   gt :> time;
   time += 1000 + 1000;
   gt when timerafter(time) :> void;

   // start bit on SDI
   scl <: 1;
   sda  <: 0;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 0;

   // shift 7bits of address and 1bit R/W (fixed to write).
   // WARNING: Assume MSB first.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (device >> (7 - i)) & 0x1;
      sda <: Temp;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> void;
      scl <: 1;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> void;
      scl <: 0;
   }

   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 1;

   // sample first ACK.
   sda :> ack[0];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 0;

   CtlAdrsData = (addr & 0xFF);

   // shift first 8 bits.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (CtlAdrsData >> (7 - i)) & 0x1;
      sda <: Temp;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> void;
      scl <: 1;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> void;
      scl <: 0;
   }
   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 1;
   // sample second ACK.
   sda :> ack[1];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 0;

   CtlAdrsData = (data & 0xFF);
   // shift second 8 bits.
   for (i = 0; i < 8; i += 1)
   {
      Temp = (CtlAdrsData >> (7 - i)) & 0x1;
      sda <: Temp;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> void;
      scl <: 1;
      gt :> time;
      time += 1000;
      gt when timerafter(time) :> void;
      scl <: 0;
   }
   // turn the data to input
   sda :> Temp;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 1;
   // sample second ACK.
   sda :> ack[2];
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 0;
   gt :> time;
   time += 1000;
   gt when timerafter(time) :> void;
   scl <: 1;
   // put the data to a good value for next round.
   sda  <: 1;
   // validate all items are ACK properly.
   //Result = 0;
   //for (i = 0; i < 3; i += 1)
   //{
      //if ((ack[i]&1) != 0)
      //{
         //Result = Result | (1 << i);
      //}
   //}
   //return(Result);
}






#define CODEC_FUNCTIONAL_MODE          (0x3) 
#define CODEC_INTERFACE_FORMATS        (0x4)
#define CODEC_ADC_CONTROL              (0x5)


void REGWR(unsigned reg, unsigned val, struct r_i2c &r_i2c)
{
	struct i2c_data_info data;
	data.master_num = 0;
	data.data_len = 1;
	data.clock_mul = 1;
	data.data[0] = val;

	i2c_master_tx(DEVICE_ADRS, reg, data, r_i2c);
}

unsigned int REGRD(unsigned reg, struct r_i2c &r_i2c)
{
	struct i2c_data_info data;
	data.master_num = 0;
	data.data_len = 1;
	data.clock_mul = 1;

	i2c_master_rx(DEVICE_ADRS, reg, data, r_i2c);
	return data.data[0];
}


void audio_codec_CS42448_init(out port AUD_RESET_N, 
                              struct r_i2c &r_i2c,
                              int mode) 
{
   timer t;
   unsigned int time;
   
   printstr("CS42448 CODEC initializing\n");

   i2c_master_init(r_i2c);

   // Reset the codec
   AUD_RESET_N <: 0;
   t :> time;
   t when timerafter(time + 100000) :> time;
   AUD_RESET_N <: 1;
   
   t :> time;
   t when timerafter(time + 100000) :> time;
   
   // Initialise the ports.
   // control interface simple
   //   AUD_SCLK <: 1;
   //   AUD_SDIN <: 1;

   // DAC_FM = 0 (single speed), ADC_FM = 0 (single speed), MFREQ = 2:  MCLK/512 - 48Khz
   //   WrData = 0x54;
   //   WrData = 0b01010100;
   if (mode == CODEC_TDM)
     REGWR(CODEC_FUNCTIONAL_MODE, 0b11111000, r_i2c);
   else  // Default to I2S
     REGWR(CODEC_FUNCTIONAL_MODE, 0b00000100, r_i2c);
   
   // FREEZE = 0, AUX_DIF = 0, DAC_DIF=1 (I2S), ADC_DIF = 1 (I2S)
   //WrData = 0x11;
   //WrData = 0b00010001;
   
   if (mode == CODEC_TDM)
     REGWR(CODEC_INTERFACE_FORMATS, 0b00110110, r_i2c);
   else  // Default to I2S
     REGWR(CODEC_INTERFACE_FORMATS, 0b00001001, r_i2c);
   
   // ADC1-2_HPF FREEZE = 0, ADC3_HPF FREEZE = 0, DAC_DEM = 0, 
   // ADC1_SINGLE = 1(single ended), ADC2_SINGLE = 1, ADC3_SINGLE = 1, AIN5_MUX = 0, AIN6_MUX = 0  
   REGWR(CODEC_ADC_CONTROL, 0x1C, r_i2c);

   printstr("CS42448 CODEC initialized\n");
}
