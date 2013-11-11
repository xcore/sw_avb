#include <print.h>
#include "avb_1722_common.h"


short ntoh_16(unsigned char x[2]) {
  return ((x[0] << 8) | x[1]);
}

int ntoh_32(unsigned char x[4]) {
  return ((x[0] << 24) | x[1] << 16 | x[2] << 8 | x[3]);
}

void get_64(unsigned char g[8], unsigned char c[8]) {
    for (int i=0; i < 8; i++)
    {
        g[7-i] = c[i];
    }
}

void set_64(unsigned char g[8], unsigned char c[8]) {
    for (int i=0; i < 8; i++)
    {
        g[i] = c[7-i];
    }
}

void hton_16(unsigned char x[2], unsigned short v) {
    x[0] = (v >> 8) & 0xFF;
    x[1] = (v & 0xFF);
}

void hton_32(unsigned char x[4], unsigned int v) {
    x[0] = (unsigned char) (v >> 24);
    x[1] = (unsigned char) (v >> 16);
    x[2] = (unsigned char) (v >> 8);
    x[3] = (unsigned char) (v);
}
