# ==============================================
# BootSYS — RISC-V low-level verification (C)
# ==============================================
#
# Layout:
#   arch/riscv/     early boot + cache (ASM)
#   src/            app / shared protocol
#   boards/<board>/ board HAL + linker
#   docs/

BOARD         ?= k230
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
LOAD          ?= sram

CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

ROOT    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUT_DIR := $(ROOT)/out/$(BOARD)
OBJ_DIR := $(OUT_DIR)/obj
GIT_REV := $(shell git -C $(ROOT) rev-parse --short HEAD 2>/dev/null || echo none)

BOARD_DIR := $(ROOT)/boards/$(BOARD)
ARCH_DIR  := $(ROOT)/arch/riscv
SRC_DIR   := $(ROOT)/src

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
             -DBOOTSYS_VERSION=\"0.1.0\" \
             -DBOOTSYS_GIT=\"$(GIT_REV)\"
LDFLAGS    = $(ARCH_FLAGS) -nostdlib -nostartfiles -Wl,--gc-sections \
             -T $(LDSCRIPT)

# --- sources ---
ARCH_SRCS = $(ARCH_DIR)/start.S $(ARCH_DIR)/cache.S

SRC_SRCS  = $(SRC_DIR)/main.c \
            $(SRC_DIR)/banner.c \
            $(SRC_DIR)/hart1_main.c \
            $(SRC_DIR)/shmem.c

BOARD_SRCS = $(BOARD_DIR)/sysctl.c \
             $(BOARD_DIR)/cache_image.c \
             $(BOARD_DIR)/uart.c \
             $(BOARD_DIR)/clint.c \
             $(BOARD_DIR)/cpu1.c \
             $(BOARD_DIR)/cpu1_status.c

ALL_SRCS = $(ARCH_SRCS) $(SRC_SRCS) $(BOARD_SRCS)

# Map each source to out/<board>/obj/<name>.o
OBJS = $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(basename $(notdir $(ALL_SRCS)))))

# VPATH so %.o can find sources in multiple dirs
vpath %.c $(SRC_DIR) $(BOARD_DIR)
vpath %.S $(ARCH_DIR)

TARGET = bootsys
ELF    = $(OUT_DIR)/$(TARGET).elf
BIN    = $(OUT_DIR)/$(TARGET).bin

.PHONY: all release bin clean disasm size help ddr exp1

all: release

help:
	@echo "BootSYS targets:"
	@echo "  make / make BOARD=k230   Multi-cpu demo -> out/k230/bootsys.bin  (exp2)"
	@echo "  make exp1               UART-only boot  -> out/exp01/bootsys.bin (exp1)"
	@echo "  make LOAD=ddr           Link multi-cpu for DDR"
	@echo "  make disasm / clean"

release: bin

bin: $(BIN)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "=> $(ELF)"
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
	@echo "=> $(OUT_DIR)/$(TARGET).s"

size: $(ELF)
	$(SIZE) $<

ddr:
	$(MAKE) LOAD=ddr

exp1:
	$(MAKE) -C experiments/01-uart-boot

clean:
	rm -rf $(ROOT)/out
	$(MAKE) -C experiments/01-uart-boot clean || true
