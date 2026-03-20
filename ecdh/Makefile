# Makefile for ECDH Benchmark on CW308T-STM32F3

# ============================================
# Configuration
# ============================================

# Target MCU
DEVICE = stm32f303rct7
OPENCM3_DIR = libopencm3
OPENCM3_TARGET = lib/stm32/f3
MICROECC_DIR = micro-ecc

# Target name
TARGET = ecdh_benchmark
PLATFORM = cw308t-stm32f3

# ============================================
# Optimization Flags
# ============================================

OPT_FLAGS=-O3 -g3

# ============================================
# Toolchain
# ============================================

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE = $(PREFIX)size

# ============================================
# Device Information (from libopencm3)
# ============================================

DEVICES_DATA = $(OPENCM3_DIR)/ld/devices.data
LDSCRIPT = generated.$(DEVICE).ld

GENLINK_FAMILY    := $(shell python $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) FAMILY)
GENLINK_CPU       := $(shell python $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) CPU)
GENLINK_FPU       := $(shell python $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) FPU)
GENLINK_CPPFLAGS  := $(shell python $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) CPPFLAGS)
GENLINK_DEFS      := $(shell python $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) DEFS)

LIBNAME = opencm3_$(GENLINK_FAMILY)

# ============================================
# Compiler Flags
# ============================================

SYSROOT := $(shell $(CC) --print-sysroot)

# FPU flags (for Cortex-M4 performance)
ifeq ($(GENLINK_FPU),hard-fpv4-sp-d16)
ARCH_FLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
endif

# Architecture flags
ARCH_FLAGS = -mcpu=$(GENLINK_CPU) -mthumb
ifeq ($(GENLINK_FPU),hard-fpv4-sp-d16)
ARCH_FLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
endif

# Preprocessor flags
CPPFLAGS = $(GENLINK_CPPFLAGS) $(GENLINK_DEFS)
CPPFLAGS += -I$(OPENCM3_DIR)/include
CPPFLAGS += -I$(MICROECC_DIR)
CPPFLAGS += -I.

# Compiler flags
CFLAGS = $(ARCH_FLAGS)
CFLAGS += $(OPT_FLAGS)
CFLAGS += -std=gnu99
CFLAGS += --sysroot=$(SYSROOT)
CFLAGS += -Wall -Wextra -Wshadow
CFLAGS += -MMD
CFLAGS += -fno-common
CFLAGS += $(CPPFLAGS)

# Linker flags
LDFLAGS = -L$(OPENCM3_DIR)/lib
LDFLAGS += --specs=nosys.specs
LDFLAGS += -nostartfiles
LDFLAGS += -ffreestanding
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += $(ARCH_FLAGS)

LDLIBS = -l$(LIBNAME) -lc -lgcc

# ============================================
# Build Targets
# ============================================

# Output files
HEXFILE = $(TARGET).hex
ELFFILE = $(TARGET).elf

# Default target
.PHONY: all
all: $(HEXFILE)

# Build libopencm3
$(OPENCM3_DIR)/lib/lib$(LIBNAME).a:
	@echo "Building libopencm3..."
	$(MAKE) -C $(OPENCM3_DIR) $(OPENCM3_TARGET)

# Generate linker script
$(LDSCRIPT): $(OPENCM3_DIR)/ld/linker.ld.S $(OPENCM3_DIR)/ld/devices.data
	@echo "Generating linker script..."
	$(CC) -E -P $(ARCH_FLAGS) $(GENLINK_DEFS) $< -o $@

# Compile and link
$(ELFFILE): ecdh_benchmark.c hal.c randombytes.c hal.h randombytes.h \
            $(OPENCM3_DIR)/lib/lib$(LIBNAME).a $(LDSCRIPT)
	@echo "Compiling and linking..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ \
	    ecdh_benchmark.c hal.c randombytes.c \
	    -Wl,--start-group $(LDLIBS) -Wl,--end-group
	@echo ""
	$(SIZE) $@

# Create hex file
$(HEXFILE): $(ELFFILE)
	@echo "Creating hex file..."
	$(OBJCOPY) -O ihex $< $@

# Clean
clean:
	rm -f $(HEXFILE) $(ELFFILE) $(LDSCRIPT) *.o *.d