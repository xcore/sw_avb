# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = XR-AVB-LC-BRD

# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to 
# the project name

APP_NAME =

# This variable controls where the include files for the app are found.
# In this application the extra include files may be found in a target
# specific directory for XMOS development boards

INCLUDE_DIRS = src xmos_board_info/$(TARGET)

# This header file is marked as optional since it is only included for
# a known XMOS development board
OPTIONAL_HEADERS = avb_app_board_config.h

# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
#
#    XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
#
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to 
# xcc for the final link (mapping) stage.

XCC_FLAGS = -O2 -save-temps -g -report -Xmapper --map -Xmapper MAPFILE

XCC_FLAGS_main.xc = $(XCC_FLAGS) -falways-inline

# The USED_MODULES variable lists other module used by the application. 

USED_MODULES = module_avb_1722 module_avb_1722_maap module_avb_srp module_avb_media_clock module_avb_audio module_random
USED_MODULES += module_avb module_avb_util module_gptp module_ethernet module_ethernet_board_support module_avb_1722_1

ifeq ($(TARGET),$(filter $(TARGET),XR-AVB-LC-BRD D4U-AVB-DG SLICEKIT-L2))
    USED_MODULES += module_i2c_simple
endif

ifeq ($(TARGET),$(filter $(TARGET),XR-AVB-LC-BRD SLICEKIT-L2))
    XCC_FLAGS += -fxscope
endif

ifeq ($(TARGET),SLICEKIT-L2)
    USED_MODULES += module_slicekit_support
endif

#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
-include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
