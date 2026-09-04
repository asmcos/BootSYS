# BootSYS

RISC-V 底层验证固件（**不是** OS，也**不是** bootloader）。纯 C + ASM，按目录递进。

工具链：`riscv64-*-gcc`（详见 [docs/ENV_SETUP_UBUNTU_CN.md](docs/ENV_SETUP_UBUNTU_CN.md)）。  
文档总索引：[docs/README.md](docs/README.md)。  
若前缀不同：`make CROSS_COMPILE=riscv64-linux-gnu-`。

## 编译项目

进入对应目录执行 `make`：

```bash
make -C 01-base-boot
make -C 02-multi-cpu
make -C 03-exception
```

可选：`make -C 02-multi-cpu LOAD=ddr`（需 DRAM 已初始化）。

## 编译结果

| 项目 | 产物路径 | 加载地址 |
|------|----------|----------|
| 01 | `out/01-base-boot/bootsys.bin` | `0x80200000` |
| 02 | `out/02-multi-cpu/bootsys.bin` | `0x80200000` |
| 03 | `out/03-exception/bootsys.bin` | `0x80200000` |

本机拷贝（脚本不入库）：

```bash
./cp_bin.sh 01-base-boot
./cp_bin.sh 02-multi-cpu
./cp_bin.sh 03-exception
```

## 项目与文档

| 目录 | 含义 | 文档 |
|------|------|------|
| **[01-base-boot](01-base-boot/)** | 最小启动 + UART（仅 CPU0） | [BASE_BOOT_CN.md](01-base-boot/docs/BASE_BOOT_CN.md) |
| **[02-multi-cpu](02-multi-cpu/)** | 多核：CPU1、shmem、菜单、tick | [MULTI_CPU_CN.md](02-multi-cpu/docs/MULTI_CPU_CN.md) · [K230_DUAL_CORE_CN.md](02-multi-cpu/docs/K230_DUAL_CORE_CN.md) |
| **[03-exception](03-exception/)** | 异常实验：access fault / 除零探测 / 非法指令 | [EXCEPTION_CN.md](03-exception/docs/EXCEPTION_CN.md) |
