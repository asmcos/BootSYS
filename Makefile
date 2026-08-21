# ==============================================
# BootSYS — RISC-V low-level verification system
# ==============================================

BOARD ?= k230
TARGET = bootsys
BOARD_FEATURE = board-$(BOARD)

CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

CARGO ?= cargo
PROFILE ?= release
BUILD_DIR = target/riscv64imac-unknown-none-elf/$(PROFILE)
ELF = $(BUILD_DIR)/$(TARGET)
OUT_DIR = out/$(BOARD)

.PHONY: all release debug clean disasm size bin help color-check

all: release

help:
	@echo "BootSYS targets:"
	@echo "  make / make release   Build bootsys for BOARD=$(BOARD)"
	@echo "  make BOARD=k230       Select board (feature board-k230)"
	@echo "  make debug            Debug build + .bin"
	@echo "  make disasm           Dump disassembly"
	@echo "  make color-check      Host ANSI color probe (PC terminal)"
	@echo "  make clean            Remove build artifacts"

release: bin

bin: $(OUT_DIR)/$(TARGET).bin

$(OUT_DIR)/$(TARGET).bin: $(ELF)
	@mkdir -p $(OUT_DIR)
	$(OBJCOPY) -O binary $< $@
	@cp -f $< $(OUT_DIR)/$(TARGET).elf
	@echo "=> $(OUT_DIR)/$(TARGET).elf"
	@echo "=> $(OUT_DIR)/$(TARGET).bin"

$(ELF): force
	$(CARGO) build -p bootsys --no-default-features --features $(BOARD_FEATURE) --$(PROFILE)

debug:
	$(MAKE) PROFILE=debug bin

disasm: $(ELF)
	@mkdir -p $(OUT_DIR)
	$(OBJDUMP) -d $< > $(OUT_DIR)/$(TARGET).s
	@echo "=> $(OUT_DIR)/$(TARGET).s"

size: $(ELF)
	$(READELF) -h $<
	@$(CROSS_COMPILE)size $<

clean:
	$(CARGO) clean
	rm -rf out;rm -f Cargo.lock

# Host-only: verify ANSI colors in the PC terminal (same escapes as firmware).
color-check:
	cd tools/color-check && $(CARGO) run --target x86_64-unknown-linux-gnu

force: ;
