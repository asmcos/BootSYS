# 实验 01：最小启动 + 串口（仅 CPU0）

BootSYS **实验序列**中的第一课：验证「能跑起来、能打印」。

**不含** CPU1 / shmem / CLINT / 菜单。

## 构建

```bash
# 仓库根目录
make exp1
# 或
make -C experiments/01-uart-boot
```

产物：`out/exp01/bootsys.bin`（加载地址 `0x80200000`）。

## 期望串口

- 圆角信息框（Experiment 01）
- `cpu0 mhartid=0x0...`
- 键盘回显

## 文件（本目录自洽）

| 文件 | 作用 |
|------|------|
| `start.S` | CPU0 `_start` |
| `cache.S` | `thead_cpu_init` |
| `uart.c` / `board.h` | UART0 |
| `main.c` | banner + echo |
| `linker.ld` | SRAM 布局（单栈） |

## Git 标签

`exp1-uart-boot` — 本实验里程碑（可后补打，见 `docs/EXPERIMENTS_CN.md`）。
