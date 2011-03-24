#include <xs1.h>
#include <print.h>
#include <print.h>



/*
int b0o[] = { 619489, 1194043, 2223731, 3891548, 6130479, 8136044,};
int b2o[] = { -619489, -1194043, -2223731, -3891548, -6130479, -8136044,};
int a1o[] = { -32291694, -31074719, -28765182, -24567970, -17409222, -5822246,};
int a2o[] = { 15538238, 14389130, 12329753, 8994119, 4516259, 505128,};

#define BANKS 6
*/

#include "banks.h"
#include "filtercoefficients.h"

struct filterPars {
    int b0[BANKS];
    int corr[BANKS];
    int b2N[BANKS];
    int a1[BANKS];
    int a2N[BANKS];
};

#define MAXGAIN  1
//#define B (1<<31)
#define B 0

void setCoeff(struct filterPars &f, int i, int from, int gain) {
    f.b0[i]   =  ((b0o[from] >> 8) * gain);
    f.b2N[i]  =  ((b2o[from] >> 8) * gain);
    f.a1[i]   = -(a1o[from]);
    f.a2N[i]  = -(a2o[from]);
    //    f.corr[i] = ((f.b0[i] - f.b2N[i] + f.a1[i] - f.a2N[i])<<7) - B;
}


#define Mac(a,b,c,d)  {b,a} = macs(c,d,b,a)
#pragma unsafe arrays
int dofilter(int &xn,
             int &xn1,
             int &xn2,
             int yn1[BANKS],
             int yn2[BANKS],
             struct filterPars &f)
{
  int ynl, ynh;
  int sum=0;
  for(int j=BANKS-1; j>=0; j--) {
    int yn1j = yn1[j];
    ynl = 0;
    ynh = 0;
    Mac(ynl,ynh,f.b2N[j],xn2);
    Mac(ynl, ynh, f.a2N[j], yn2[j]);
    yn2[j] = yn1j;
    Mac(ynl, ynh, f.a1[j], yn1j);
    Mac(ynl, ynh, f.b0[j], xn);
    ynh = (ynh << 8) | (((unsigned) ynl) >> 24);
    yn1[j]= ynh;
    sum += ynh;
  }
  xn2 = xn1; xn1= xn;
  return sum;
}

int filter_ts=0;
#pragma unsafe arrays
void stereo_biquad_filter(chanend gains, streaming chanend sin, streaming chanend sout)
{
    struct filterPars f;
    int xn_l, xn1_l, xn2_l;
    int yn1[BANKS], yn2[BANKS];
    int xn_r, xn1_r, xn2_r;
    int yn1_r[BANKS], yn2_r[BANKS];
    int i;
    int curgain[BANKS], newgain[BANKS];
    int adjust = 10;
    int g = 0;
    int lrc = 1;
    //    unsigned int prev_l = 0, prev_r = 0;
    unsigned counter = 0;
    xn_l = 0; xn1_l = 0; xn2_l = 0;
    xn_r = 0; xn1_r = 0; xn2_r = 0;
    for(i = 0; i<BANKS; i+=1) {
        yn1[i] = B;
        yn2[i] = B;
        yn1_r[i] = B;
        yn2_r[i] = B;
        setCoeff(f, i,i,64);
        curgain[i] = 64;
        newgain[i] = 32;
    }
    xn1_l = B;
    xn2_l = B;
    xn1_r = B;
    xn2_r = B;



    while (1) {
        int x;
        int sum = 0;
        unsigned sample;
        unsigned timestamp;
        select 
          {
          case sout :> timestamp:
            // timestamps coming back from the media output,
            // just pass these back along the xc channel
            filter_ts = timestamp;
            sin <: timestamp;
            break;
          case sin :> sample:
            // a sample to process
            counter++;
            if (lrc) {
              xn_l = ((signed) (sample << 8)) >> 8;
              sum = dofilter(xn_l, xn1_l, xn2_l, yn1, yn2, f);
            }
            else {
              // sign extend
              xn_r = ((signed) (sample << 8)) >> 8;
              sum = dofilter(xn_r, xn1_r, xn2_r, yn1_r, yn2_r, f);
            }
            
            sout <: sum;
            lrc = !lrc;
            break;
          case gains :> x:
            if (x >= 0 && x < BANKS) {
              gains :> newgain[x];
            } else {
              gains :> int _;
            }
            break;
          }
        if (adjust == 0) {
            adjust = 10;
            g = g + 1;
            if (g >= BANKS) g = 0;
            if (curgain[g] != newgain[g]) {
                if (curgain[g] < newgain[g]) {
                    curgain[g] += 1;
                } else {
                    curgain[g] -= 1;
                }
                setCoeff(f, g, g, curgain[g]);
            }
        }
        adjust--;
    }
}
