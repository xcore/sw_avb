#ifndef _mac_custom_filter_h_
#define _mac_custom_filter_h_
#include "avb_conf.h"
#include "avb_srp.h"
#include "avb_mvrp.h"
#include "avb_1722_common.h"
#include "avb_1722_router_table.h"
#include <print.h>

#define ROUTER_LINK(n) (1 << (4+n))

#define HTONS(x) ((x>>8)|(((x&0xff)<<8)))

#define MAC_FILTER_1722  0x1
#define MAC_FILTER_PTP   0x2
#define MAC_FILTER_ARPIP 0x4
#define MAC_FILTER_AVB_CONTROL  0x8

#define MII_FILTER_FORWARD_TO_OTHER_PORTS (0x80000000)

#if defined(__XC__)
inline int mac_custom_filter(unsigned int buf[], unsigned int mac[2], int &user_data)
{
  int result = 0;
  unsigned short etype = (unsigned short) buf[3];
  int qhdr = (etype == 0x0081);

  if (qhdr) {
    // has a 802.1q tag - read etype from next word
    etype = (unsigned short) buf[4];
  }

  switch (etype) {
    case HTONS(AVB_SRP_ETHERTYPE):
    case HTONS(AVB_MVRP_ETHERTYPE):
      result = MAC_FILTER_AVB_CONTROL;
      break;
    case 0xf788:
      result = MAC_FILTER_PTP;
      break;
    default:
      if ((buf[0] & 0x1) || // Broadcast
          (buf[0] != mac[0] || buf[1] != mac[1])) // Not unicast
      {
        result |= MII_FILTER_FORWARD_TO_OTHER_PORTS;
      }
      break;
  }

  return result;
}
#endif

#endif
