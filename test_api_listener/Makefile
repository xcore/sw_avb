# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = XR-AVB-LC-BRD

# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to 
# the project name

APP_NAME =

# global debug switches
#
XSCOPE = 1


# Settings derived from configuration names
#

# if config name contains TDM, use the TDM multi interface
ifneq (,$(findstring TDM,$(CONFIG))) 
AUDIO_INTERFACE = tdm_multi  # one of i2s, tdm, tdm_multi
endif

# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
#
#    XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
#
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to 
# xcc for the final link (mapping) stage.


###########################################################################
# Settings derived from configuration names
###########################################################################

# if config name contains TDM, use the TDM multi interface
ifneq (,$(findstring TDM,$(CONFIG))) 
AUDIO_INTERFACE = tdm_multi  # one of i2s, tdm, tdm_multi
endif

###########################################################################
# Default settings
###########################################################################

ifndef AUDIO_INTERFACE
AUDIO_INTERFACE = i2s
endif


###########################################################################
# Common Flags
###########################################################################

XCC_FLAGS = -O3 -save-temps -g -report -Xmapper --map -Xmapper MAPFILE -lxscope
XCC_FLAGS += -DAVB_AUDIO_IF_$(AUDIO_INTERFACE)
#debug
XCC_FLAGS += -DAVB_1722_RECORD_ERRORS
#XCC_FLAGS += -fllvm

XCC_FLAGS_xr_avb_demo.xc = $(XCC_FLAGS) -fsubword-select


###########################################################################
#############################  Configurations #############################
###########################################################################
# Configuration naming convention:
# $Xl$Ys$Zch[_$if]
# $Xl: 	number of listeners
# $Ys: 	number of streams
# $Zch: number of channels
# $if:	optional: audio interface other than i2s
XCC_FLAGS_1l2s16ch = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_1l2s16ch += -DAVB_NUM_LISTENER_UNITS=1 -DAVB_NUM_SINKS=2 -DAVB_NUM_MEDIA_OUTPUTS=16

XCC_FLAGS_1l1s16ch = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_1l1s16ch += -DAVB_NUM_LISTENER_UNITS=1 -DAVB_NUM_SINKS=1 -DAVB_NUM_MEDIA_OUTPUTS=16

XCC_FLAGS_2l2s16ch = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_2l2s16ch += -DAVB_NUM_LISTENER_UNITS=2 -DAVB_NUM_SINKS=2 -DAVB_NUM_MEDIA_OUTPUTS=16

XCC_FLAGS_2l2s16ch_TDM = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_2l2s16ch_TDM += -DAVB_NUM_LISTENER_UNITS=2 -DAVB_NUM_SINKS=2 -DAVB_NUM_MEDIA_OUTPUTS=16

XCC_FLAGS_2l2s20ch_TDM = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_2l2s20ch_TDM += -DAVB_NUM_LISTENER_UNITS=2 -DAVB_NUM_SINKS=2 -DAVB_NUM_MEDIA_OUTPUTS=20

#increase size of High Priority Queue above 20 channels
XCC_FLAGS_2l2s24ch_TDM = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_2l2s24ch_TDM += -DAVB_NUM_LISTENER_UNITS=2 -DAVB_NUM_SINKS=2 -DAVB_NUM_MEDIA_OUTPUTS=24
XCC_FLAGS_2l2s24ch_TDM += -DMII_RX_BUFSIZE_HIGH_PRIORITY=1400

XCC_FLAGS_2l2s32ch_TDM = $(XCC_FLAGS) -DAVB_MAX_AUDIO_SAMPLE_RATE=48000
XCC_FLAGS_2l2s32ch_TDM += -DAVB_NUM_LISTENER_UNITS=2 -DAVB_NUM_SINKS=2 -DAVB_NUM_MEDIA_OUTPUTS=32
XCC_FLAGS_2l2s32ch_TDM += -DMII_RX_BUFSIZE_HIGH_PRIORITY=1400


###########################################################################
# Modules
###########################################################################
# The USED_MODULES variable lists other module used by the application. 
USED_MODULES = module_avb_1722 module_avb_1722_maap module_avb_srp module_osc module_avb_media_clock  module_avb_audio
USED_MODULES += module_avb module_avb_util  module_gptp module_ethernet module_locks module_i2c

XCC_FLAGS_xdk_demo_audio_if.xc = $(XCC_FLAGS) -fsubword-select

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
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
-include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common

