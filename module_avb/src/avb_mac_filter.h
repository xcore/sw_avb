#ifndef _mac_custom_filter_h_
#define _mac_custom_filter_h_
#include "avb_conf.h"
#include "avb_srp.h"
#include "avb_mmrp.h"
#include "avb_mvrp.h"
#include "avb_1722_common.h"
#include "avb_1722_router_table.h"
#include <print.h>

#define MAC_FILTER_1722  0x1
#define MAC_FILTER_PTP   0x2
#define MAC_FILTER_ARPIP 0x4
#define MAC_FILTER_AVB_CONTROL  0x8

#define ROUTER_LINK(n) (1 << (4+n))

#define HTONS(x) ((x>>8)|(((x&0xff)<<8)))

#if defined(__XC__) && (!defined(ETHERNET_USE_AVB_FILTER) || ETHERNET_USE_AVB_FILTER)
#pragma unsafe arrays
inline int mac_custom_filter(unsigned int buf[])
{
  int result = 0;
  unsigned short etype = (unsigned short) buf[3];
  int qhdr = (etype == 0x0081);
  
  if (qhdr) {
    // has a 802.1q tag - read etype from next word
    etype = (unsigned short) buf[4];
  }

  switch (etype) {
    case 0xf788:
      result = MAC_FILTER_PTP;
      break;
    case HTONS(AVB_SRP_ETHERTYPE):
    case HTONS(AVB_MMRP_ETHERTYPE):
    case HTONS(AVB_MVRP_ETHERTYPE):
      result = MAC_FILTER_AVB_CONTROL;
      break;
    case 0x0608:
    case 0x0008:
      result = MAC_FILTER_ARPIP;
      break;
    case (((AVB_1722_ETHERTYPE & 0xff) << 8) | (AVB_1722_ETHERTYPE >> 8)):
      {
        int cd_flag;
        if (qhdr) {
          cd_flag = (buf[4] >> 23) & 1;
        }
        else {
          cd_flag = (buf[3] >> 23) & 1;
        }
        if (cd_flag) 
          result = MAC_FILTER_AVB_CONTROL;
        else {
          // route the 1722 streams
          unsigned id0, id1;
          unsigned link, hash;
          int lookup;
          if (qhdr) {
            id0 = (buf[7] << 16 | buf[5]>>16);
            id1 = buf[6];
          }
          else {
            id0 = (buf[6] << 16 | buf[4]>>16);
            id1 = buf[5];
          }
          lookup = 
            avb_1722_router_table_lookup(id0,
                                         id1,
                                         link,
                                         hash);
          if (lookup) {
            buf[0] = hash << 16;
            buf[1] = 0x0;
            result = ROUTER_LINK(link);
          }

        }
      }
      break;
    default:
      break;
  }

  return result;
}
#endif

#endif


