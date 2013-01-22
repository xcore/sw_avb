#ifndef __avb_conf_h__
#define __avb_conf_h__

#include "app_config.h"

/* Some of the configuration depends on the app_config.h file included above */

/******** ETHERNET MAC CONFIGURATION PARAMETERS *************************************************/
#define MAX_ETHERNET_PACKET_SIZE (1518)
#define NUM_MII_RX_BUF 6
#define NUM_MII_TX_BUF 3
#define ETHERNET_RX_HP_QUEUE 1
#define MAX_ETHERNET_CLIENTS   3
#define ETHERNET_MAX_TX_HP_PACKET_SIZE (100 + AVB_DEMO_NUM_CHANNELS * 24)
#define ETHERNET_MAX_TX_LP_PACKET_SIZE (500)
#define MII_RX_BUFSIZE_HIGH_PRIORITY (1100 + (3*(ETHERNET_MAX_TX_HP_PACKET_SIZE)))
#define MII_RX_BUFSIZE_LOW_PRIORITY (2000)
#define MII_TX_BUFSIZE_HIGH_PRIORITY (1100 + (3*(ETHERNET_MAX_TX_HP_PACKET_SIZE)))
#define MII_TX_BUFSIZE_LOW_PRIORITY (2000)

/******** ENDPOINT AUDIO AND CLOCKING PARAMETERS ************************************************/

/* Talker configuration */
#if AVB_DEMO_ENABLE_TALKER

/** The total number of AVB sources (streams that are to be transmitted). */
#define AVB_NUM_SOURCES 1
/** The total number or Talker components (typically the number of
  * tasks running the  :c:func:`avb_1722_talker` function). */
#define AVB_NUM_TALKER_UNITS 1
/** The total number of media inputs (typically number of I2S input channels). */
#define AVB_NUM_MEDIA_INPUTS AVB_DEMO_NUM_CHANNELS
/** Enable the 1722.1 Talker functionality */
#define AVB_1722_1_TALKER_ENABLED 1

#else

#define AVB_NUM_SOURCES 0
#define AVB_NUM_TALKER_UNITS 0
#define AVB_NUM_MEDIA_INPUTS 0
#define AVB_1722_1_TALKER_ENABLED 0

#endif

/* Listener configuration */
#if AVB_DEMO_ENABLE_LISTENER

/** The total number of AVB sinks (incoming streams that can be listened to) */
#define AVB_NUM_SINKS 1
/** The total number or listener components
  * (typically the number of tasks running the  :c:func:`avb_1722_listener` function) */
#define AVB_NUM_LISTENER_UNITS 1
/** The total number of media outputs (typically the number of I2S output channels). */
#define AVB_NUM_MEDIA_OUTPUTS AVB_DEMO_NUM_CHANNELS
/** Enable the 1722.1 Listener functionality */
#define AVB_1722_1_LISTENER_ENABLED 1

#else

#define AVB_NUM_SINKS 0
#define AVB_NUM_LISTENER_UNITS 0
#define AVB_NUM_MEDIA_OUTPUTS 0
#define AVB_1722_1_LISTENER_ENABLED 0

#endif

/** The maximum number of channels permitted per 1722 Talker stream */
#define AVB_MAX_CHANNELS_PER_TALKER_STREAM AVB_DEMO_NUM_CHANNELS
/** The maximum number of channels permitted per 1722 Listener stream */
#define AVB_MAX_CHANNELS_PER_LISTENER_STREAM AVB_DEMO_NUM_CHANNELS

/** Enable combination of the media clock server and PTP server in a single core */
#define COMBINE_MEDIA_CLOCK_AND_PTP 1

/** Use 61883-6 audio format for 1722 streams */
#define AVB_1722_FORMAT_61883_6 1

/** The number of components in the endpoint that will register and initialize media FIFOs 
    (typically an audio interface component such as I2S). */
#define AVB_NUM_MEDIA_UNITS 1

/** The number of media clocks in the endpoint. Typically the number of clock domains, each with a 
  * separate PLL and master clock */
#define AVB_NUM_MEDIA_CLOCKS 1

/** Add sine wave synthesis from channels ``I2S_SYNTH_FROM*2`` upwards in the I2S component */
#define I2S_SYNTH_FROM 1

/** The maximum sample rate in Hz of audio that is to be input or output */
#define AVB_MAX_AUDIO_SAMPLE_RATE 96000

// Fix for Apple
#define MEDIA_OUTPUT_FIFO_WORD_SIZE (AVB_MAX_AUDIO_SAMPLE_RATE/300)


/******** 1722.1 PARAMETERS *****************************************************************/

/** Enable 1722.1 AVDECC on the entity */
#define AVB_ENABLE_1722_1 1

/** Enable 1722.1 Controller functionality on the entity. */
#define AVB_1722_1_CONTROLLER_ENABLED 1


/** Enable SRP auto-start and auto-stop a stream when Listeners come and go */
#define SRP_AUTO_TALKER_STREAM_CONTROL 1

#endif
