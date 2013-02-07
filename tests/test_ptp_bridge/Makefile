# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = SLICEKIT-L2

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

XCC_FLAGS = -O2 -save-temps -g -report -Xmapper --map -Xmapper MAPFILE -fxscope

# -fno-xta-info

XCC_FLAGS_main.xc = $(XCC_FLAGS) -falways-inline

# The USED_MODULES variable lists other module used by the application. 
USED_MODULES = module_avb_util module_gptp module_ethernet module_ethernet_board_support module_slicekit_support module_otp_board_info

# This change to the module path is so that this application can be in the
# tests sub-directory in it's git repo
ifeq ($(notdir $(abspath ..)),tests)
PATHSEP = $(if $(findstring Windows, $(OS))$(findstring WINDOWS,$(OS)),;,:)
XMOS_MODULE_PATH := $(XMOS_MODULE_PATH)$(PATHSEP)../../..
endif


#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
-include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
