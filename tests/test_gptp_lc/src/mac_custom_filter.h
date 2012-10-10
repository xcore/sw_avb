#ifndef _mac_custom_filter_h_
#define _mac_custom_filter_h_

#define MAC_FILTER_1722  0x1
#define MAC_FILTER_PTP   0x2
#define MAC_FILTER_ARPIP 0x4
#define MAC_FILTER_AVB_CONTROL  0x8

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
    default:
      break;
  }

  return result;
}

#endif
