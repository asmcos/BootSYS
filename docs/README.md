# BootSYS 文档索引

按子项目阅读；根目录 [README.md](../README.md) 有编译总览。

## 公共

| 文档 | 说明 |
|------|------|
| [ENV_SETUP_UBUNTU_CN.md](ENV_SETUP_UBUNTU_CN.md) | Ubuntu 交叉工具链安装 |

## 子项目

| 顺序 | 目录 | 文档 |
|------|------|------|
| **01** | [01-base-boot](../01-base-boot/) | [BASE_BOOT_CN.md](../01-base-boot/docs/BASE_BOOT_CN.md) — 最小启动 + UART |
| **02** | [02-multi-cpu](../02-multi-cpu/) | [MULTI_CPU_CN.md](../02-multi-cpu/docs/MULTI_CPU_CN.md) — 多核阶进<br>[K230_DUAL_CORE_CN.md](../02-multi-cpu/docs/K230_DUAL_CORE_CN.md) — CPU1 原理与寄存器 |
| **03** | [03-exception](../03-exception/) | [EXCEPTION_CN.md](../03-exception/docs/EXCEPTION_CN.md) — trap / 异常打印 |
| **04** | [04-privilege](../04-privilege/) | [PRIVILEGE_CN.md](../04-privilege/docs/PRIVILEGE_CN.md) — M / S / U 切换与权限 |
| **05** | [05-pmp](../05-pmp/) | [PMP_CN.md](../05-pmp/docs/PMP_CN.md) — 设保护 / 取消后再测 |
| **06** | [06-tlb](../06-tlb/) | [TLB_CN.md](../06-tlb/docs/TLB_CN.md) — Sv39 / TLB / sfence.vma |
| **07** | [07-cache](../07-cache/) | [CACHE_CN.md](../07-cache/docs/CACHE_CN.md) — L1 tag dump |
| **08** | [08-timer](../08-timer/) | [TIMER_CN.md](../08-timer/docs/TIMER_CN.md) — CLINT 定时器 / 秒表 |
| **09** | [09-irq](../09-irq/) | [IRQ_CN.md](../09-irq/docs/IRQ_CN.md) — UART RX / PLIC |

建议顺序：**环境 → 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09**。
