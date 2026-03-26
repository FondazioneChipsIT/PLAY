cmake_minimum_required(VERSION 3.13)

set(SNITCH_SOFTWARE_DIR ${SPATZ_SW_DIR})
list(APPEND CMAKE_MODULE_PATH ${SNITCH_SOFTWARE_DIR}/cmake)
set(CMAKE_TOOLCHAIN_FILE toolchain-llvm)
set(SNITCH_RUNTIME snRuntime-cluster CACHE STRING "")
set(SNRT_NFPU_PER_CORE "0" CACHE STRING "Number of FPUs per Spatz")

project(${APP} LANGUAGES C ASM)
include(SnitchUtilities)

add_subdirectory(${SNITCH_SOFTWARE_DIR}/snRuntime snRuntime)
include_directories(${SNRUNTIME_INCLUDE_DIRS})

add_compile_options(-O3 -g -ffunction-sections -ffast-math)

if(DEFINED APP_CFLAGS)
    add_compile_options(${APP_CFLAGS})
endif()

add_executable(${APP} ${PRJ_SRCS})
include_directories(../)
include_directories(../../../include/)
include_directories(../../common)
target_link_libraries(${APP} ${SNITCH_RUNTIME})
target_link_options(${APP} PRIVATE "SHELL:-T ${LINKER_SCRIPT}")
target_compile_definitions(${APP} PUBLIC SNRT_NFPU_PER_CORE=${SNRT_NFPU_PER_CORE})
target_compile_definitions(${APP} PRIVATE TARGET_IS_SPATZ)
