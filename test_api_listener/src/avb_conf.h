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

/* Listener configuration */
#define AVB_NUM_SINKS 2
#define AVB_CHANNELS_PER_SINK 16
#define AVB_NUM_LISTENER_UNITS 2

/* Talker configuration */
#define AVB_NUM_SOURCES 0
#define AVB_NUM_TALKER_UNITS 0

/* Media configuration */
#define AVB_1722_FORMAT_61883_6
#define AVB_NUM_MEDIA_OUTPUTS 16  // only map half the available channels (2*16) to output FIFO
#define AVB_NUM_MEDIA_INPUTS 0
#define AVB_NUM_MEDIA_UNITS 2

/* Media clock configuration */
#define AVB_NUM_MEDIA_CLOCKS 1

/* Allow for high sample rates */
#define AVB_MAX_AUDIO_SAMPLE_RATE 48000

/* Add synths from channels 3/4 upwards in I2S for this demo */
//#define I2S_SYNTH_FROM 1

// Defining this makes SRP auto-start and auto-stop a stream when listeners come and go
#define SRP_AUTO_TALKER_STREAM_CONTROL

// XScope
// #define XSCOPE_OUTPUT_FIFO_PULL

#define AVB_NUM_SDATA_OUT AVB_NUM_MEDIA_OUTPUTS/2
#define AVB_NUM_SDATA_IN AVB_NUM_MEDIA_INPUTS/2

//==========================================================================================
// Defaults
//==========================================================================================
#ifndef AVB_NUM_SINKS
#define AVB_NUM_SINKS 1
#endif

#ifndef AVB_CHANNELS_PER_SOURCE
#define AVB_CHANNELS_PER_SOURCE 0
#endif

//==========================================================================================
// Derived Defines.
//==========================================================================================

//#define AVB_MAX_CHANNELS_PER_STREAM (AVB_CHANNELS_PER_SOURCE >= AVB_CHANNELS_PER_SINK) ? AVB_CHANNELS_PER_SOURCE : AVB_CHANNELS_PER_SINK
#define AVB_MAX_CHANNELS_PER_STREAM 16

#define AVB_SINK_MAP_SIZE (AVB_NUM_MEDIA_OUTPUTS/AVB_NUM_SINKS)

#endif

//==========================================================================================
// Checks
//==========================================================================================
#if(AVB_NUM_MEDIA_INPUTS>16)
#error "AVB_NUM_MEDIA_INPUTS exceeds 16"
#endif

#if(AVB_NUM_MEDIA_OUTPUTS>16)
#error "AVB_NUM_MEDIA_OUTPUTS exceeds 16"
#endif

#if((AVB_NUM_MEDIA_INPUTS+AVB_NUM_MEDIA_OUTPUTS)>16)
#error "AVB_NUM_MEDIA_INPUTS+AVB_NUM_MEDIA_OUTPUTS exceeds 16, the max channels over 8 i2s lines"
#endif
