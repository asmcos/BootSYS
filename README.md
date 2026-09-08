# BootSYS

## 什么是 BootSYS

BootSYS 是在机器开机后，通过 JTAG 烧写到 RAM，或由引导设备加载后执行的程序。  
它**不是** bootloader，也**不是**操作系统，而是一个裸机程序。

## 为什么要写 BootSYS？

在实际开发芯片和开发板的过程中，工程师需要验证 CPU、SoC 或设备驱动的能力，以及设备本身是否工作正常。  
复杂的操作系统有时会干扰判断；这时可以用 BootSYS 较轻松地验证 CPU 的某些能力或设备驱动是否正常。

## 我们支持哪些硬件？

项目启动时验证的是平头哥 C908 与 K230 板卡。计划中将支持其他 RISC-V 芯片以及 ARM 体系的开发板。

## 多个硬件可以验证吗？

开发时一般是单设备、单 CPU 验证。若要验证两个设备同时工作会不会互相干扰，这时可能需要同时启动多个核，每个核运行一个例程。  
这类验证远不及操作系统复杂，只能做简单验证，辅助判断。

---

工具链：`riscv64-*-gcc`（详见 [docs/ENV_SETUP_UBUNTU_CN.md](docs/ENV_SETUP_UBUNTU_CN.md)）。  
文档总索引：[docs/README.md](docs/README.md)。  
若前缀不同：`make CROSS_COMPILE=riscv64-linux-gnu-`。

## 编译

```bash
make -C <NN-项目名>          # 例：make -C 01-base-boot
# → out/<NN-项目名>/bootsys.bin   @ 0x80200000
```

部分项目可选 `LOAD=ddr`（需 DRAM 已初始化），见对应目录说明。

## 项目与文档

| 目录 | 含义 | 文档 |
|------|------|------|
| **[01-base-boot](01-base-boot/)** | 最小启动 + UART（仅 CPU0） | [BASE_BOOT_CN.md](01-base-boot/docs/BASE_BOOT_CN.md) |
| **[02-multi-cpu](02-multi-cpu/)** | 多核：CPU1、shmem、菜单、tick | [MULTI_CPU_CN.md](02-multi-cpu/docs/MULTI_CPU_CN.md) · [K230_DUAL_CORE_CN.md](02-multi-cpu/docs/K230_DUAL_CORE_CN.md) |
| **[03-exception](03-exception/)** | 异常实验：access fault / 除零探测 / 非法指令 | [EXCEPTION_CN.md](03-exception/docs/EXCEPTION_CN.md) |
| **[04-privilege](04-privilege/)** | 特权级：M / S / U 切换与权限验证 | [PRIVILEGE_CN.md](04-privilege/docs/PRIVILEGE_CN.md)（含 ecall/`mcause` 说明） |
| **[05-pmp](05-pmp/)** | PMP：设保护区测 M/S/U，取消后再测 | [PMP_CN.md](05-pmp/docs/PMP_CN.md) |
| **[06-tlb](06-tlb/)** | Sv39 页表 / TLB：映射、缺页、sfence.vma | [TLB_CN.md](06-tlb/docs/TLB_CN.md) |
| **[07-cache](07-cache/)** | L1 cache tag dump（MCINDEX / MCINS / MCDATA） | [CACHE_CN.md](07-cache/docs/CACHE_CN.md) |
| **[08-timer](08-timer/)** | CLINT 定时器中断 + 串口秒表 | [TIMER_CN.md](08-timer/docs/TIMER_CN.md) |
| **[09-irq](09-irq/)** | UART RX → PLIC 外部中断 | [IRQ_CN.md](09-irq/docs/IRQ_CN.md) |
