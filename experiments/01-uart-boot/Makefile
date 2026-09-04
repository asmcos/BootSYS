# Experiment 01 — minimal UART boot (CPU0 only)
#
#   make                 -> ../../out/exp01/bootsys.bin
#   make CROSS_COMPILE=riscv64-linux-gnu-

CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE    = $(CROSS_COMPILE)size

ROOT    = $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
OUT_DIR = $(ROOT)/out/exp01
GIT_REV = $(shell git -C $(ROOT) rev-parse --short HEAD 2>/dev/null || echo none)

ARCH_FLAGS = -march=rv64imac -mabi=lp64 -mcmodel=medany
CFLAGS  = $(ARCH_FLAGS) -nostdlib -nostartfiles -ffreestanding \
          -fno-builtin -msmall-data-limit=0 -O2 -Wall -Wextra \
          -DBOOTSYS_GIT=\"$(GIT_REV)\"
LDFLAGS = $(ARCH_FLAGS) -nostdlib -nostartfiles -Wl,--gc-sections -T linker.ld

SRCS = start.S cache.S uart.c main.c
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.S=.o)

.PHONY: all clean bin

all: bin

bin: $(OUT_DIR)/bootsys.bin

$(OUT_DIR)/bootsys.bin: bootsys.elf
	@mkdir -p $(OUT_DIR)
	$(OBJCOPY) -O binary $< $@
	cp -f $< $(OUT_DIR)/bootsys.elf
	@echo "=> $(OUT_DIR)/bootsys.elf"
	@echo "=> $(OUT_DIR)/bootsys.bin"

bootsys.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	@$(SIZE) $@

%.o: %.c board.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) bootsys.elf
