#ifndef __avb_conf_h__
#define __avb_conf_h__

/* Debug Switches */
//#define AVB_1722_DEBUG_SHOW_FIRST_PACKET
#define ETHERNET_COUNT_PACKETS

/* Configuration parameters for the ethernet code */
#define PHY_ADDRESS 0x0
#define MAX_ETHERNET_PACKET_SIZE (1518)
#define NUM_MII_RX_BUF 6
#define NUM_MII_TX_BUF 3
#define ETHERNET_RX_HP_QUEUE 1
#define MAX_ETHERNET_CLIENTS   5
#ifndef MII_RX_BUFSIZE_HIGH_PRIORITY
  #define MII_RX_BUFSIZE_HIGH_PRIORITY (700)    // override in Makefile for high channel configs
#endif
#define MII_RX_BUFSIZE_LOW_PRIORITY (300)
#define MII_TX_BUFSIZE_HIGH_PRIORITY (300)
#define MII_TX_BUFSIZE_LOW_PRIORITY (200)
#define ETHERNET_MAX_TX_HP_PACKET_SIZE (300)

/* General purpose AVB configuration */
#define AVB_MAX_NAME_LEN 25

/* Listener configuration */

/* Talker configuration */
#define AVB_NUM_SOURCES 0
#define AVB_NUM_TALKER_UNITS 0
#define AVB_NUM_SDATA_IN 0

/* Media configuration */
#define AVB_1722_FORMAT_61883_6
#define AVB_NUM_MEDIA_INPUTS 0
#define AVB_NUM_MEDIA_UNITS 2

/* Media clock configuration */
#define AVB_NUM_MEDIA_CLOCKS 1

/* Add synths from channels 3/4 upwards in I2S for this demo */
//#define I2S_SYNTH_FROM 1

// Defining this makes SRP auto-start and auto-stop a stream when listeners come and go
#define SRP_AUTO_TALKER_STREAM_CONTROL

// XScope
// #define XSCOPE_OUTPUT_FIFO_PULL

//==========================================================================================
// Defaults
//==========================================================================================
#ifndef AVB_NUM_SINKS
#define AVB_NUM_SINKS 2
#endif

#ifndef AVB_NUM_LISTENER_UNITS
#define AVB_NUM_LISTENER_UNITS 2
#endif

#ifndef AVB_NUM_MEDIA_OUTPUTS
#define AVB_NUM_MEDIA_OUTPUTS 16
#endif

#ifndef AVB_MAX_AUDIO_SAMPLE_RATE
// standard sample rate. Allows for more channels
#define AVB_MAX_AUDIO_SAMPLE_RATE 48000
#endif

//==========================================================================================
// Derived Defines.
//==========================================================================================

//#define AVB_MAX_CHANNELS_PER_STREAM (AVB_CHANNELS_PER_SOURCE >= AVB_CHANNELS_PER_SINK) ? AVB_CHANNELS_PER_SOURCE : AVB_CHANNELS_PER_SINK
#define AVB_MAX_CHANNELS_PER_STREAM 16

#define AVB_SINK_MAP_SIZE (AVB_NUM_MEDIA_OUTPUTS/AVB_NUM_SINKS)

#if(AVB_AUDIO_IF_i2s)
#define AVB_NUM_SDATA_OUT AVB_NUM_MEDIA_OUTPUTS/2
// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512
#endif

#if(AVB_AUDIO_IF_tdm_multi)
// How many channels there are in the TDM window
#define TDM_NUM_CHANNELS 8
// Must be a multiple of 4
#define CLOCKS_PER_CHANNEL 32
#define MASTER_TO_WORDCLOCK_RATIO TDM_NUM_CHANNELS*CLOCKS_PER_CHANNEL
#define AVB_NUM_SDATA_OUT (AVB_NUM_MEDIA_OUTPUTS+(TDM_NUM_CHANNELS-1))/TDM_NUM_CHANNELS
#endif

//==========================================================================================
// Checks
//==========================================================================================
//// i2s specific checks
#if(AVB_AUDIO_IF_i2s)
#if(AVB_NUM_MEDIA_INPUTS>16)
#error "AVB_NUM_MEDIA_INPUTS exceeds 16"
#endif

#if(AVB_NUM_MEDIA_OUTPUTS>16)
#error "AVB_NUM_MEDIA_OUTPUTS exceeds 16"
#endif

#if((AVB_NUM_MEDIA_INPUTS+AVB_NUM_MEDIA_OUTPUTS)>16)
#error "AVB_NUM_MEDIA_INPUTS+AVB_NUM_MEDIA_OUTPUTS exceeds 16, the max channels over 8 i2s lines"
#endif
#endif

#if(AVB_NUM_MEDIA_INPUTS!=0)
#error "test_api_listener doesn't support input channels"
#endif

#if(AVB_NUM_SOURCES!=0)
#error "test_api_listener doesn't support input streams"
#endif

#if(AVB_NUM_LISTENER_UNITS>AVB_NUM_SINKS)
#error "AVB_NUM_SINKS must be >= AVB_NUM_LISTENER_UNITS"
#endif

#if(AVB_NUM_SINKS%AVB_NUM_LISTENER_UNITS!=0)
#error "AVB_NUM_SINKS must be a multiple of AVB_NUM_LISTENER_UNITS"
#endif

#if(AVB_NUM_LISTENER_UNITS>2)
#warning "test_api_listener has only been tested with 1 or 2 Listeners"
#endif

#if(AVB_NUM_SDATA_OUT>7)
#warning "J12 jumper on AVB Board must be removed because SDATA_IN0 is reconfigured as output!!!"
#endif

#endif // __avb_conf_h__
