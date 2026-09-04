# BootSYS — RISC-V 底层验证系统（非 OS / 非 bootloader）

**纯 C + ASM**。

## 目录

```
BootSYS/
├── arch/riscv/          # 启动 / cache（ASM）
├── src/                 # 主程序与共享协议（banner / shmem / hart1）
├── boards/k230/         # 板级 HAL + linker.ld
├── docs/
└── Makefile
```

## 构建

```bash
make BOARD=k230          # SRAM @ 0x80200000 → out/k230/bootsys.bin
make LOAD=ddr            # DDR 链接（需 DRAM 已初始化）
./cp_bin.sh
```

需要：`riscv64-*-gcc`（或 `make CROSS_COMPILE=riscv64-linux-gnu-`）。

说明：
- [docs/K230_DUAL_CORE_CN.md](docs/K230_DUAL_CORE_CN.md)
- [docs/ENV_SETUP_UBUNTU_CN.md](docs/ENV_SETUP_UBUNTU_CN.md)
