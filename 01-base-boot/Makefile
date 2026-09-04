# 01-base-boot — CPU0 最小启动 + 串口
#
#   cd 01-base-boot && make
#   → ../out/01-base-boot/bootsys.bin

CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE    = $(CROSS_COMPILE)size

HERE    = $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO    = $(abspath $(HERE)/..)
OUT_DIR = $(REPO)/out/01-base-boot
GIT_REV = $(shell git -C $(REPO) rev-parse --short HEAD 2>/dev/null || echo none)

ARCH_FLAGS = -march=rv64imac -mabi=lp64 -mcmodel=medany
CFLAGS  = $(ARCH_FLAGS) -nostdlib -nostartfiles -ffreestanding \
          -fno-builtin -msmall-data-limit=0 -O2 -Wall -Wextra \
          -DBOOTSYS_GIT=\"$(GIT_REV)\"
LDFLAGS = $(ARCH_FLAGS) -nostdlib -nostartfiles -Wl,--gc-sections -T linker.ld

SRCS = start.S cache.S uart.c main.c
OBJS = $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS)))

.PHONY: all clean bin

all: bin

bin: $(OUT_DIR)/bootsys.bin

$(OUT_DIR)/bootsys.bin: bootsys.elf
	@mkdir -p $(OUT_DIR)
	$(OBJCOPY) -O binary $< $@
	cp -f $< $(OUT_DIR)/bootsys.elf
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
	rm -rf $(OUT_DIR)
