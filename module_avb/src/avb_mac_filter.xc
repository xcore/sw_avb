#include "avb_conf.h"

#if !defined(ETHERNET_USE_AVB_FILTER) || ETHERNET_USE_AVB_FILTER
#include "avb_mac_filter.h"
extern inline int mac_custom_filter(unsigned int buf[], unsigned int mac[2], int &user_data);
#endif
