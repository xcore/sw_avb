#ifndef __c_io_h__
#define __c_io_h__
#include <xccompat.h>

#ifdef __C_IO_IMPLEMENTATION
#define INLINE 
#else
#define INLINE inline
#endif

INLINE unsigned xc_abi_inuint(chanend c) 
{
  unsigned x;
  __asm__ volatile ("chkct  res[%0], 0x1;"
      "outct  res[%0], 0x1;"::"r"(c));
  __asm__ volatile ("in     %0, res[%1]":"=r"(x):"r"(c));
  __asm__ volatile ("chkct  res[%0], 0x1;"
      "outct  res[%0], 0x1;"::"r"(c));  
  return x;
}

INLINE void xc_abi_outuint(chanend c, unsigned x) 
{
  __asm__ volatile ("outct  res[%0], 0x1;"
      "chkct  res[%0], 0x1;"
      "out    res[%0], %1;"
      "outct  res[%0], 0x1;"
      "chkct  res[%0], 0x1;"  :: "r"(c),"r"(x));
  return;
}


#endif // __c_io_h__
