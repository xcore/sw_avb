# The following works around a bug in the 11.11.0 toolchain
# For later versions of the toolchain it should not be needed

ALL_GENERATED_FILES := $(ALL_GENERATED_FILES) $(foreach x,$(GENERATED_FILES),$(GEN_DIR)/$x)
GEN_FILES_MODULE_DIRS := $(GEN_FILES_MODULE_DIRS) $(foreach x,$(GENERATED_FILES),$(GEN_DIR)/$x*****$(strip $(CURRENT_MODULE_DIR)))
GEN_DIRS := $(GEN_DIRS) $(GEN_DIR)
GENERATED_FILES :=

ifeq ($(OS),WINDOWS)
JYTHON = (echo source $1>$(TARGET_DIR)/tmp.xta) & (echo exit >> $(TARGET_DIR)/tmp.xta) & (xta source $(TARGET_DIR)/tmp.xta)
else
JYTHON = echo "source $1">$(TARGET_DIR)/tmp.xta;echo "exit" >> $(TARGET_DIR)/tmp.xta;xta source $(TARGET_DIR)/tmp.xta
endif