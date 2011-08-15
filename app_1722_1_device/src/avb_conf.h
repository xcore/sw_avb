#ifndef __avb_conf_h__
#define __avb_conf_h__

/* Configuration parameters for the ethernet code */
#define PHY_ADDRESS 0x0
#define MAX_ETHERNET_PACKET_SIZE (1518)
#define NUM_MII_RX_BUF 6
#define NUM_MII_TX_BUF 3
#define ETHERNET_RX_HP_QUEUE 1
#define MAX_ETHERNET_CLIENTS   5
#define MII_RX_BUFSIZE_HIGH_PRIORITY (700)    
#define MII_RX_BUFSIZE_LOW_PRIORITY (300)
#define MII_TX_BUFSIZE_HIGH_PRIORITY (300)
#define MII_TX_BUFSIZE_LOW_PRIORITY (200)
#define ETHERNET_MAX_TX_HP_PACKET_SIZE (300)

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
#define AVB_NUM_MEDIA_OUTPUTS 2
#define AVB_NUM_MEDIA_INPUTS 2
#define AVB_NUM_MEDIA_UNITS 2

/* Media clock configuration */
#define AVB_NUM_MEDIA_CLOCKS 1

/* Add synths from channels 3/4 upwards in I2S for this demo */
#define I2S_SYNTH_FROM 1

#define USE_1722_1

#define AVB_1722_1_ADP_ENTITY_CAPABILITIES  	(AVB_1722_1_ADP_ENTITY_CAPABILITIES_AVDECC_CONTROL)
#define AVB_1722_1_ADP_LISTENER_STREAM_SINKS 	1
#define AVB_1722_1_ADP_LISTENER_CAPABILITIES 	(AVB_1722_1_ADP_LISTENER_CAPABILITIES_IMPLEMENTED|AVB_1722_1_ADP_LISTENER_CAPABILITIES_AUDIO_SINK)
#define AVB_1722_1_ADP_TALKER_STREAM_SOURCES 	1
#define AVB_1722_1_ADP_TALKER_CAPABILITIES 		(AVB_1722_1_ADP_TALKER_CAPABILITIES_IMPLEMENTED|AVB_1722_1_ADP_TALKER_CAPABILITIES_AUDIO_SOURCE)

#define AVB_1722_SAF 1

// Defining this makes SRP auto-start and auto-stop a stream when listeners come and go
#define SRP_AUTO_TALKER_STREAM_CONTROL

#endif
