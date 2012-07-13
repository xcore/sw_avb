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
XSCOPE = 1

LISTENER_DEBUG_FLAGS = -DAVB_1722_RECORD_ERRORS 

ifeq ($(XSCOPE),1)
LISTENER_DEBUG_FLAGS += -lxscope -DUSE_XSCOPE -DUSE_XSCOPE_PROBES
TALKER_DEBUG_FLAGS += -lxscope -DUSE_XSCOPE -DUSE_XSCOPE_PROBES 
endif

# common flags
XCC_FLAGS = -O3 -save-temps -g -report -Xmapper --map -Xmapper MAPFILE
XCC_FLAGS += -DAVB_1722_RECORD_ERRORS 
#XCC_FLAGS += -fllvm
#XCC_FLAGS = -lxscope

# Config specific flags
XCC_FLAGS_Talker = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=1
#XCC_FLAGS_Talker += -lxscope

###########################################################################
#############################  Configurations #############################
###########################################################################

#----------------------------- Talker Only -------------------------------#
XCC_FLAGS_Talker2x4Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker2x4Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker2x4Chan += -DAVB_NUM_SOURCES=2 -DAVB_CHANNELS_PER_SOURCE=4

XCC_FLAGS_Talker4x2Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x2Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2

XCC_FLAGS_Talker4x2Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x2Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2

XCC_FLAGS_Talker4x2ChanSAF = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x2ChanSAF += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x2ChanSAF += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=2 -DAVB_1722_FORMAT_SAF

XCC_FLAGS_Talker4x3Chan = -DTALKER $(XCC_FLAGS)
XCC_FLAGS_Talker4x3Chan += $(TALKER_DEBUG_FLAGS)
XCC_FLAGS_Talker4x3Chan += -DAVB_NUM_SOURCES=4 -DAVB_CHANNELS_PER_SOURCE=3

#----------------------------- Listener Only -------------------------------#
XCC_FLAGS_Listener = -DLISTENER $(XCC_FLAGS)
XCC_FLAGS_Listener += $(LISTENER_DEBUG_FLAGS)
XCC_FLAGS_Listener += -DAVB_NUM_SINKS=4 -DAVB_CHANNELS_PER_SINK=1

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

# This uses the avb mac filtering in module_avb_common
USE_AVB_MAC_CUSTOM_FILTER=1

#Add codecs from module_avb_audio
USED_CODECS = CS42448

#Add audio interfaces from module_avb_audio
USED_AUDIO_INTERFACES = i2s

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


