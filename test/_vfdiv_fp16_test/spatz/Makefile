# -----------------------------------------------------------------------------
# CHECKs
# -----------------------------------------------------------------------------
PLATFORM ?= VLT

ifeq ($(PLATFORM), GVSOC)

ifeq ($(GVSOC_PATH),)
$(error GVSOC_PATH is not set)
endif
RUNNER_CMD = $(GVSOC_PATH) --target spatz --binary $(APP) run

else ifeq ($(PLATFORM), VLT)

ifeq ($(VLT_PATH),)
$(error VLT_PATH is not set)
endif
RUNNER_CMD = $(VLT_PATH) $(APP)

else ifeq ($(PLATFORM), VSIM)

ifeq ($(VSIM_PATH),)
$(error VSIM_PATH is not set)
endif
RUNNER_CMD = $(VSIM_PATH) $(APP)

else

$(error PLATFORM not supported)

endif

ifeq ($(LLVM_PATH),)
$(error LLVM_PATH is not set)
endif

ifeq ($(GCC_PATH),)
$(error GCC_PATH is not set)
endif

ifeq ($(SPATZ_SW_DIR),)
$(error SPATZ_SW_PATH is not set)
endif

# -----------------------------------------------------------------------------
# FLAGs
# -----------------------------------------------------------------------------
NUM_CC ?= 1	# Default

ifeq ($(shell [ $(NUM_CC) -ge 2 ] && echo true),true)
  APP_CFLAGS += -DNUM_CC=2
else
  APP_CFLAGS += -DNUM_CC=$(NUM_CC)
endif

PRECISION ?= SINGLE
ifeq ($(PRECISION), DOUBLE)
APP_CFLAGS += -DDOUBLE_PRECISION
endif


# -----------------------------------------------------------------------------
# VARIABLEs
# -----------------------------------------------------------------------------
MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(dir $(MKFILE_PATH))

APP_SRCS += $(MKFILE_DIR)/main.c

empty :=
space := $(empty) $(empty)
CMAKE_APP_SRCS := $(subst $(space),;,$(strip $(APP_SRCS)))
CMAKE_APP_CFLAGS := $(subst $(space),;,$(strip $(APP_CFLAGS)))
SPATZ_WORK_DIR := $(MKFILE_DIR)/
SPATZ_BUILD_DIR := $(MKFILE_DIR)/BUILD/

# -----------------------------------------------------------------------------
# RULEs
# -----------------------------------------------------------------------------

CMAKE ?= cmake

CMAKE_CONFIG_COMMAND := $(CMAKE)\
	-S $(SPATZ_WORK_DIR) -B $(SPATZ_BUILD_DIR) \
	-DLLVM_PATH=$(LLVM_PATH) \
	-DGCC_PATH=/$(GCC_PATH) \
	-DAPP=$(APP) \
	-DSPATZ_SW_DIR=$(SPATZ_SW_DIR) \
	-DPRJ_SRCS="$(CMAKE_APP_SRCS)" \
	-DAPP_CFLAGS="$(CMAKE_APP_CFLAGS)" \
	-DPYTHON=python3 \
	-DSPATZ_CLUSTER_CFG=spatz_cluster.default.dram.hjson \
	-DMEM_DRAM_ORIGIN=2147483648 \
	-DMEM_DRAM_SIZE=2147483648 \
	-DSNRT_BASE_HARTID=0 \
	-DSNRT_CLUSTER_CORE_NUM=$(NUM_CC) \
	-DSNRT_TCDM_START_ADDR=1048576 \
	-DSNRT_CLUSTER_OFFSET=0 \
	-DSNRT_TCDM_SIZE=131072 \
	-DSNRT_NFPU_PER_CORE=4

.PHONY: all config build run clean

all: build

$(SPATZ_BUILD_DIR):
	mkdir -p $(SPATZ_BUILD_DIR)

config: $(SPATZ_BUILD_DIR)
	$(CMAKE_CONFIG_COMMAND)

build: config
	$(MAKE) -C $(SPATZ_BUILD_DIR)

run: build
	cd $(SPATZ_BUILD_DIR) && $(RUNNER_CMD)

clean:
	rm -rf $(SPATZ_BUILD_DIR)
