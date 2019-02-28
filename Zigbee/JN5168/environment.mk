
##############################################################################
# Application specific include dirs
INCFLAGS += -I$(APP_BASE)/../../utils  
INCFLAGS += -I$(APP_BASE)/../../AppQueueApi/Include
INCFLAGS += -I$(APP_BASE)/../../../..
INCFLAGS += -I$(APP_BASE)/../../../../common
INCFLAGS += -I$(APP_BASE)/../../../../common/2g
INCFLAGS += -I$(SDK_BASE_DIR)/Components/OAD/Include

##############################################################################
# Application libraries
# Specify additional Component libraries

ifeq ($(NEED_TOF), TRUE)
APPLIBS += TOF
endif

ifeq ($(NEED_OAD), TRUE)
APPLIBS += OAD
OAD_APP = $(SDK_BASE_DIR)/Tools/OAD
OAD_BIN = ../../../../utils
endif

##############################################################################
# Additional Application Source directories
# Define any additional application directories outside the application directory
# e.g. for AppQueueApi

ADDITIONAL_SRC_DIR += $(APP_BASE)/../../utils
ADDITIONAL_SRC_DIR += $(APP_BASE)/../../AppQueueApi/Source
ADDITIONAL_SRC_DIR += $(APP_BASE)/../../../../common
ADDITIONAL_SRC_DIR += $(APP_BASE)/../../../../common/2g

##############################################################################
# Application utilitie files

APPSRC += AppQueueApi.c
APPSRC += mac_util.c
APPSRC += event_util.c
APPSRC += system_util.c
APPSRC += timer_util.c
APPSRC += sleep_util.c
APPSRC += spi_util.c
APPSRC += led_util.c
APPSRC += error_util.c
APPSRC += mem_util.c
APPSRC += string_util.c
APPSRC += crc.c
//APPSRC += bsmac.c
//APPSRC += uart.c


ifeq ($(NEED_TOF), TRUE)
APPSRC += tof_util.c
endif 

APPSRC += i2c_printf_util.c
APPSRC += printf_util.c

##############################################################################
# Build options
ifeq ($(NEED_TOF), TRUE)
CFLAGS += -DNEED_TOF
endif

ifeq ($(NEED_I2C_PRINT), TRUE)
CFLAGS += -DNEED_I2C_PRINT
endif 

# for MicorSpecific.h
CFLAGS += -DJENNIC_CHIP_JN5168 -DEMBEDDED -DUSER_VSR_HANDLER
