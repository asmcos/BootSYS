# 02-multi-cpu

完整多核固件（CPU0 + CPU1）。**在 01-base-boot 能力之上叠加**第二核；本目录代码是**全部业务代码**，文档按阶进阅读。

## 编译

```bash
cd 02-multi-cpu
make
# → ../out/02-multi-cpu/bootsys.bin   @ 0x80200000
```

可选：`make LOAD=ddr`（需 DRAM 已初始化）。

## 文档（阶进）

1. 先完成 [01 · BASE_BOOT_CN.md](../01-base-boot/docs/BASE_BOOT_CN.md)
2. 再读 [MULTI_CPU_CN.md](docs/MULTI_CPU_CN.md)（相对 01 多出来的部分）
3. 细节与寄存器：[K230_DUAL_CORE_CN.md](docs/K230_DUAL_CORE_CN.md)
4. 仓库：[文档索引](../docs/README.md) · [首页](../README.md)

## 目录

```
02-multi-cpu/
├── arch/riscv/     start.S（含 _hart1_entry）、cache.S
├── boards/k230/    UART / CPU1 RMU / CLINT / linker
├── src/            main、banner、shmem、hart1_main
└── docs/
```

## 下一课

[`../03-exception/`](../03-exception/) — M-mode trap / 异常打印。
