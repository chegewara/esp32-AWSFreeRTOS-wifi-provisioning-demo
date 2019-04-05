#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := aws_demos

ifndef AMAZON_FREERTOS_PATH
export AMAZON_FREERTOS_PATH := $(CURDIR)/../../../..
endif

ifndef IDF_PATH
export IDF_PATH := $(AMAZON_FREERTOS_PATH)/lib/third_party/mcu_vendor/espressif/esp-idf
# else
# $(error ERROR: IDF_PATH is defined in your environement, it will not point to lib/third_party/mcu_vendor/espressif/esp-idf. To remove the variable run the command "unset IDF_PATH")
endif

EXTRA_COMPONENT_DIRS := $(AMAZON_FREERTOS_PATH)/demos/espressif/esp32_devkitc_esp_wrover_kit/common/application_code/espressif_code

include $(IDF_PATH)/make/project.mk



app: versioned_binary

# include $(IDF_PATH)/make/project.mk

versioned_binary: $(BUILD_DIR_BASE)/$(PROJECT_NAME).bin
	rm -vf $(BUILD_DIR_BASE)/$(PROJECT_NAME)-*.*.bin
	cp -v $(BUILD_DIR_BASE)/$(PROJECT_NAME).bin $(BUILD_DIR_BASE)/$(PROJECT_NAME)-`./version.sh`.bin
