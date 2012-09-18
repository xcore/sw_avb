#define ETHERNET_DEFAULT_IMPLEMENTATION full

#ifndef ETHERNET_RX_HP_QUEUE 
#define ETHERNET_RX_HP_QUEUE 1
#endif

#ifndef ETHERNET_TX_HP_QUEUE 
#define ETHERNET_TX_HP_QUEUE 1
#endif

//#define ETHERNET_TRAFFIC_SHAPER 1

#define MAC_CUSTOM_FILTER 1
#define AVB_MAC
#include "avb_conf.h"

