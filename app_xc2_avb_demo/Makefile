# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = XC-2

# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to 
# the project name

APP_NAME =

# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
#
#    XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
#
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to 
# xcc for the final link (mapping) stage.

XCC_FLAGS = -O3 -save-temps -g -report -lflash

XCC_FLAGS_xc2_demo.xc = $(XCC_FLAGS) -fsubword-select

# The USED_MODULES variable lists other module used by the application. 

USED_MODULES = module_avb_1722 module_avb_1722_maap module_avb_srp  module_avb_audio  module_avb_media_clock module_avb module_avb_util module_gptp module_osc module_xtcp module_zeroconf module_ethernet module_locks module_xlog module_i2c module_dfu module_tftp

# This uses the avb mac filtering in module_avb_common
USE_AVB_MAC_CUSTOM_FILTER=1

#Add audio interfaces from module_avb_audio
USED_AUDIO_INTERFACES = synth

#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
-include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
