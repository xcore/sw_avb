// Copyright (c) 2011, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#ifndef __nettypes_h__
#define __nettypes_h__

/* Useful types for network packet processing */


/* Host data types - little endian */
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
#ifndef __XC__
typedef long long u64_t;  
#endif
typedef struct { unsigned char data[10]; } u80_t;
typedef struct { unsigned int data[3]; } u96_t;

/* Network data types - big endian  */
typedef unsigned char n8_t;
typedef struct { unsigned char data[2]; } n16_t;
typedef struct { unsigned char data[4]; } n32_t;
typedef struct { unsigned char data[8]; } n64_t;
typedef struct { unsigned char data[10]; } n80_t;
typedef struct { unsigned char data[12]; } n96_t;


inline n16_t hton16(u16_t x) {
  n16_t ret;
  ret.data[0] = x >> 8;
  ret.data[1] = (x & 0xff);
  return ret;
}

inline u16_t ntoh16(n16_t x) {
  return ((x.data[0] << 8) | x.data[1]);
}

inline u32_t ntoh32(n32_t x) {
  return ((x.data[0] << 24) | x.data[1] << 16 | x.data[2] << 8 | x.data[1]);
}

#ifndef __XC__
inline u64_t ntoh64(n64_t x) {
  long long ret=0;
  for (int i=0;i<8;i++)
    ret = (ret << 8) + x.data[i];
  return ret;
}
#endif


inline n32_t hton32(u32_t x) {
  n32_t ret;

  ret.data[0] = ((x >> 24) & 0xff);
  ret.data[1] = ((x >> 16) & 0xff);
  ret.data[2] = ((x >>  8) & 0xff);
  ret.data[3] = ((x >>  0) & 0xff);
  return ret;
}

inline n80_t hton80(u80_t x) {
  n80_t ret;
  for (int i=0;i<10;i++) 
    ret.data[i] = x.data[9-i];
  return ret;
}


/* Ethernet headers */
typedef struct ethernet_hdr_t {
  unsigned char dest_addr[6];
  unsigned char src_addr[6];
  unsigned char ethertype[2];
} ethernet_hdr_t;

typedef struct tagged_ethernet_hdr_t {
  unsigned char dest_addr[6];
  unsigned char src_addr[6];
  unsigned char qtag[2];
  unsigned char ethertype[2];
} tagged_ethernet_hdr_t;

#endif
