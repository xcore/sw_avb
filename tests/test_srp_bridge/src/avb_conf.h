#ifndef __avb_conf_h__
#define __avb_conf_h__

/******** ETHERNET MAC CONFIGURATION PARAMETERS *************************************************/
#define ETHERNET_DEFAULT_IMPLEMENTATION full

#define MAX_ETHERNET_PACKET_SIZE (1518)

#define NUM_MII_RX_BUF 6
#define NUM_MII_TX_BUF 3

#define ETHERNET_RX_HP_QUEUE 1
#define ETHERNET_TX_HP_QUEUE 1

#define MAX_ETHERNET_CLIENTS   3

#define ETHERNET_MAX_TX_HP_PACKET_SIZE (300)
#define ETHERNET_MAX_TX_LP_PACKET_SIZE (1518)

#define MII_RX_BUFSIZE_HIGH_PRIORITY (1100 + (3*(ETHERNET_MAX_TX_HP_PACKET_SIZE)))
#define MII_RX_BUFSIZE_LOW_PRIORITY (1518*3)

#define MII_TX_BUFSIZE_HIGH_PRIORITY (1100 + (3*(ETHERNET_MAX_TX_HP_PACKET_SIZE)))
#define MII_TX_BUFSIZE_LOW_PRIORITY (2000)

#define NUM_ETHERNET_PORTS 2
#define NUM_ETHERNET_MASTER_PORTS 2

#define ETHERNET_RX_ENABLE_TIMER_OFFSET_REQ 1


#define AVB_NUM_SOURCES 0
#define AVB_NUM_TALKER_UNITS 0
#define AVB_NUM_MEDIA_INPUTS 0
#define AVB_1722_1_TALKER_ENABLED 0

#define AVB_NUM_SINKS 0
#define AVB_NUM_LISTENER_UNITS 0
#define AVB_NUM_MEDIA_OUTPUTS 0
#define AVB_1722_1_LISTENER_ENABLED 0

#define AVB_ENABLE_1722_1 0

/** The maximum number of channels permitted per 1722 Talker stream */
#define AVB_MAX_CHANNELS_PER_TALKER_STREAM 8
/** The maximum number of channels permitted per 1722 Listener stream */
#define AVB_MAX_CHANNELS_PER_LISTENER_STREAM 8


/** The number of components in the endpoint that will register and initialize media FIFOs 
    (typically an audio interface component such as I2S). */
#define AVB_NUM_MEDIA_UNITS 0

/** The number of media clocks in the endpoint. Typically the number of clock domains, each with a 
  * separate PLL and master clock */
#define AVB_NUM_MEDIA_CLOCKS 1

#define AVB_1722_FORMAT_61883_6 1

 #define MRP_MAX_ATTRS 20

#endif