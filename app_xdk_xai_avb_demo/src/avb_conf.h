#ifndef __avb_conf_h__
#define __avb_conf_h__

/* Configuration parameters for the ethernet code */
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
    
/* General purpose AVB configuration */
#define AVB_MAX_NAME_LEN 25
#define AVB_MAX_CHANNELS_PER_STREAM 8

/* Listener configuration */
#define AVB_NUM_SINKS 1
#define AVB_NUM_LISTENER_UNITS 1

/* Talker configuration */
#define AVB_NUM_SOURCES 1
#define AVB_NUM_TALKER_UNITS 1 

/* Media configuration */
#define AVB_NUM_MEDIA_OUTPUTS 8
#define AVB_NUM_MEDIA_INPUTS 8
#define AVB_MAX_SAMPLE_RATE 48000
#define AVB_NUM_MEDIA_UNITS 2

/* Media clock configuration */
#define AVB_NUM_MEDIA_CLOCKS 1

/* Add synths from channels 7/8 */
#define I2S_SYNTH_FROM 3

#endif
