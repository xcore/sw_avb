#ifndef __avb_conf_h__
#define __avb_conf_h__

//#define XSCOPE_1722_TALKER
#define PRINT
#define ETHERNET_COUNT_PACKETS  //activate MII error counters

//#define BUGFIX_12860

//==========================================================================================
// Configuration parameters for the ethernet code
//==========================================================================================

#define PHY_ADDRESS 0x0
#define MAX_ETHERNET_PACKET_SIZE (1518)
#define NUM_MII_RX_BUF 6
#define NUM_MII_TX_BUF 3
#define ETHERNET_RX_HP_QUEUE 1
#define MAX_ETHERNET_CLIENTS   5
#ifndef MII_RX_BUFSIZE_HIGH_PRIORITY
#define MII_RX_BUFSIZE_HIGH_PRIORITY (700)
#endif
#define MII_RX_BUFSIZE_LOW_PRIORITY (300)
#define MII_TX_BUFSIZE_HIGH_PRIORITY (300)
#define MII_TX_BUFSIZE_LOW_PRIORITY (200)
#define ETHERNET_MAX_TX_HP_PACKET_SIZE (300)

//==========================================================================================
// General purpose AVB configuration
//==========================================================================================
#define AVB_MAX_NAME_LEN 25
#ifndef AVB_CHANNELS_PER_SINK
  #define AVB_CHANNELS_PER_SINK 0
#endif
#ifndef AVB_CHANNELS_PER_SOURCE
  #define AVB_CHANNELS_PER_SOURCE 0
#endif

#ifdef TALKER
/* Talker configuration */
#ifndef AVB_NUM_SOURCES
  #define AVB_NUM_SOURCES 2
#endif
#define AVB_NUM_TALKER_UNITS 1
#ifndef LISTENER  // avoid redefine warnings
  #define AVB_NUM_MEDIA_UNITS 1
#endif
#endif

#ifdef LISTENER
/* Listener configuration */
#ifndef AVB_NUM_SINKS
  #define AVB_NUM_SINKS 2
#endif
#define AVB_NUM_LISTENER_UNITS 1
#define AVB_NUM_MEDIA_UNITS 2
#endif


/* Media configuration */
//#define AVB_1722_FORMAT_61883_6

/* Media clock configuration */
#define AVB_NUM_MEDIA_CLOCKS 1

#ifndef AVB_MAX_AUDIO_SAMPLE_RATE
/* Allow for high sample rates */
#define AVB_MAX_AUDIO_SAMPLE_RATE 96000
#endif

/* Add synths from channels 3/4 upwards in I2S for this demo */
//#define I2S_SYNTH_FROM 1

// Defining this makes SRP auto-start and auto-stop a stream when listeners come and go
#define SRP_AUTO_TALKER_STREAM_CONTROL

//==========================================================================================
// Defaults
//==========================================================================================
#ifndef AVB_NUM_SINKS
#define AVB_NUM_SINKS 1
#endif

#ifndef AVB_NUM_LISTENER_UNITS
#define AVB_NUM_LISTENER_UNITS 0
#endif

#ifndef AVB_NUM_SOURCES
#define AVB_NUM_SOURCES 0
#endif

#ifndef AVB_NUM_TALKER_UNITS
#define AVB_NUM_TALKER_UNITS 0
#endif

#ifndef AVB_AUDIO_IF
#define AVB_AUDIO_IF i2s
#endif


//==========================================================================================
// Derived Defines.
//==========================================================================================
#ifndef AVB_NUM_MEDIA_INPUTS
#define AVB_NUM_MEDIA_INPUTS (AVB_NUM_SOURCES*AVB_CHANNELS_PER_SOURCE)
#endif

#ifndef AVB_NUM_MEDIA_OUTPUTS
#define AVB_NUM_MEDIA_OUTPUTS (AVB_NUM_SINKS*AVB_CHANNELS_PER_SINK)
#endif

#if(AVB_AUDIO_IF_i2s)
#define AVB_NUM_SDATA_OUT AVB_NUM_MEDIA_OUTPUTS/2
#define AVB_NUM_SDATA_IN AVB_NUM_MEDIA_INPUTS/2
// This is the number of master clocks in a word clock
#define MASTER_TO_WORDCLOCK_RATIO 512
#endif

#if(AVB_AUDIO_IF_tdm_multi)
// How many channels there are in the TDM window
#define TDM_NUM_CHANNELS 8
// Must be a multiple of 4
#define CLOCKS_PER_CHANNEL 32
#define MASTER_TO_WORDCLOCK_RATIO TDM_NUM_CHANNELS*CLOCKS_PER_CHANNEL
#define AVB_NUM_SDATA_OUT AVB_NUM_MEDIA_OUTPUTS/TDM_NUM_CHANNELS
#define AVB_NUM_SDATA_IN AVB_NUM_MEDIA_INPUTS/TDM_NUM_CHANNELS
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

#if(AVB_NUM_SDATA_OUT>7)
#warning "J12 jumper on AVB Board must be removed because SDATA_IN0 is reconfigured as output!!!"
#endif

// Todo: This should be per Talker/Listener
//#define AVB_MAX_CHANNELS_PER_STREAM (AVB_CHANNELS_PER_SOURCE >= AVB_CHANNELS_PER_SINK) ? AVB_CHANNELS_PER_SOURCE : AVB_CHANNELS_PER_SINK
// 8 Fails for some reason
#define AVB_MAX_CHANNELS_PER_STREAM 24

#endif
