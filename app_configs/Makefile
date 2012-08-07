# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = XR-AVB-LC-BRD

# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to 
# the project name
APP_NAME =

# Verbosity of Build Messages
VERBOSE = 0

# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
#
#    XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
#
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to 
# xcc for the final link (mapping) stage.

#debug switches
XSCOPE = 0

###########################################################################
# Settings derived from configuration names
###########################################################################

# if config name contains TDM, use the TDM multi interface
ifneq (,$(findstring TDM,$(CONFIG))) 
AUDIO_INTERFACE = tdm_multi  # one of i2s, tdm, tdm_multi
endif

# if config name contains SAF, use  the Simple Audio Format
ifneq (,$(findstring SAF,$(CONFIG))) 
AUDIO_FORMAT = SAF
endif


###########################################################################
# Default settings
###########################################################################

ifndef AUDIO_INTERFACE
AUDIO_INTERFACE = i2s
endif

# defaults
ifndef AUDIO_FORMAT
AUDIO_FORMAT = 61883_6
endif

LISTENER_DEBUG_FLAGS = -DAVB_1722_RECORD_ERRORS 

###########################################################################
# Common Flags
###########################################################################
XCC_FLAGS = -O3 -save-temps -g -report -Xmapper --map -Xmapper MAPFILE
XCC_FLAGS += -DAVB_1722_RECORD_ERRORS -DAVB_AUDIO_IF_$(AUDIO_INTERFACE) -DAVB_1722_FORMAT_$(AUDIO_FORMAT)
#XCC_FLAGS += -fllvm
#XCC_FLAGS = -lxscope

XSCOPE_LIB = xscope

ifeq ($(XSCOPE),1)
XCC_FLAGS += -l$(XSCOPE_LIB) -DUSE_XSCOPE #-DUSE_XSCOPE_PROBES
endif


###########################################################################
#############################  Configurations #############################
###########################################################################
# Configuration naming convention:
# example: 1t1s16ch_2l2s32ch_TDM
# $X[tl]$Ys$Zch[_$if]
# $Xt: 	number of talkers
# $Xl: 	number of listeners
# $Ys: 	number of streams
# $Zch: number of channels
# $if:	optional: audio interface other than i2s


#----------------------------- Talker Only -------------------------------#
# Config specific flags

XCC_FLAGS_1t1s16ch = -DTALKER $(XCC_FLAGS) -DUSE_LOCAL_CLOCK -DAVB_MAX_AUDIO_SAMPLE_RATE=48000 
XCC_FLAGS_1t1s16ch += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=16 

XCC_FLAGS_1t1s16ch_TDM = $(XCC_FLAGS_1t1s16ch)
#same flags as XCC_FLAGS_1t1s16ch. TDM in name will cause AUDIO_INTERFACE to be set to tdm_multi

XCC_FLAGS_Talker = -DTALKER $(XCC_FLAGS)

XCC_FLAGS_Talker2x4Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker2x4Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x4Chan += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=4 -DUSE_LOCAL_CLOCK

XCC_FLAGS_Talker4x2Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x2Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2 -DUSE_LOCAL_CLOCK

XCC_FLAGS_Talker4x2Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x2Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2 -DUSE_LOCAL_CLOCK

XCC_FLAGS_Talker4x2ChanSAF = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2ChanSAF += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x2ChanSAF += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2 -DUSE_LOCAL_CLOCK

XCC_FLAGS_Talker4x3Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x3Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x3Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=3 -DUSE_LOCAL_CLOCK

XCC_FLAGS_Talker7x2Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker7x2Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker7x2Chan += -DAVB_NUM_SOURCES=7 -DAVB_CHANNELS_PER_SOURCE=2 -DUSE_LOCAL_CLOCK

XCC_FLAGS_Talker1x8Chan192kHz = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker1x8Chan192kHz += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker1x8Chan192kHz += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=8 -DUSE_LOCAL_CLOCK -DAVB_MAX_AUDIO_SAMPLE_RATE=129000

#----------------------------- Listener Only -------------------------------#
XCC_FLAGS_Listener = -DLISTENER $(XCC_FLAGS)

XCC_FLAGS_Listener2x4Chan = -DLISTENER $(XCC_FLAGS) 
XCC_FLAGS_Listener2x4Chan += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener2x4Chan += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=4

XCC_FLAGS_Listener4x2Chan = -DLISTENER $(XCC_FLAGS) 
XCC_FLAGS_Listener4x2Chan += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener4x2Chan += -DAVB_NUM_SINKS=4 -DAVB_CHANNELS_PER_SINK=2

XCC_FLAGS_Listener4x2ChanSAF = -DLISTENER $(XCC_FLAGS) 
XCC_FLAGS_Listener4x2ChanSAF += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener4x2ChanSAF += -DAVB_NUM_SINKS=4 -DAVB_CHANNELS_PER_SINK=2 -DAVB_1722_FORMAT_SAF

XCC_FLAGS_Listener4x3Chan = -DLISTENER $(XCC_FLAGS) 
XCC_FLAGS_Listener4x3Chan += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener4x3Chan += -DAVB_NUM_SINKS=4 -DAVB_CHANNELS_PER_SINK=3

XCC_FLAGS_Listener7x2Chan = -DLISTENER $(XCC_FLAGS) 
XCC_FLAGS_Listener7x2Chan += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener7x2Chan += -DAVB_NUM_SINKS=7 -DAVB_CHANNELS_PER_SINK=2

XCC_FLAGS_Listener1x8Chan192kHz = -DLISTENER $(XCC_FLAGS) 
XCC_FLAGS_Listener1x8Chan192kHz += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener1x8Chan192kHz += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=8


#---------------------------Talker and Listener ----------------------------#
XCC_FLAGS_TalkerListener = -DLISTENER -DTALKER $(XCC_FLAGS)
XCC_FLAGS_TalkerListener += $(LISTENER_DEBUG_FLAGS) $(TALKER_DEBUG_FLAGS)

XCC_FLAGS_Talker4x2Chan_Listener4x2Chan = -DLISTENER -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2Chan_Listener4x2Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2 
XCC_FLAGS_Talker4x2Chan_Listener4x2Chan += -DAVB_NUM_SINKS=4  -DAVB_CHANNELS_PER_SINK=2 
XCC_FLAGS_Talker4x2Chan_Listener4x2Chan += $(LISTENER_DEBUG_FLAGS) $(TALKER_DEBUG_FLAGS)

# Pair of Assymetric configs
XCC_FLAGS_Talker2x6Chan_Listener1x4Chan = -DLISTENER -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker2x6Chan_Listener1x4Chan += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=6 
XCC_FLAGS_Talker2x6Chan_Listener1x4Chan += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=4 
XCC_FLAGS_Talker2x6Chan_Listener1x4Chan += $(LISTENER_DEBUG_FLAGS) $(TALKER_DEBUG_FLAGS)

XCC_FLAGS_Talker1x4Chan_Listener2x6Chan = -DLISTENER -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker1x4Chan_Listener2x6Chan += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=4 
XCC_FLAGS_Talker1x4Chan_Listener2x6Chan += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=6 
XCC_FLAGS_Talker1x4Chan_Listener2x6Chan += $(LISTENER_DEBUG_FLAGS) $(TALKER_DEBUG_FLAGS)
# End: Asymmetric pair of configs

# Pair of Assymetric configs
XCC_FLAGS_Talker2x7Chan_Listener1x2Chan = -DLISTENER -DTALKER $(XCC_FLAGS) 
XCC_FLAGS_Talker2x7Chan_Listener1x2Chan += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=7
XCC_FLAGS_Talker2x7Chan_Listener1x2Chan += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=2 
XCC_FLAGS_Talker2x7Chan_Listener1x2Chan += $(LISTENER_DEBUG_FLAGS) $(TALKER_DEBUG_FLAGS)

XCC_FLAGS_Talker1x2Chan_Listener2x7Chan = -DLISTENER -DTALKER $(XCC_FLAGS) 
XCC_FLAGS_Talker1x2Chan_Listener2x7Chan += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=2 
XCC_FLAGS_Talker1x2Chan_Listener2x7Chan += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=7
XCC_FLAGS_Talker1x2Chan_Listener2x7Chan += $(LISTENER_DEBUG_FLAGS) $(TALKER_DEBUG_FLAGS)
# End: Asymmetric pair of configs


XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=8 
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=8 
#XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM += -DCHECK_HW_LOOPBACK

#XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM_test = $(XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM)
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM_test += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=8 
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM_test += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=8 
XCC_FLAGS_Talker1x8Chan_Listener1x8Chan_TDM_test += -DGEN_TEST_SIGNAL #-DCHECK_TEST_SIGNAL 

XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=8 
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=8 
#XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM += -DCHECK_HW_LOOPBACK

XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM_test += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=8 
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM_test += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=8 
XCC_FLAGS_Talker2x8Chan_Listener2x8Chan_TDM_test += -DUSE_LOCAL_CLOCK -DGEN_TEST_SIGNAL #-DCHECK_TEST_SIGNAL

XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=4 
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM += -DAVB_NUM_SINKS=4 -DAVB_CHANNELS_PER_SINK=4 

XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM_test += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=4 
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM_test += -DAVB_NUM_SINKS=4 -DAVB_CHANNELS_PER_SINK=4 
XCC_FLAGS_Talker4x4Chan_Listener4x4Chan_TDM_test += -DUSE_LOCAL_CLOCK -DGEN_SIGNAL #-DCHECK_SIGNAL

XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=12 
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=12 
#XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM += -DCHECK_HW_LOOPBACK

XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM_test += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=12 
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM_test += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=12 
XCC_FLAGS_Talker2x12Chan_Listener2x12Chan_TDM_test += -DUSE_LOCAL_CLOCK -DGEN_TEST_SIGNAL #-DCHECK_TEST_SIGNAL

# Start: Asymmetric pair of configs
# For 3 Endpoints ! Talker2x8Chan_Listener2x16Chan_TDM running on one. _test on the other two.
XCC_FLAGS_Talker2x8Chan_Listener2x16Chan_TDM = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker2x8Chan_Listener2x16Chan_TDM += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x8Chan_Listener2x16Chan_TDM += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=8
XCC_FLAGS_Talker2x8Chan_Listener2x16Chan_TDM += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=16

XCC_FLAGS_Talker1x16Chan_Listener1x8Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker1x16Chan_Listener1x8Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker1x16Chan_Listener1x8Chan_TDM_test += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=16
XCC_FLAGS_Talker1x16Chan_Listener1x8Chan_TDM_test += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=8
XCC_FLAGS_Talker1x16Chan_Listener1x8Chan_TDM_test += -DUSE_LOCAL_CLOCK -DGEN_TEST_SIGNAL #-DCHECK_TEST_SIGNAL
# End: Asymmetric pair of configs

# Start: Asymmetric trio of configs
# For 3 Endpoints
XCC_FLAGS_Listener2x16Chan_TDM = -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Listener2x16Chan_TDM += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener2x16Chan_TDM += -DAVB_NUM_SINKS=2 -DAVB_CHANNELS_PER_SINK=16

XCC_FLAGS_Talker1x16Chan_Listener1x16Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker1x16Chan_Listener1x16Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker1x16Chan_Listener1x16Chan_TDM_test += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=16
XCC_FLAGS_Talker1x16Chan_Listener1x16Chan_TDM_test += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=16
XCC_FLAGS_Talker1x16Chan_Listener1x16Chan_TDM_test += -DGEN_TEST_SIGNAL #-DCHECK_TEST_SIGNAL

XCC_FLAGS_Talker1x16Chan_TDM_test = -DTALKER -DLISTENER $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_Talker1x16Chan_TDM_test += $(TALKER_DEBUG_FLAGS) $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Talker1x16Chan_TDM_test += -DAVB_NUM_SOURCES=1 -DAVB_CHANNELS_PER_SOURCE=16
XCC_FLAGS_Talker1x16Chan_TDM_test += -DAVB_NUM_SINKS=1 -DAVB_CHANNELS_PER_SINK=8
XCC_FLAGS_Talker1x16Chan_TDM_test += -DUSE_LOCAL_CLOCK -DGEN_TEST_SIGNAL #-DCHECK_TEST_SIGNAL
# End: Asymmetric trio of configs


# Configuration Specific Messages
ifeq ($(CONFIG),Talker4x2Chan_Listener4x2Chan) 
   $(warning WARNING: Configuration Talker4x2Chan_Listener4x2Chan currently throws Exception)
endif

XCC_FLAGS_xr_avb_demo.xc = $(XCC_FLAGS) -fsubword-select

# The USED_MODULES variable lists other module used by the application. 

USED_MODULES = module_avb_1722 module_avb_1722_maap module_avb_srp module_osc module_avb_media_clock  module_avb_audio
USED_MODULES += module_avb module_avb_util  module_gptp module_ethernet module_locks module_i2c 
ifeq ($(XSCOPE), 0)
USED_MODULES += module_xlog
endif

XCC_FLAGS_xdk_demo_audio_if.xc = $(XCC_FLAGS) -fsubword-select
XCC_FLAGS_xlog_server_support.c = -O0 -g

# This uses the avb mac filtering in module_avb_common
USE_AVB_MAC_CUSTOM_FILTER=1

#Add codecs from module_avb_audio
USED_CODECS = 

#Add audio interfaces from module_avb_audio
USED_AUDIO_INTERFACES = $(AUDIO_INTERFACE)


#Add external clocking chips/plls from module_avb_audio
USED_EXTERNAL_CLOCK_CHIPS = CS2300CP

USE_LARGE_1722_ROUTER_TABLE = 1

#=============================================================================
#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
ifneq ($(wildcard $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common),)
include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
else
include ../module_xcommon/build/Makefile.common
endif


