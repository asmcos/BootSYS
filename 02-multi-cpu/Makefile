# 02-multi-cpu — 完整多核固件（在 01 能力上叠加 CPU1）
#
#   cd 02-multi-cpu && make
#   → ../out/02-multi-cpu/bootsys.bin

BOARD         ?= k230
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
LOAD          ?= sram

CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

HERE    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO    := $(abspath $(HERE)/..)
OUT_DIR := $(REPO)/out/02-multi-cpu
OBJ_DIR := $(OUT_DIR)/obj
GIT_REV := $(shell git -C $(REPO) rev-parse --short HEAD 2>/dev/null || echo none)

BOARD_DIR := $(HERE)/boards/$(BOARD)
ARCH_DIR  := $(HERE)/arch/riscv
SRC_DIR   := $(HERE)/src

ifeq ($(LOAD),ddr)
  LDSCRIPT := $(BOARD_DIR)/linker_ddr.ld
  CFLAGS_LOAD := -DLOAD_DDR
else
  LDSCRIPT := $(BOARD_DIR)/linker.ld
  CFLAGS_LOAD :=
endif

ARCH_FLAGS = -march=rv64imac -mabi=lp64 -mcmodel=medany
INCLUDES   = -I$(BOARD_DIR) -I$(SRC_DIR)
CFLAGS     = $(ARCH_FLAGS) -nostdlib -nostartfiles -ffreestanding \
             -fno-builtin -msmall-data-limit=0 -O2 -Wall -Wextra \
             $(INCLUDES) $(CFLAGS_LOAD) \
             -DBOOTSYS_VERSION=\"0.2.0\" \
             -DBOOTSYS_GIT=\"$(GIT_REV)\"
LDFLAGS    = $(ARCH_FLAGS) -nostdlib -nostartfiles -Wl,--gc-sections \
             -T $(LDSCRIPT)

ARCH_SRCS = $(ARCH_DIR)/start.S $(ARCH_DIR)/cache.S
SRC_SRCS  = $(SRC_DIR)/main.c $(SRC_DIR)/banner.c \
            $(SRC_DIR)/hart1_main.c $(SRC_DIR)/shmem.c
BOARD_SRCS = $(BOARD_DIR)/sysctl.c $(BOARD_DIR)/cache_image.c \
             $(BOARD_DIR)/uart.c $(BOARD_DIR)/clint.c \
             $(BOARD_DIR)/cpu1.c $(BOARD_DIR)/cpu1_status.c

ALL_SRCS = $(ARCH_SRCS) $(SRC_SRCS) $(BOARD_SRCS)
OBJS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(basename $(notdir $(ALL_SRCS)))))

vpath %.c $(SRC_DIR) $(BOARD_DIR)
vpath %.S $(ARCH_DIR)

TARGET = bootsys
ELF    = $(OUT_DIR)/$(TARGET).elf
BIN    = $(OUT_DIR)/$(TARGET).bin

.PHONY: all release bin clean disasm size help ddr

all: release

help:
	@echo "02-multi-cpu:"
	@echo "  make / make BOARD=k230   -> ../out/02-multi-cpu/bootsys.bin"
	@echo "  make LOAD=ddr            DDR link"
	@echo "  make clean"

release: bin
bin: $(BIN)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "=> $(BIN)"

$(ELF): $(OBJS) $(LDSCRIPT)
	@mkdir -p $(OUT_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	@$(SIZE) $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

disasm: $(ELF)
	$(OBJDUMP) -d $< > $(OUT_DIR)/$(TARGET).s

size: $(ELF)
	$(SIZE) $<

ddr:
	$(MAKE) LOAD=ddr

clean:
	rm -rf $(OUT_DIR)
