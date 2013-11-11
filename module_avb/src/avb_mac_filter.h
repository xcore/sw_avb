#ifndef _mac_custom_filter_h_
#define _mac_custom_filter_h_
#include "avb_conf.h"
#include "avb_srp.h"
#include "avb_mvrp.h"
#include "avb_1722_common.h"
#include "avb_1722_router_table.h"
#include <print.h>

enum mac_clients {
  MAC_FILTER_1722=0,
  MAC_FILTER_PTP,
  MAC_FILTER_AVB_CONTROL,
  MAC_FILTER_AVB_SRP,
  NUM_FILTER_CLIENTS
};

#define ROUTER_LINK(n) (1 << (NUM_FILTER_CLIENTS+n))

#define HTONS(x) ((x>>8)|(((x&0xff)<<8)))

#define MII_FILTER_FORWARD_TO_OTHER_PORTS (0x80000000)

#if defined(__XC__) && (!defined(ETHERNET_USE_AVB_FILTER) || ETHERNET_USE_AVB_FILTER)
#pragma unsafe arrays
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
    case 0xf788:
      result = MAC_FILTER_PTP;
      break;
    case HTONS(AVB_SRP_ETHERTYPE):
    case HTONS(AVB_MVRP_ETHERTYPE):
      result = MAC_FILTER_AVB_SRP;
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
        {
          result = MAC_FILTER_AVB_CONTROL;
#if NUM_ETHERNET_MASTER_PORTS == 2
          if ((buf[0] & 0x1) || // Broadcast
          (buf[0] != mac[0] || buf[1] != mac[1])) // Not unicast
          {
            result |= MII_FILTER_FORWARD_TO_OTHER_PORTS;
          }
#endif
        }
        else {
          // route the 1722 streams
          unsigned id0, id1;
          int link, hash, f0rward;
          int lookup;
          if (qhdr) {
            id0 = (buf[7] << 16 | buf[5]>>16);
            id1 = buf[6];
          }
          else {
            id0 = (buf[6] << 16 | buf[4]>>16);
            id1 = buf[5];
          }
#pragma xta endpoint "hp_1722_lookup"
          lookup =
            avb_1722_router_table_lookup(id0,
                                         id1,
                                         link,
                                         hash,
                                         f0rward);

          if (lookup) {
            if (link != -1)
            {
              result = ROUTER_LINK(link);
            }
            else
            {
              result = 0;
            }
            user_data = hash;
#if NUM_ETHERNET_MASTER_PORTS == 2
            if (f0rward)
            {
              result |= MII_FILTER_FORWARD_TO_OTHER_PORTS;
            }
#endif
          }

        }
      }
      break;
    default:
#if NUM_ETHERNET_MASTER_PORTS == 2
      if ((buf[0] & 0x1) || // Broadcast
          (buf[0] != mac[0] || buf[1] != mac[1])) // Not unicast
      {
        result |= MII_FILTER_FORWARD_TO_OTHER_PORTS;
      }
#endif
      break;
  }

  return result;
}
#endif

#endif


