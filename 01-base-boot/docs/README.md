# 01-base-boot 目录说明

| 文件 | 作用 |
|------|------|
| `start.S` | CPU0 `_start` |
| `cache.S` | `thead_cpu_init` |
| `uart.c` / `board.h` | UART0 |
| `main.c` | banner + echo |
| `linker.ld` | SRAM，单栈 |

上级总览见仓库根 `README.md`。
