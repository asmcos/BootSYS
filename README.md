# BootSYS

RISC-V 底层验证（非 OS / 非 bootloader）。按目录递进：

| 目录 | 内容 | 编译 |
|------|------|------|
| [`01-base-boot/`](01-base-boot/) | 最小启动 + 串口（仅 CPU0） | `cd 01-base-boot && make` |
| [`02-multi-cpu/`](02-multi-cpu/) | 完整多核固件（01 之上叠加） | `cd 02-multi-cpu && make` |

产物分别在 `out/01-base-boot/`、`out/02-multi-cpu/`。

环境简述见 [`docs/ENV_SETUP_UBUNTU_CN.md`](docs/ENV_SETUP_UBUNTU_CN.md)。  
交叉编译器：`riscv64-*-gcc`，或 `make CROSS_COMPILE=riscv64-linux-gnu-`。
