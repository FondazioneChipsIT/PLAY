# -----------------------------------------------------------------------------
# VARIABLEs
# -----------------------------------------------------------------------------
CURRENT_DIR := $(shell pwd)

SRC_DIR_REL = ../../source/linalg_svd_lsv/
INC_DIR_REL = ../../include
DATA_DIR_REL = test_data
COMMON_DIR_REL = ../common

SRC_DIR = $(foreach dir, $(SRC_DIR_REL), $(abspath $(CURRENT_DIR)/$(dir)))
INC_DIR = $(abspath $(CURRENT_DIR)/$(INC_DIR_REL))
DATA_DIR = $(abspath $(CURRENT_DIR)/$(DATA_DIR_REL))
COMMON_DIR = $(abspath $(CURRENT_DIR)/$(COMMON_DIR_REL))

APP = linalg-svd-lsv-test
include ../common/arch_select.mk

# Collect only top-level SRC_DIR .c files (exclude arch/ subdir explicitly)
SRC_SRCS := $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))
SRC_SRCS := $(filter-out %/arch/%,$(SRC_SRCS))
APP_SRCS := $(SRC_SRCS) $(wildcard $(COMMON_DIR)/*.c)

# Append the appropriate arch implementation
$(call add_arch_impl,linalg_svd_lsv)

# -----------------------------------------------------------------------------
# FLAGs
# -----------------------------------------------------------------------------
ifdef PRINT_DATA
APP_CFLAGS += -DPRINT_DATA
endif

ifdef STATS
APP_CFLAGS += -DSTATS
endif

ifdef PACKED
APP_CFLAGS += -DPLAY_PACKED
endif

ifdef ENABLE_LOGGING
APP_CFLAGS += -DENABLE_LOGGING
endif

# -----------------------------------------------------------------------------
# TARGETs
# -----------------------------------------------------------------------------
ifeq ($(TARGET), PULP_OPEN)
include pulp-open/Makefile
else ifeq ($(TARGET), SPATZ)
include spatz/Makefile
else
$(error Invalid TARGET: $(TARGET))
endif
