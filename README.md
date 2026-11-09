
# PLAY — Pulp Linear Algebra librarY

A collection of linear algebra kernels optimized for PULP-based architectures. The project aims to be portable, testable, and easy to extend across architectures. Currently PLAY supports two PULP architectures: Pulp Open and Spatz.

## Table of Contents

- [Repository Structure](#repository-structure)
    - [Directory Layout](#directory-layout)
    - [Multi Architecture Strategy](#multi-architecture-strategy)
    - [Data Formats](#data-formats)

- [Test](#test)
    - [Environment Setup](#environment-setup)
        - [Pulp Open](#pulp-open)
            - [On GVSoC](#pulp-open-on-gvsoc)
            - [On QuestaSim](#pulp-open-on-questasim)
        - [Spatz](#spatz)
            - [On GVSoC](#spatz-on-gvsoc)
            - [On Verilator](#spatz-on-verilator)
            - [On QuestaSim](#spatz-on-questasim)
    - [Build Time Architecture Selection](#build-time-architecture-selection)
    - [Run Test](#run-test)
    - [Generating Test Data](#generating-test-data)

- [How to Use PLAY in your own project](#how-to-use-play-in-your-own-project)

- [How to Add a New Target Architecture](#how-to-add-a-new-target-architecture)


## Repository Structure

### Directory Layout
- `include/` — Contains the single public header, `play.h`, which exposes the library APIs.
- `source/` — Contains the implementation of the linear algebra kernels. Each subdirectory corresponds to a specific kernel implementation and each kernel subdirectory has a top-level `<kernel>.c` and an `arch/` folder with per-arch sources.
- `test/` — Contains test cases for each kernel for each supported architecture. Each kernel under `test/<kernel>/` includes test code, per-target wrappers, test data and test data generation scripts.

### Multi Architecture Strategy
This project uses a consistent pattern to support multiple architectures hiding specific implementation details to the user.

For each kernel:
- There is a _public dispatcher_, placed in `source/<kernel>/<kernel.c>`, and forwards to a _single unified symble_ named `<kernel>_impl()` (for example `matrix_mul_impl()`).
- Each architecture implements its own version of the kernel in `source/<kernel>/arch/<kernel>_<arch>.c` (for example `matrix_mul_pulp_open.c`, `matrix_mul_spatz.c`).
- To avoid duplicating small per-kernel header files, this repository uses a centralized internal header `include/internal/arch_interface.h`, which collects the prototypes for all unified per-kernel implementation symbols (for example `vector_mul_impl`, `vector_sub_impl`, ...).
    -  Dispatcher files (`source/<kernel>/<kernel>.c`) and arch implementations (`source/<kernel>/arch/<kernel>_<arch>.c`) include this centralized header.

This means the public API and the test-level code do not need to know architecture internals; the build picks the correct arch implementation which provides the unified symbol. The user just call the API he is interested in, exposed and documented in the single header file  `play.h`.

### Data Formats
The kernels are written once on the `play_fmt_t` type, which `play.h` defines according to the `PLAY_FORMAT` macro selected at build time (`-DPLAY_FORMAT=<value>`):

| `PLAY_FORMAT` | `play_fmt_t` | Requirements |
|---|---|---|
| `PLAY_FMT_FP32` (default) | `float` | none |
| `PLAY_FMT_FP16` | `_Float16` | clang; `Zhinx` for the scalar operations, `Xcvhv` for the packed-SIMD ones |
| `PLAY_FMT_BF16` | `__bf16` | clang; `Xcvbf` for the scalar operations, `Xcvbfv` for the packed-SIMD ones |

With the default format the library is the plain `float` library it has always been. The narrow formats are supported on Pulp Open only: the Spatz kernels are written in assembly on 32-bit lanes and a format change would be a rewrite, not a retype. The `-march` carrying the required extensions is part of the toolchain environment, not of PLAY. The few math functions used by the kernels (`fabs`, `sqrt`) follow the format through `include/internal/fmt_math.h`, and the tests check the results against a tolerance made of an absolute and a relative term chosen per format (`TOLL_ABS` and `TOLL_REL` in `test/common/utils.h`, both overridable with `-D`).

The narrow formats can use the packed-SIMD instructions (two elements in one 32-bit register) in two ways. The compiler's autovectorizer works on the scalar kernels as they are (`VECTORIZE=1`, clang only). Some kernels also have an explicit packed version of their cluster implementation, written on the `play_fmt_v2_t` pair type and kept in the same source file under `#ifdef PLAY_PACKED`; it is built with `PACKED=1` (`-DPLAY_PACKED`, narrow formats only) and the fabric-controller implementation stays scalar.

## Test
Each kernel has an associated test case in the test directory. Each test subfolder contains a dedicated folder for the source code and compilation files for each supported architecture, along with a shared folder containing the test input data and a Python script for generating them. Each architecture implements its own  `main.c`, which loads the source data, calls the kernel under test with this data, and compares the calculated result with the expected one.

### Environment Setup
Environment setup is a prerequisite for test execution, varying according to the specific _target architecture_ and _simulation platform_

#### Pulp Open
To run a test targeting the Pulp Open architecture, it is necessary to install the RISCV GNU Toolchain and the pulp-sdk. Follow the instructions provided [here](https://github.com/pulp-platform/pulp-riscv-gnu-toolchain?tab=readme-ov-file#risc-v-gnu-compiler-toolchain) for the toolchain and [here](https://github.com/pulp-platform/pulp-sdk?tab=readme-ov-file#pulp-sdk) for the pulp-sdk.

##### Pulp Open on GVSoC
Once installed the toolchain and sdk, simply set the PULP_RISCV_GCC_TOOLCHAIN environment variable to your toolchain path and source the pulp-open configuration from the SDK. For example:

```sh
export PULP_RISCV_GCC_TOOLCHAIN="/opt/riscv"
source /home/sam/repos/pulp-sdk/configs/pulp-open.sh
```

**Note:** use the actual installation paths on your local machine.

##### Pulp Open on QuestaSim
To run a test targeting the Pulp Open architecture in RTL, it is necessary to build also the hardware first. Follow the instruction provided [here](https://github.com/FondazioneChipsIT/pulp_cluster).

#### Spatz
To run test targeting the Spatz architecture, it is necessary to build and install the Spatz repository. Follow the instruction provided [here](https://github.com/FondazioneChipsIT/spatz?tab=readme-ov-file#spatz). Note: This will install also the specific LLVM and GCC for Spatz. Once installed, set the following environment variables:

```sh
export LLVM_PATH="/opt/riscv/spatz-14-llvm"
export GCC_PATH="/home/sam/repos/spatz/install/riscv-gcc"
export SPATZ_SW_DIR="/home/sam/repos/spatz/sw"
```
**Note:** use the actual installation paths on your local machine.

##### Spatz on GVSoC
To run a test targeting the Spatz architecture on GVSoC, it is necessary set an additional environment variable.

```sh
export GVSOC_PATH="/home/sam/repos/gvsoc/install/bin/gvsoc"
```
**Note:** use the actual installation paths on your local machine.

##### Spatz on Verilator
To run a test targeting the Spatz architecture on Verilator, it is necessary set an additional environment variable.

```sh
export VLT_PATH="/home/sam/repos/spatz/hw/system/spatz_cluster/bin/spatz_cluster.vlt"
```
**Note:** use the actual installation paths on your local machine.

##### Spatz on QuestaSim
To run a test targeting the Spatz architecture on QuestaSim, it is necessary set an additional environment variable.

```sh
export VSIM_PATH="/home/sam/repos/spatz/hw/system/spatz_cluster/bin/spatz_cluster.vsim"
```
**Note:** use the actual installation paths on your local machine.

### Build Time Architecture Selection
For testing the architecture is selected via the `TARGET` make variable at compile time (e.g. `make TARGET=PULP_OPEN` or `make TARGET=SPATZ`). The top level test Makefile conditionally includes the arch specific Makefile; these target-specific Makefiles build sources and set compile-time flags (for example `APP_CFLAGS += -DTARGET_IS_PULP_OPEN`). Test sources are collected with a wildcard like `$(wildcard $(SRC_DIR)/<kernel>/*.c)` but are filtered to exclude any arch/ subdirectory so architecture implementations are not pulled in automatically. The `add_arch_impl` macro (defined in `test/common/arch_select.mk`) appends the architecture-specific implementation file `source/<kernel>/arch/<kernel>_$(ARCH_SUFFIX).c` to the list of source files to be compiled.

### Run test

For testing, several make flags are available:

- Common flags to all targets
    - `TARGET=<target>` - Specify the target architecture. Supported targets are `PULP_OPEN` and `SPATZ`
    - `STATS=1` — Collect and print performance counters
    - `ENABLE_LOGGING=1` — Enable logging
    - `PRINT_DATA=1` — Print inputs, computed and reference values

- Flags for `PULP_OPEN`
    - `USE_CLUSTER=1` — Run on the cluster (multi-core) instead of the fabric controller (default)
    - `NUM_CORES=<n>` — Number of cluster cores when `USE_CLUSTER=1` (default `NUM_CORES=1`, max value `NUM_CORES=8`)
    - `FORMAT=<format>` — Data format of the kernels and of the test data: `FP32` (default), `FP16` or `BF16` (see [Data Formats](#data-formats); the narrow formats need a clang toolchain whose `-march` carries the matching extensions)
    - `VECTORIZE=1` — Enable the packed-SIMD autovectorizer of the narrow formats (`-mno-strict-align -mllvm -riscv-xcvhv-vectorize`, clang only)
    - `PACKED=1` — Build the explicit packed-SIMD kernels where they exist (`-DPLAY_PACKED`, narrow formats only; see [Data Formats](#data-formats))

- Flags for `SPATZ`
    - `NUM_CC=<n>` - Number of Core-Complexes in Cluster (default `NUM_CC=1`, max value `NUM_CC=2`)
    - `PLATFORM=<platform`> - Emulator platform. Supported platforms are `GVSOC`, `VLT` and `VSIM` (default `VLT`)

To build and run a test, run:

```sh
make -C test/<kernel> clean all run TARGET=<target> PLATFORM=<platform> <FLAGS>
```

For example:
```sh
make -C test/matrix_mul clean all run TARGET=PULP_OPEN PLATFORM=GVSOC USE_CLUSTER=1 NUM_CORES=1 ENABLE_LOGGING=1 STATS=1
```

### Generating Test Data
Each test case includes a Python script (`generator.py`) to generate input data and expected results. The script typically takes parameters such as vector or matrix dimensions. Data is saved to `test/<kernel/test_data/data.h`.

To regenerate data:
```bash
$ python3 generator.py <LEN>
```

For example:

```bash
python3 test/vector_mul/test_data/generator.py 128
```

### Scripts & Benchmarks
The `test/runners/<arch>` directory contains several Python scripts to automate test execution, parse outputs and traces, and to generate graphs and benchmark tables. For example, see the Pulp Open benchmarks for QuestaSim [here](test/runners/pulp-open/benchmarks/rtl/benchmarks.md) and the Pulp Open benchmarks for GVSoC [here](test/runners/pulp-open/benchmarks/gvsoc/benchmarks.md).


## How to Use PLAY in your own project
To use **PLAY**  as a library in your project, follow these steps:

1.  **Include the Header**: Add the single header in your source code: `#include "play.h"`
2.  **Include Path**: Add the library's `include` directory to your compiler's include path (e.g., `-Ipath/to/PLAY/include`).
3.  **Compilation & Linking**: Compile and link the library by adding:
    * The top-level implementation files: `source/*/*.c`
    * The architecture-specific implementation file(s) for your target: `source/<kernel>/arch/<kernel>_$(ARCH_SUFFIX).c` (e.g., `source/<kernel>/arch/<kernel>_pulp_open.c`).
4.  **Data Format** (optional): add `-DPLAY_FORMAT=PLAY_FMT_FP16` or `-DPLAY_FORMAT=PLAY_FMT_BF16` to the compile flags of both the library and your code to work on half-precision data (see [Data Formats](#data-formats)). Without it the library works on `float`.

The single `play.h` header simplifies adoption: it exposes the public API while the build system selects and compiles the correct backend implementation for the chosen architecture.

## How to add a new architecture

1. Kernel implementation (add architecture support to kernels)
   - Pick an architecture suffix, e.g. `myarch`.
   - Add `source/<kernel>/arch/<kernel>_myarch.c` and implement `<kernel>_impl(...)`.
   - **Note:** the implementation must match the function signature declared in `include/internal/arch_interface.h` exactly.

2. Register the architecture in the build mapping
   - Update `test/common/arch_select.mk` to map your `TARGET` to `ARCH_SUFFIX` so `add_arch_impl` will pick the correct file for each kernel.

3. Add test cases
   - Implement the test case under `test/<kernel>/<new-arch>` (wraps for PULP and/or SPATZ as needed).

4. Test data generation
   - If your new architecture requires different test data or conditional data, adapt the Python test-data generator(s) (e.g. `test/*/test_data/generator.py`) so generated headers (`data.h`) or test inputs include the appropriate `#if TARGET_IS_<ARCH>` branches.

5. Update top-level build files
   - Adapt the top-level Makefile(s) or CMake configuration to include/build the target architecture. This usually means adding a target or including the proper per-architecture build snippet so the project supports a `make TARGET=MYARCH` (or equivalent) invocation.

**Note:** remember to document any new toolchain or dependency requirements for your architecture, add example build commands to the README.
