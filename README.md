# BootSYS — RISC-V 底层验证系统（非 OS / 非 bootloader）

**纯 C + ASM**。按实验递进（见 [docs/EXPERIMENTS_CN.md](docs/EXPERIMENTS_CN.md)）。

## 实验

| # | 内容 | 构建 | Tag |
|---|------|------|-----|
| 1 | 最小启动 + 串口（仅 CPU0） | `make exp1` | `exp1-uart-boot` |
| 2 | 多核运行 demo | `make BOARD=k230` | `exp2-multi-cpu` |

## 目录

```
BootSYS/
├── experiments/01-uart-boot/   # 实验一（纯净、自洽）
├── arch/ / src/ / boards/k230/ # 当前主树（多核）
├── docs/
└── Makefile
```

## 环境

交叉编译：`riscv64-*-gcc`（或 `make CROSS_COMPILE=riscv64-linux-gnu-`）。  
说明：[docs/ENV_SETUP_UBUNTU_CN.md](docs/ENV_SETUP_UBUNTU_CN.md)
