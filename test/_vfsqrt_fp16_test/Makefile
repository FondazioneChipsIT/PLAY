# -----------------------------------------------------------------------------
# VARIABLEs
# -----------------------------------------------------------------------------
CURRENT_DIR := $(shell pwd)

SRC_DIR_REL =
INC_DIR_REL = ../../include
DATA_DIR_REL = test_data
COMMON_DIR_REL = ../common

SRC_DIR = $(abspath $(CURRENT_DIR)/$(SRC_DIR_REL))
INC_DIR = $(abspath $(CURRENT_DIR)/$(INC_DIR_REL))
DATA_DIR = $(abspath $(CURRENT_DIR)/$(DATA_DIR_REL))
COMMON_DIR = $(abspath $(CURRENT_DIR)/$(COMMON_DIR_REL))

APP = vfsqrt-fp16-test

SRC_SRCS := $(wildcard $(SRC_DIR)/*.c)
SRC_SRCS := $(filter-out $(SRC_DIR)/arch/%,$(SRC_SRCS))
APP_SRCS_REL := $(SRC_SRCS) $(wildcard $(COMMON_DIR)/*.c)
APP_SRCS := $(foreach file, $(APP_SRCS_REL), $(abspath $(file)))

# -----------------------------------------------------------------------------
# FLAGs
# -----------------------------------------------------------------------------
ifdef PRINT_DATA
APP_CFLAGS += -DPRINT_DATA
endif

ifdef STATS
APP_CFLAGS += -DSTATS
endif

ifdef ENABLE_LOGGING
APP_CFLAGS += -DENABLE_LOGGING
endif

APP_CFLAGS += -O3 -g -I$(INC_DIR) -I$(DATA_DIR) -I$(COMMON_DIR)
APP_LDFLAGS += -O3 -g

# -----------------------------------------------------------------------------
# TARGETs
# -----------------------------------------------------------------------------
ifeq ($(TARGET), SPATZ)
include spatz/Makefile
else
$(error Invalid TARGET: $(TARGET). This Makefile supports TARGET=SPATZ only.)
endif
