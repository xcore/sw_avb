#ifndef __avb_conf_h__
#define __avb_conf_h__

// Configuration parameters for the ethernet code
#define PHY_ADDRESS 0x0
#define MAX_ETHERNET_PACKET_SIZE (1518)
#define NUM_MII_RX_BUF 14
#define NUM_MII_TX_BUF 5
#define ETHERNET_RX_HP_QUEUE 1
#define MAX_ETHERNET_CLIENTS   5
#define MII_RX_BUFSIZE_HIGH_PRIORITY (2048)    
#define MII_RX_BUFSIZE_LOW_PRIORITY (2048)
#define MII_TX_BUFSIZE_HIGH_PRIORITY (2048)    
#define MII_TX_BUFSIZE_LOW_PRIORITY (2048)
#define ETHERNET_MAX_TX_HP_PACKET_SIZE (300)

#define MAX_ETHERNET_CLIENTS   5
    
#define MEDIA_OUTPUT_FIFO_WORD_SIZE  (120)

#define MAX_INCOMING_AVB_STREAMS 8

#define AVB_MAX_NAME_LEN 32

#define AVB_MAX_CHANNELS_PER_STREAM 8

#define AVB_NUM_SINKS 1

#define AVB_NUM_SOURCES 1

#define AVB_NUM_LISTENER_UNITS 1 

#define AVB_NUM_TALKER_UNITS 1 

#define AVB_NUM_MEDIA_INPUTS 2

#define AVB_NUM_MEDIA_OUTPUTS 2

#define AVB_NUM_MEDIA_UNITS 2

#define AVB_NUM_MEDIA_CLOCKS 1

// Defining this makes SRP auto-start and auto-stop a stream when listeners come and go
#define SRP_AUTO_TALKER_STREAM_CONTROL

#endif
