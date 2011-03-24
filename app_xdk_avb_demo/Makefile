# The TARGET variable determines what target system the application is 
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

TARGET = XDK

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

XCC_FLAGS = -O3 -save-temps -g -report -DAVB1722_ROUTER -DI2C_TI_COMPATIBILITY -DI2C_BIT_TIME=2000 -Xmapper --map -Xmapper MAPFILE

# The USED_MODULES variable lists other module used by the application. 

USED_MODULES = module_xdk_avb_common module_avb_1722 module_avb_1722_maap module_avb_srp  module_avb_audio  module_avb_media_clock module_avb module_avb_util  module_gptp module_osc module_xtcp module_zeroconf module_ethernet module_locks module_xlog module_i2c


XCC_FLAGS_xdk_demo_standalone.xc = $(XCC_FLAGS) -fsubword-select

# This uses the avb mac filtering in module_avb_common
USE_AVB_MAC_CUSTOM_FILTER=1

#Add codecs from module_avb_audio
USED_CODECS = TI_TLV320AIC23B

EXCLUDE_FILES += audio_clock_CS2300CP.xc

#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.


include ../../xcommon/module_xcommon/build/Makefile.common

# The final target of the build. This is usually dependent on a binary file
# in the $(BIN_DIR) directory e.g.
#
#  all : $(BIN_DIR)/my_app.xe

all: $(BIN_DIR)/$(APP_NAME).xe
	@echo Build Complete


clean: clean_common

