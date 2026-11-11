# -----------------------------------------------------------------------------
# VARIABLEs
# -----------------------------------------------------------------------------
CURRENT_DIR := $(shell pwd)

SRC_DIR_REL = ../../source/matrix_memcpy/
INC_DIR_REL = ../../include
DATA_DIR_REL = test_data
COMMON_DIR_REL = ../common

SRC_DIR = $(abspath $(CURRENT_DIR)/$(SRC_DIR_REL))
INC_DIR = $(abspath $(CURRENT_DIR)/$(INC_DIR_REL))
DATA_DIR = $(abspath $(CURRENT_DIR)/$(DATA_DIR_REL))
COMMON_DIR = $(abspath $(CURRENT_DIR)/$(COMMON_DIR_REL))

APP = matrix-memcpy-test
include ../common/arch_select.mk

# Collect only top-level SRC_DIR .c files (exclude arch/ subdir explicitly)
SRC_SRCS := $(wildcard $(SRC_DIR)/*.c)
SRC_SRCS := $(filter-out $(SRC_DIR)/arch/%,$(SRC_SRCS))
APP_SRCS_REL := $(SRC_SRCS) $(wildcard $(COMMON_DIR)/*.c)
APP_SRCS := $(foreach file, $(APP_SRCS_REL), $(abspath $(file)))

# Append the appropriate arch implementation
$(call add_arch_impl,matrix_memcpy)

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
