# 01-base-boot 目录说明

| 文件 | 作用 |
|------|------|
| `start.S` | CPU0 `_start` |
| `cache.S` | `thead_cpu_init` |
| `uart.c` / `board.h` | UART0 |
| `main.c` | banner + echo |
| `linker.ld` | SRAM，单栈 |

原理与验收见 [BASE_BOOT_CN.md](BASE_BOOT_CN.md)。  
上级总览：[仓库首页](../../README.md) · [文档索引](../../docs/README.md)。
