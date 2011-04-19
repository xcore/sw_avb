# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = XR-AVB-LC-BRD

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

XCC_FLAGS = -O3 -save-temps -g -report -Xmapper --map -Xmapper MAPFILE

XCC_FLAGS_xr_avb_demo.xc = $(XCC_FLAGS) -fsubword-select

# The USED_MODULES variable lists other module used by the application. 

USED_MODULES = module_avb_1722 module_avb_1722_maap module_avb_srp module_osc module_avb_media_clock  module_avb_audio  module_avb module_avb_util  module_gptp module_ethernet module_locks module_i2c module_xlog

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
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.


include ../../xcommon/module_xcommon/build/Makefile.common

# The final target of the build. This is usually dependent on a binary file
# in the $(BIN_DIR) directory e.g.
#
#  all : $(BIN_DIR)/my_app.xe

test: $(BIN_DIR)/$(APP_NAME).xe
	@echo Build Complete


clean: clean_common
all: test


