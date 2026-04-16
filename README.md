# Post-Quantum Bluetooth Classic Evaluation
This repository provides the experimental framework and benchmarking data for the research article "Quantum-Resistant Pairing for Resource-Constrained Bluetooth Classic".

## Overview
The objective of this work is to evaluate the protocol-level and computational overhead incurred when migrating Bluetooth Classic pairing from traditional Elliptic Curve Cryptography (ECC) to Post-Quantum Cryptography (PQC).

This repository specifically focuses on comparing the computational performance of the P-192 (secp192r1) and P-256 (secp256r1) curves against the NIST-standardized ML-KEM variants when executed on resource-constrained hardware under the same compilation and board conditions ressembling the constraints posed by commercial Bluetooth modules. As a result, experiments were conducted using:

- *Target MCU*: STM32F3 (ARM Cortex-M4).

- *Platform*: ChipWhisperer [CW308-STM32F3](https://chipwhisperer.readthedocs.io/en/latest/Targets/CW308%20UFO.html) UFO Baseboard.

- *Metrics*: Precise CPU cycle counts for each of the cryptographic functions.

This repository is organized into two main modules to separate ECDH and ML-KEM tests, containing the algorithm implementations, benchmarking materials, and results.

## ML-KEM tests (`pqbt-eval/ml-kem`)
This subdirectory is a submodule of a [pqm4](https://github.com/mupq/pqm4) fork, a project tasked with providing PQC reference and optimized implementations that target the ARM Cortex-M4 processor. The parameters used during compilation are:

- CFLAGS (Compiler Flags) found in `pqm4/mupq/mk/config.mk`: Options for the C compiler used to set optimization, warnings, debugging...
    - The `interface.py` SCRIPT FROM PQM4 allows to choose different optimization flags: `--opt` or `-o` {speed,size,debug} (default is speed, which includes the `-O3` `-g3` flags)
    
- ARCH_FLAGS (Architecture Flags) found in pqm4/mk/opencm3.mk: Sets hardware-specific compiler flags.
- LDFLAGS (Linker Flags) found in `pqm4/mk/cw308t-stm32f3.mk`: Specifies the target device and library target.

The clock frequency is set to 36 MHz by adjusting the settings in `pqm4/common/hal-opencm3.c` for the CW308-STM32F3 board, thus ressembling the clock frequency of Silicon Lab's BT122 Bluetooth module.

### Results:
The ML-KEM benchmarking results obtained during experimentation can be found under the `pqbt/ml-kem/pqm4/benchmarks`, ordered by security level and implementation (optimized/reference). These are the results obtained from executing 100 runs of each function with a clock frequency of 36 MHz. 

| scheme | implementation | key generation [cycles] | encapsulation [cycles] | decapsulation [cycles] |
| ------ | -------------- | ----------------------- | ---------------------- | ---------------------- |
| ml-kem-1024 (100 executions) | clean | AVG: 1,667,847 <br /> MIN: 1,665,825 <br /> MAX: 1,679,154 | AVG: 1,879,878 <br /> MIN: 1,875,841 <br /> MAX: 1,893,182 | AVG: 2,218,146 <br /> MIN: 2,216,203 <br /> MAX: 2,229,436 |
| ml-kem-1024 (100 executions) | m4fspeed | AVG: 1,108,155 <br /> MIN: 1,105,857 <br /> MAX: 1,119,611 | AVG: 1,117,974 <br /> MIN: 1,113,840 <br /> MAX: 1,131,496 | AVG: 1,181,828 <br /> MIN: 1,179,528 <br /> MAX: 1,193,235 |
| ml-kem-1024 (100 executions) | m4fstack | AVG: 1,117,490 <br /> MIN: 1,115,192 <br /> MAX: 1,128,894 | AVG: 1,133,514 <br /> MIN: 1,129,307 <br /> MAX: 1,147,018 | AVG: 1,198,272 <br /> MIN: 1,195,989 <br /> MAX: 1,209,662 |
| ml-kem-512 (100 executions) | clean | AVG: 642,211 <br /> MIN: 641,355 <br /> MAX: 653,892 | AVG: 761,987 <br /> MIN: 759,112 <br /> MAX: 775,701 | AVG: 962,941 <br /> MIN: 962,116 <br /> MAX: 974,620 |
| ml-kem-512 (100 executions) | m4fspeed | AVG: 430,087 <br /> MIN: 429,065 <br /> MAX: 441,870 | AVG: 426,644 <br /> MIN: 423,631 <br /> MAX: 440,476 | AVG: 464,116 <br /> MIN: 463,117 <br /> MAX: 475,904 |
| ml-kem-512 (100 executions) | m4fstack | AVG: 430,468 <br /> MIN: 429,461 <br /> MAX: 442,273 | AVG: 429,188 <br /> MIN: 426,184 <br /> MAX: 442,998 | AVG: 467,040 <br /> MIN: 466,020 <br /> MAX: 478,840 |
| ml-kem-768 (100 executions) | clean | AVG: 1,070,422 <br /> MIN: 1,068,908 <br /> MAX: 1,082,110 | AVG: 1,248,926 <br /> MIN: 1,245,295 <br /> MAX: 1,262,731 | AVG: 1,521,015 <br /> MIN: 1,519,501 <br /> MAX: 1,532,703 |
| ml-kem-768 (100 executions) | m4fspeed | AVG: 701,045 <br /> MIN: 699,190 <br /> MAX: 712,993 | AVG: 716,417 <br /> MIN: 712,439 <br /> MAX: 730,494 | AVG: 765,580 <br /> MIN: 763,719 <br /> MAX: 777,540 |
| ml-kem-768 (100 executions) | m4fstack | AVG: 702,992 <br /> MIN: 701,192 <br /> MAX: 714,918 | AVG: 722,778 <br /> MIN: 718,881 <br /> MAX: 736,810 | AVG: 772,746 <br /> MIN: 770,965 <br /> MAX: 784,662 |

## ECDH tests (`pqbt-eval/ecdh`)

### Overview

This subdirectory benchmarks the previously mentioned ECDH algorithms using [micro-ecc](https://github.com/kmackay/micro-ecc) as the base implementations and [libopencm3](https://github.com/libopencm3/libopencm3) to build the executables that are flashed into the board.

The implementation and compilation of the ECDH algorithms during testing underwent several adjustments to accurately match the same criteria used when testing the ML-KEM variants. Consequently, this subdirectory:

- Utilize's pqm4's RNG instead of the one provided by the micro-ecc implementation by using `randombytes.c` from `pqm4/common` and `randombytes.h` from `pqm4/mupq/pqclean/common`:
The `uECC_set_rng()` function from the micro-ecc repo is used to set the randombytes function from pqm4. Since randombytes returns 0 on success and micro-ecc expects 1 on success, a callback function `rng_callback()` is defined in ecdh_benchmark.c which points to randombytes's memory address but handling the output success/failure output correctly.

- Uses the same compilation parameters as those used in the pqm4 project.

- Enables the use of optimization techniques provided by the micro-ecc library:
    - `uECC_ARM_USE_UMAAL`: Controls whether to use the UMAAL instruction (Unsigned Multiply Accumulate Accumulate Long), a powerful ARM DSP instruction. It is enabled when set to 1.
    - `uECC_SQUARE_FUNC`: Swaps a new defined function for scalar squaring instead of the generic multiplication function. This can make things faster somewhat faster, but increases the code size. It is enabled when set to 1.
    - `uECC_OPTIMIZATION_LEVEL`: Different optimization levels ranging from 0-4. The larger the value, the faster th resultant code, yet it increases in code size. They are specified in the `.inc` files, which are called from `uECC.c` depending on the platform used.

| Level | Effect                                              |
|-------|-----------------------------------------------------|
| 0     | Generic C code only                                 |
| 1     | Adds curve-specific modular reduction               |
| 2     | Adds ARM assembly for add/subtract operations       |
| 3     | Adds ARM assembly for multiplication (single curve) |
| 4     | Adds ARM assembly for multiplication (multi-curve)  |

- The ARM instruction set is assigned to ARM Thumb 2:

In `ecdh_behcmark.c`: `#define uECC_PLATFORM uECC_arm_thumb2`

This ensures that the executables will contain CPU instructions supported by the ARM Cortex-M4, since this processor only supports instructions from the thumb/thumb2 instruction set. In particular, thumb2 keeps the 16‑bit Thumb instructions but adds many more instructions plus 32‑bit Thumb encodings, letting the CPU mix 16‑bit and 32‑bit instructions in the same code stream to get both density and capability/performance.

### Results:
The P-192 and P-256 results obtained during experimentation can be found under the `pqbt/ecdh/benchmarks`, structured by the optimization options available in the implementation of the algorithms. These are the results obtained from executing 100 runs of each function with a clock frequency of 36 MHz.

### High Optimization (uECC_ARM_USE_UMAAL = 1, Optimization Level 4, SQUARE_FUNC = 1)

| Curve   | Operation      | Avg Cycles | Min Cycles | Max Cycles | Avg Time (ms) |
|---------|----------------|------------|------------|------------|---------------|
| P-192   | keygen         | 1,974,939  | 1,965,621  | 1,985,076  | 54.859        |
| P-192   | sharedsecret   | 1,973,691  | 1,962,257  | 1,983,669  | 54.825        |
| P-256   | keygen         | 5,763,536  | 5,750,075  | 5,779,386  | 160.098       |
| P-256   | sharedsecret   | 5,761,502  | 5,748,325  | 5,778,084  | 160.042       |

### Medium Optimization (uECC_ARM_USE_UMAAL = 0, Optimization Level 3, SQUARE_FUNC = 0)

| Curve   | Operation      | Avg Cycles | Min Cycles | Max Cycles | Avg Time (ms) |
|---------|----------------|------------|------------|------------|---------------|
| P-192   | keygen         | 2,451,094  | 2,441,746  | 2,461,264  | 68.086        |
| P-192   | sharedsecret   | 2,449,757  | 2,438,246  | 2,459,687  | 68.049        |
| P-256   | keygen         | 7,142,321  | 7,128,905  | 7,158,118  | 198.398       |
| P-256   | sharedsecret   | 7,140,100  | 7,126,949  | 7,156,853  | 198.336       |


### Default Optimization (uECC_ARM_USE_UMAAL = 1, Optimization Level 2, SQUARE_FUNC = 0)

| Curve   | Operation      | Avg Cycles | Min Cycles | Max Cycles | Avg Time (ms) |
|---------|----------------|------------|------------|------------|---------------|
| P-192   | keygen         | 3,742,977  | 3,733,508  | 3,753,183  | 103.972       |
| P-192   | sharedsecret   | 3,741,528  | 3,729,920  | 3,751,579  | 103.931       |
| P-256   | keygen         | 9,933,674  | 9,920,209  | 9,949,593  | 275.935       |
| P-256   | sharedsecret   | 9,931,275  | 9,918,109  | 9,947,856  | 275.869       |


## Notes
To successfuly execute the code provided in this repository, it is required to first install [ChipWhisperer's python module](https://chipwhisperer.readthedocs.io/en/latest/index.html). This package is used to perform any sort of interaction with the target board, like flashing binaries or retrieving output data.
