# BootSYS — RISC-V 底层验证系统（非 OS / 非 bootloader）

BootSYS 用 Rust 编写，在裸机 M-mode 下跑，用来验证 RISC-V SoC 底层子系统。  
**不是操作系统，也不是 bootloader**；第一步先把板级启动与串口打通。

## 目录结构

```
BootSYS/
├── crates/bootsys/          # 主框架（main.rs）
├── crates/bootsys-core/     # BoardInfo / 控制台 / NS16550
├── arch/riscv/boot.S        # RISC-V 公共启动
├── boards/k230/             # 仅板级信息 + linker.ld
│   └── src/lib.rs           # 板名、UART 基址、时钟、波特率
└── Makefile                 # make BOARD=k230
```

## 支持板卡

| 板卡 | 状态 | 说明 |
|------|------|------|
| **K230** | 已实现 | `BoardInfo` + UART bring-up |
| **GSC64B0** | 规划中 | 后续只加 `boards/gsc64b0` |

## 环境安装（Ubuntu）

见 [docs/ENV_SETUP_UBUNTU_CN.md](docs/ENV_SETUP_UBUNTU_CN.md)（Ubuntu 24.04 实测通过）。

## 快速构建

```bash
make BOARD=k230
# 产物：
#   out/k230/bootsys.elf
#   out/k230/bootsys.bin
```

板名与 UART 地址来自 `boards/k230`，不在 `main.rs` 写死。上电串口期望：

```text
BootSYS / K230
UART0 @ 0x91400000, baud 115200
ready - RISC-V low-level verification system
```

## 下一步

- GSC64B0 板级 `BoardInfo` + `linker.ld`
- 更多底层验证项（CSR、cache、PMP/PMA、中断等）
