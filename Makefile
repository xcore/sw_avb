# This Makefile acts as a composite builder for all the elements 
# of this repository.

# It has target patterns for all, clean and test for sub-directories of the 
# form dir.target e.g. calling:
#
# xmake app_uart_demo.all 
#
# will execute 'xmake all' in the app_uart_demo sub-directory.
#
# In addition the following targets are defined:
#
# all:
#
#    This target will build all the applications listed in the BUILD_SUBDIRS
#    variable.
#
# plugins:
# 
#    This target will build all the plugins listed in the PLUGIN_SUBDIRS 
#    variable
#
# clean:
#    
#    This target will clean all the applications listed in the BUILD_SUBDIRS
#    variable.
#
# clean_plugins:
#
#    This target will clean all the plugins listed in the PLUGIN_SUBDIRS 
#    variable.
#
# test:
#
#   This target will make the test make target in all the directories
#   listed in TEST_SUBDIRS.
#  


# This variable should contain a space separated list of all
# the directories containing buildable applications (usually
# prefixed with the app_ prefix)
BUILD_SUBDIRS  = app_xr_avb_lc_demo app_xc2_avb_demo app_xdk_avb_demo app_xdk_xai_avb_demo 
BUILD_SUBDIRS += app_1722_1_listener app_1722_1_talker app_1722_1_device 
BUILD_SUBDIRS += app_two_channel_listener app_two_channel_talker

# This variable should contain a space separated list of all
# the directories containing buildable plugins (usually
# prefixed with the plugin_ prefix)
PLUGIN_SUBDIRS = plugin_template

# This variable should contain a space separated list of all
# the directories containing applications with a 'test' make target
TEST_SUBDIRS = test_api_listener test_api_talker

# Provided that the above variables are set you shouldn't need to modify
# the targets below here. 

%.all:
	cd $* && xmake all

%.clean:
	cd $* && xmake clean

%.test:
	cd $* && xmake test

all: $(foreach x, $(BUILD_SUBDIRS), $x.all) 
plugins: $(foreach x, $(PLUGIN_SUBDIRS), $x.all) 
clean: $(foreach x, $(BUILD_SUBDIRS), $x.clean)
clean_plugins: $(foreach x, $(PLUGIN_SUBDIRS), $x.clean) 
test: $(foreach x, $(TEST_SUBDIRS), $x.test)
