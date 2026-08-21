# BootSYS — RISC-V 底层验证固件（非 OS / 非 bootloader）

BootSYS 用 Rust 编写，在裸机 M-mode 下跑，用来验证 RISC-V SoC 底层子系统。  
**不是操作系统，也不是 bootloader**；第一步先把板级启动与串口打通。

## 支持板卡

| 板卡 | 状态 | 说明 |
|------|------|------|
| **K230** | 已实现 | 启动 + UART0（参考 `riscvboot`） |
| **GSC64B0** | 规划中 | 后续接入 |

## K230 快速构建

```bash
# 依赖：rustup + target riscv64imac-unknown-none-elf
#       riscv64-unknown-linux-gnu-objcopy（或改 CROSS_COMPILE）

make                # 默认 BOARD=k230
# 产物：
#   out/k230/bootsys-k230.elf
#   out/k230/bootsys-k230.bin
```

加载地址与 `riscvboot` 一致：镜像入口 **`0x80200000`**，UART0 **`0x91400000`**，波特率 **115200**（输入时钟 48.6 MHz）。

上电后串口应看到：

```
BootSYS / K230
UART0 @ 0x91400000, baud 115200
ready — RISC-V low-level verification firmware
```

之后串口输入会回显（冒烟测试）。

## 目录结构

```
BootSYS/
├── crates/bootsys-core/   # MMIO / UART trait / print!
├── boards/k230/           # K230 启动、链接脚本、UART0
├── Makefile
└── out/                   # 构建产物
```

## 与 riscvboot 的对应关系

| riscvboot | BootSYS |
|-----------|---------|
| `arch/boot.S` | `boards/k230/src/boot.S` |
| `linker.ld` | `boards/k230/linker.ld` |
| `src/uart0.c` | `boards/k230/src/uart.rs` |
| `src/main.c` | `boards/k230/src/main.rs` |

## 工具链

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup target add riscv64imac-unknown-none-elf
```

`CROSS_COMPILE` 默认 `riscv64-unknown-linux-gnu-`，可按本机工具链覆盖。

## 下一步

- GSC64B0 板级包（UART / 内存布局）
- 更多底层验证项（CSR、cache、PMP/PMA、中断等）
