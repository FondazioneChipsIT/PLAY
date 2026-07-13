# -----------------------------------------------------------------------------
# VARIABLEs
# -----------------------------------------------------------------------------
CURRENT_DIR := $(shell pwd)

SRC_DIR_REL =
INC_DIR_REL = ../../include

SRC_DIR = $(abspath $(CURRENT_DIR)/$(SRC_DIR_REL))
INC_DIR = $(abspath $(CURRENT_DIR)/$(INC_DIR_REL))

APP = vlsu-test

# Self-contained micro-test: only spatz/main.c is compiled (no source kernel,
# no test/common). Collect any top-level .c (currently none) for symmetry with
# the other tests.
SRC_SRCS := $(wildcard $(SRC_DIR)/*.c)
SRC_SRCS := $(filter-out $(SRC_DIR)/arch/%,$(SRC_SRCS))
APP_SRCS_REL := $(SRC_SRCS)
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

APP_CFLAGS += -O3 -g -I$(INC_DIR)

# -----------------------------------------------------------------------------
# TARGETs
# -----------------------------------------------------------------------------
ifeq ($(TARGET), SPATZ)
include spatz/Makefile
else
$(error Invalid TARGET: $(TARGET). This Makefile supports TARGET=SPATZ only.)
endif
