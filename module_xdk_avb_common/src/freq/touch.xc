/////////////////////////////////////////////////////////
//                                                     //
//              XMOS Semiconductor Ltd                 //
//                 * Touch screen *                    //
//                                                     //
//    	Simple touchscreen test and calib (revA)       //
//						                               //
//                    Ross Owen                        //
//                    June 2008                        //
/////////////////////////////////////////////////////////


#include <xs1.h>
#include <xclib.h>
#include "banks.h"
#include <platform.h>

#define BACKGROUND_COLOUR LCD_BLACK
#define CROSS_COLOUR LCD_BLUE

#define TOP             220
#define BOTTOM          20
#define RIGHT           300
#define LEFT            20

{unsigned, unsigned} getTouchScreenPos(out port CS_port, out port DCLK_port_port, out port DIN_port, in port DOUT_port);


// ADC ports/ clock block
on stdcore[3]: in port ADC_PENIRQ_port = XS1_PORT_4C;
on stdcore[3]: out port ADC_CS_port = XS1_PORT_4E;
on stdcore[3]: out port ADC_DCLK_port = XS1_PORT_4D; // clocked
on stdcore[3]: out port ADC_DIN_port = XS1_PORT_1P;  // clocked
on stdcore[3]: in port ADC_DOUT_port = XS1_PORT_1O;  // clocked
on stdcore[3]: clock ADC_clk = XS1_CLKBLK_2;

extern unsigned doADCTransaction(unsigned controlReg);

// Calibration data
unsigned right, left, top, bottom, x_scale, y_scale;

void process(chanend c_adc, chanend toVis, int i, int v) {
    c_adc <: i;
    c_adc <: v;
    toVis <: i;
    toVis <: v;
}

int absDiff(int x, int y) {
    if (x > y) return x-y;
    return y-x;
}

int acceptable(int x, int y) {
#define N_L2 2
#define N (1<<N_L2)
#define M 10
    static int cnt = 0;
    static int xs[N], ys[N];
    if (cnt > 0) {
        if (absDiff(x, xs[0]) > M || absDiff(y,ys[0]) > M) {
            for (int i = 1; i < cnt; i++) {
                xs[i-1] = xs[i];
                ys[i-1] = ys[i];
            }
            cnt--;
        } else if(cnt == N) {
            for (int i = 0; i < cnt; i++) {
                x += xs[i];
                y += ys[i];
            }
            x >>= N_L2;
            y >>= N_L2;
            cnt = 0;
            return 1;
        }
    }
    xs[cnt] = x;
    ys[cnt] = y;
    cnt++;
    return 0;
}

void touch(chanend toVis, chanend c_adc)
{
    int tmpx, tmpy, time;
    int x, y;
    timer t;

    set_clock_div(ADC_clk,50);
    start_clock(ADC_clk);
    set_port_clock(ADC_DCLK_port, ADC_clk);
    set_port_clock(ADC_DIN_port, ADC_clk);
    set_port_clock(ADC_DOUT_port, ADC_clk);

    ADC_CS_port <: 1;
    ADC_DCLK_port <: 0;

    for(int i = 0; i < BANKS; i++){
      int v = 200*i/(BANKS-1) - 200*i*i/(BANKS-1)/(BANKS-1);
      if (v > 64) v = 63;
      if (v < 0) v = 1;
      process(c_adc, toVis, i, v);
    }


    while(1) {
        select {
        case ADC_PENIRQ_port when pinsneq(1) :> int _:
            // Get positions on screen
            tmpy = doADCTransaction(0x1a);
            tmpx = doADCTransaction(0x5a);
            if (acceptable(tmpx,tmpy)) {
                tmpx = ((tmpx - 20) * 17 / 64);
                tmpy = (tmpy - 20) * BANKS / 225;
                if (tmpx != x || tmpy != y) {
                    x = tmpx;
                    y = tmpy;
                    if (y >= 0 && y < BANKS && x >= 0 && x <=64) {
                      process(c_adc, toVis, y, x);
                    }
                }
            }
            break;
        }
        t :> time;
        t when timerafter(time+500000) :> time;
    }
}

unsigned doADCTransaction(unsigned controlReg) {
  unsigned returnVal = 0;
  int i;

  ADC_CS_port <: 0;
  controlReg=bitrev(controlReg);
  controlReg=controlReg >> 25;

  ADC_DCLK_port <: 0;
  ADC_DCLK_port <: 0;

  // Start bit
  ADC_DIN_port <: 1;
  ADC_DCLK_port <: 1;

  for(i = 0; i< 7; i+=1) {
    ADC_DCLK_port <: 0;
    ADC_DIN_port <: >> controlReg;
    ADC_DCLK_port <: 1;
    ADC_DCLK_port <: 1;
  }

  //Busy clock.  Should wait for busy low here..
  ADC_DCLK_port <:0;
  ADC_DCLK_port <:1;

  // TODO: check control reg val to see how many bits to clock in
  for(i = 0; i < 8; i+=1) {
    ADC_DCLK_port <: 0;
    ADC_DCLK_port <: 1;
    ADC_DCLK_port <: 1;
    ADC_DOUT_port :> >> returnVal;
  }

  ADC_CS_port <: 1;
  ADC_DCLK_port <: 0;

  return bitrev(returnVal) ;
}
