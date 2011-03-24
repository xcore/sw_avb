/* FFT Based on code by Tom Roberts 11/8/89, Malcolm Slaney 12/15/94 */
#include <xs1.h>
#include <xclib.h>
#include "banks.h"
#include "gptp.h"
#include "avb_1722_def.h"
#include "LCD_Comp_Def.h"

extern int Sinewave[]; /* under the fft */

#pragma unsafe arrays
void fix_fft_512(int fin[], unsigned int fout[]) {
    int fr[512], fi[512];
    int l,k;
    int sum = 0;


    for(int m=0; m<512; m++) {
        sum += fin[m] >> 9;
        fi[m] = 0;
        fr[bitrev(m<<23)] = fin[m];
    }
#pragma unsafe arrays
    for(int m=0; m<512; m++) {
        fr[m] -= sum;
    }

    l = 1;
    k = 8;
#pragma unsafe arrays
    while(l < 512) {
        for(int m=0; m < l; m++) {
            int j = m << k;
            int wr =  Sinewave[j+128];
            int wi = -Sinewave[j];
            wr >>= 1;
            wi >>= 1;
            for(int i=m; i< 512; i+=l+l) {
                int j = i + l;
                int tr = (wr*fr[j] - wi*fi[j])>>15;
                int ti = (wr*fi[j] + wi*fr[j])>>15;
                int qr = fr[i];
                int qi = fi[i];
                qr >>= 1;
                qi >>= 1;
                fr[j] = qr - tr;
                fi[j] = qi - ti;
                fr[i] = qr + tr;
                fi[i] = qi + ti;
            }
        }
        k-=1;
        l = l << 1;
    }
    for(int m=0; m<512; m += 1) {
        fout[m] = fr[m] * fr[m] + fi[m] * fi[m];
    }
}

int Sinewave[512] = {
 0, 402, 804, 1206, 1608, 2009, 2410, 2811, 3212, 3612, 4011, 4410, 4808, 5205, 5602, 5998, 6393, 6786, 7179, 7571, 7962, 8351, 8739, 9126, 9512, 9896, 10278, 10659, 11039, 11417, 11793, 12167, 12539, 12910, 13279, 13645, 14010, 14372, 14732, 15090, 15446, 15800, 16151, 16499, 16846, 17189, 17530, 17869, 18204, 18537, 18868, 19195, 19519, 19841, 20159, 20475, 20787, 21096, 21403, 21705, 22005, 22301, 22594, 22884, 23170, 23452, 23731, 24007, 24279, 24547, 24811, 25072, 25329, 25582, 25832, 26077, 26319, 26556, 26790, 27019, 27245, 27466, 27683, 27896, 28105, 28310, 28510, 28706, 28898, 29085, 29268, 29447, 29621, 29791, 29956, 30117, 30273, 30424, 30571, 30714, 30852, 30985, 31113, 31237, 31356, 31470, 31580, 31685, 31785, 31880, 31971, 32057, 32137, 32213, 32285, 32351, 32412, 32469, 32521, 32567, 32609, 32646, 32678, 32705, 32728, 32745, 32757, 32765,
 32767, 32765, 32757, 32745, 32728, 32705, 32678, 32646, 32609, 32567, 32521, 32469, 32412, 32351, 32285, 32213, 32137, 32057, 31971, 31880, 31785, 31685, 31580, 31470, 31356, 31237, 31113, 30985, 30852, 30714, 30571, 30424, 30273, 30117, 29956, 29791, 29621, 29447, 29268, 29085, 28898, 28706, 28510, 28310, 28105, 27896, 27683, 27466, 27245, 27019, 26790, 26556, 26319, 26077, 25832, 25582, 25329, 25072, 24811, 24547, 24279, 24007, 23731, 23452, 23170, 22884, 22594, 22301, 22005, 21705, 21403, 21096, 20787, 20475, 20159, 19841, 19519, 19195, 18868, 18537, 18204, 17869, 17530, 17189, 16846, 16499, 16151, 15800, 15446, 15090, 14732, 14372, 14010, 13645, 13279, 12910, 12539, 12167, 11793, 11417, 11039, 10659, 10278, 9896, 9512, 9126, 8739, 8351, 7962, 7571, 7179, 6786, 6393, 5998, 5602, 5205, 4808, 4410, 4011, 3612, 3212, 2811, 2410, 2009, 1608, 1206, 804, 402,
 0, -402, -804, -1206, -1608, -2009, -2410, -2811, -3212, -3612, -4011, -4410, -4808, -5205, -5602, -5998, -6393, -6786, -7179, -7571, -7962, -8351, -8739, -9126, -9512, -9896, -10278, -10659, -11039, -11417, -11793, -12167, -12539, -12910, -13279, -13645, -14010, -14372, -14732, -15090, -15446, -15800, -16151, -16499, -16846, -17189, -17530, -17869, -18204, -18537, -18868, -19195, -19519, -19841, -20159, -20475, -20787, -21096, -21403, -21705, -22005, -22301, -22594, -22884, -23170, -23452, -23731, -24007, -24279, -24547, -24811, -25072, -25329, -25582, -25832, -26077, -26319, -26556, -26790, -27019, -27245, -27466, -27683, -27896, -28105, -28310, -28510, -28706, -28898, -29085, -29268, -29447, -29621, -29791, -29956, -30117, -30273, -30424, -30571, -30714, -30852, -30985, -31113, -31237, -31356, -31470, -31580, -31685, -31785, -31880, -31971, -32057, -32137, -32213, -32285, -32351, -32412, -32469, -32521, -32567, -32609, -32646, -32678, -32705, -32728, -32745, -32757, -32765,
 -32767, -32765, -32757, -32745, -32728, -32705, -32678, -32646, -32609, -32567, -32521, -32469, -32412, -32351, -32285, -32213, -32137, -32057, -31971, -31880, -31785, -31685, -31580, -31470, -31356, -31237, -31113, -30985, -30852, -30714, -30571, -30424, -30273, -30117, -29956, -29791, -29621, -29447, -29268, -29085, -28898, -28706, -28510, -28310, -28105, -27896, -27683, -27466, -27245, -27019, -26790, -26556, -26319, -26077, -25832, -25582, -25329, -25072, -24811, -24547, -24279, -24007, -23731, -23452, -23170, -22884, -22594, -22301, -22005, -21705, -21403, -21096, -20787, -20475, -20159, -19841, -19519, -19195, -18868, -18537, -18204, -17869, -17530, -17189, -16846, -16499, -16151, -15800, -15446, -15090, -14732, -14372, -14010, -13645, -13279, -12910, -12539, -12167, -11793, -11417, -11039, -10659, -10278, -9896, -9512, -9126, -8739, -8351, -7962, -7571, -7179, -6786, -6393, -5998, -5602, -5205, -4808, -4410, -4011, -3612, -3212, -2811, -2410, -2009, -1608, -1206, -804, -402,};


#define SHIFT 64
#define BUFSIZE 512

void buffer(chanend fromDAC, chanend toBuf) {
  int buf[BUFSIZE]={0};
    int rd = 0, wr = 0;
    int sample;

#pragma unsafe arrays
    while (1) {
#pragma ordered
        select {
        case fromDAC :> sample:
          sample = (int) (short) (sample >> 8);
          buf[wr] = sample;
          wr = (wr+1) & (BUFSIZE-1);
          break;
        case (rd != wr) => toBuf :> int x:
            toBuf <: buf[rd];
            rd = (rd + 1) & (BUFSIZE-1);
          break;
        }
    }
}


void freq_tap(streaming chanend adc_in_c, 
              streaming chanend dac_in_c, 
              streaming chanend dac_out_c,
              chanend toBuf, chanend freq_ctl)
{
  unsigned int sample;
  int skip_count1 = 0, skip_count2 = 1;//isSamplePush;
  int count=0;
  int useADC = 1;
  int changeADC = 0;
  //chan tap_c;
  int prev_l=0, prev_r=0;
  int adc_lrc = 0;

  int buf[BUFSIZE]={0};
    int rd = 0, wr = 0;


#pragma unsafe arrays
    while (1) {
#pragma ordered
        select {
        case dac_in_c :> sample:
           // pass on the sample pull
          dac_out_c <: sample;
           if (adc_lrc)
             dac_in_c <: prev_l;
           else
             dac_in_c <: prev_r;
           break;
        case dac_out_c :> sample:
           if (adc_lrc)
             prev_l = sample;
           else
             prev_r = sample;

           adc_lrc = !adc_lrc;

           if (!useADC) {
             count++;
             skip_count2++;
             if (skip_count2 == 4) {
               int new_wr;
             skip_count2 = 0;
             sample = (int) (short) ((sample >> 8) & 0xffff);
        	   buf[wr] = sample;
                wr = (wr+1) & (BUFSIZE-1);
                if (wr == rd)
                  rd = (rd + 1) & (BUFSIZE-1);

                if (changeADC) {
                  useADC = !useADC;
                  changeADC = 0;
                }
             }
           }
           break;
        case adc_in_c :> sample:
          //          adc_out_c <: sample;

           if (useADC) {
             count++;
             skip_count1++;
             if (skip_count1 == 4) {
                int new_wr;
              skip_count1 = 0;
        	    sample = (int) (short) ((sample >> 8) & 0xffff);


        	    buf[wr] = sample;
                wr = (wr+1) & (BUFSIZE-1);
                if (wr == rd)
                  rd = (rd + 1) & (BUFSIZE-1);

                if (changeADC) {
                  useADC = !useADC;
                  changeADC = 0;
                }

             }
           }
           break;
        case (rd != wr) => toBuf :> int x:
          toBuf <: buf[rd];          
          rd = (rd + 1) & (BUFSIZE-1);
        break;
        case freq_ctl :> int:
            changeADC=1;
          break;
        }
    }
}





void ffter(streaming chanend fromBuf, chanend toScreen) {
    int fi[512];
    unsigned int fo[512];
#pragma unsafe arrays
    while (1) {
        int avg;

        avg = 0;
        for(int i=0; i < 512-SHIFT; i++) {
            fi[i] = fi[i+SHIFT];
        }
        
        //        fromBuf <: SHIFT;
        for(int i=512-SHIFT; i<512; i++) {
          //fromBuf :> fi[i];
          fromBuf <: 0;
          fromBuf :> fi[i];
            if (fi[i] == 0x80000000) {
                toScreen <: fi[i];
                return;
            }
            fi[i] <<= 3;
        }

        fix_fft_512(fi,fo);
        avg = 0;
        for(int i=1; i<256; i++) {
            int a = fo[i] + fo[512-i];
            int b = (32 - clz(a));
            a = (b<<3)-80;
            if (a < 0) {
                a = 0;
            }
            if (i < 2) {
                for (int j = 0; j < 8; j++ ) {
                    toScreen <: a;
                }
            } else if (i < 4) {
                for (int j = 0; j < 4; j++ ) {
                    toScreen <: a;
                }
            } else if (i < 8) {
                for (int j = 0; j < 2; j++ ) {
                    toScreen <: a;
                }
            } else if (i < 16) {
                toScreen <: a;
            } else {
                int mask;
                if (i < 32) {
                    mask = 1;
                } else if (i < 64) {
                    mask = 3;
                } else if (i < 128) {
                    mask = 7;
                } else if (i < 256) {
                    mask = 15;
                }
                if ((i & mask) == mask) {
                    toScreen <: avg/mask;
                    avg = 0;
                } else {
                    avg += a;
                }
            }
        }
    }
}


#define FILTERCOLOUR  TO_565(0x003f)
#define INPUTCOLOUR  TO_565(0x00c00)
#define BACKGROUND   TO_565(0xffff)
#define BLACK TO_565(0)
#define TIMERBARCOLOUR TO_565(0xff0000)

#define LTACOLOUR  TO_565(0x003f)
#define EQCOLOUR  TO_565(0)

#define TIMER_POLL_TIME (1 << 10)

void realtime(chanend fromEqualiser, chanend fromFFTBefore, chanend toLCD, chanend ptp) {
    int cnt = 0;
    int rowCount = 0;
    int x;
    int subColCount = 0;
    int colCount = -8;
    int lta[64] = {0};
    int buf[64] = {0};
    int next[8], col[8], theNext, theCol, equaliser[BANKS], ltaRow = -1, eqRow = -1;
    int i;
    unsigned int globtime;
    unsigned int timerCol = TIMERBARCOLOUR;
    ptp_time_info_mod64 time_info;
    timer tmr;
    unsigned int globclock_timeout;
    tmr :> globclock_timeout;
    globclock_timeout += TIMER_POLL_TIME;


    for(int i = 0; i<BANKS; i++) {
        equaliser[i] = 0;
    }
    for(int i = 0; i<64; i++) {
        lta[i] = (i*2)<<8;
    }
    i = 0;
        //outuint(toLCD, 0);

    colCount=-8;
    subColCount=0;

#pragma unsafe arrays
    while(1) {
#pragma ordered
        select {

        //case ptp:> globtime:
          //         break;

        case chkct(toLCD, XS1_CT_END):
          outct(toLCD, XS1_CT_ACK);
          for (colCount=-8;colCount < 72;colCount++) 
            {
              for (subColCount=0;subColCount < 4;subColCount++) 
                {
                  if (colCount >= 0 && colCount < 64)
                    {
                      int v0 = buf[colCount];
                      int aa;
                      int marker;
                      
                      ltaRow = lta[colCount] >> 8;
                      if (subColCount > 2)
                        eqRow = 2*equaliser[colCount*BANKS>>6];
                      else
                        eqRow = -1;
                      aa = ltaRow;
                    marker = FILTERCOLOUR;
                    
                    i = 0;
                    if (subColCount == 0 && (colCount & 7) == 0 ) {
                      theNext = 128;
                      theCol = LCD_565_GREY3;
                    } else {
                      if (v0 == aa) {
                        theNext = aa;
                        next[0] = aa+1;
                        next[1] = 128;
                        theCol = INPUTCOLOUR;
                        col[0] = marker;
                        col[1] = BACKGROUND;
                      } else if (v0 < aa) {
                        theNext = v0;
                            next[0] = aa;
                            next[1] = aa+1;
                            next[2] = 128;
                            theCol = INPUTCOLOUR;
                            col[0] = BACKGROUND;
                            col[1] = marker;
                            col[2] = BACKGROUND;
                      } else {
                            theNext = aa;
                            next[0] = aa+1;
                            next[1] = v0;
                            next[2] = 128;
                            theCol = INPUTCOLOUR;
                            col[0] = marker;
                            col[1] = INPUTCOLOUR;
                            col[2] = BACKGROUND;
                        }
                    }
                }
                else {
                  i=0;
                  theNext = (globtime >> 24) & 0x7f;
                  next[0] = 128;
                  next[1] = 128;
                  next[2] = 128;
                  theCol = timerCol;
                  col[0] = BLACK;
                  col[1] = INPUTCOLOUR;
                  col[2] = BACKGROUND;
                  ltaRow = -1;
                  eqRow = -1;
                }

                  select
                    {
                    case fromFFTBefore :> x:
                      buf[cnt] = x ;
                      lta[cnt] = (lta[cnt] * 31 + (x<<8)) >> 5;
                      cnt++;
                      if (cnt == 64) {
                        cnt = 0;
                      }
                      break;
                    default:
                      break;
                    }
          
          for (rowCount=0;rowCount < 120;rowCount++) {
            int curcol;
            if (rowCount >= theNext) {
              theCol = col[i];
              theNext = next[i];
              i++;
            }
            curcol = theCol;
            if (rowCount == ltaRow)
              curcol = LTACOLOUR;
            
            if (rowCount == eqRow)
              curcol = EQCOLOUR;
            
            
            outuint(toLCD, curcol);
            outuint(toLCD, curcol);
            
            
          }
                }
            }
          //          stop_streaming_slave(toLCD);
          outct(toLCD, XS1_CT_END);
          chkct(toLCD, XS1_CT_END);
        globclock_timeout += TIMER_POLL_TIME;

        ptp_get_time_info_mod64(ptp, time_info);
        {unsigned t; 
          tmr :> t;
          globtime = local_timestamp_to_ptp_mod32(t, time_info);
        }        
        break;        
        case fromFFTBefore :> x:
          buf[cnt] = x ;
          lta[cnt] = (lta[cnt] * 31 + (x<<8)) >> 5;
          cnt++;
          if (cnt == 64) {
            cnt = 0;
          }
          break;
        case fromEqualiser :> x:
            if (x >=0 && x < BANKS) {
                fromEqualiser :> equaliser[x];
            } else {
                fromEqualiser :> x;
            }
            break;
case tmr when timerafter(globclock_timeout) :> int _:

            break;
        }

    }
}



void freq_buffer(streaming chanend c_in,
                 streaming chanend c_out)
{
  int buf[BUFSIZE]={0};
  int rd = 0, wr = 0;
  unsigned int sample;
  int skip_count = 0;//isSamplePush;
  int count=0;

  while (1) 
    {
      select 
        {
        case c_in :> sample:
          count++;
          skip_count++;
          if (skip_count == 2) {
            int new_wr;
            skip_count = 0;
            sample = (int) (short) ((sample >> 8) & 0xffff);
            buf[wr] = sample;
            wr = (wr+1) & (BUFSIZE-1);
            if (wr == rd)
              rd = (rd + 1) & (BUFSIZE-1);
          }
          break;
        case c_out :> int tmp:
          c_out <: buf[rd];          
          rd = (rd + 1) & (BUFSIZE-1);         
          break;
        }
    }
}
                 

void freq(chanend fromEqualiser, streaming chanend audio, chanend toLCD, 
          chanend ptp)
{
  chan toScreenBefore;
  streaming chan to_fft;

    par {
      freq_buffer(audio, to_fft);
      ffter(to_fft, toScreenBefore);
      realtime(fromEqualiser, toScreenBefore, toLCD, ptp);
    }
}
