# 02-multi-cpu

完整多核固件（CPU0 + CPU1）。**在 01-base-boot 能力之上叠加**第二核；本目录代码是**全部业务代码**，文档按阶进阅读。

## 编译

```bash
cd 02-multi-cpu
make
# → ../out/02-multi-cpu/bootsys.bin
```

可选：`make LOAD=ddr`

拷贝到 J-Link 目录：`./cp_bin.sh`（按需改路径）。

## 文档（阶进）

1. 先完成 [`../01-base-boot/docs/BASE_BOOT_CN.md`](../01-base-boot/docs/BASE_BOOT_CN.md)
2. 再读 [`docs/MULTI_CPU_CN.md`](docs/MULTI_CPU_CN.md)（由原双核文档整理）
3. 细节仍见 [`docs/K230_DUAL_CORE_CN.md`](docs/K230_DUAL_CORE_CN.md)

## 目录

```
02-multi-cpu/
├── arch/riscv/     start.S（含 _hart1_entry）、cache.S
├── boards/k230/    UART / CPU1 RMU / CLINT / linker
├── src/            main、banner、shmem、hart1_main
└── docs/
```
