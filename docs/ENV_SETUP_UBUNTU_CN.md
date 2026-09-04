# 环境（Ubuntu）

固件为 **纯 C**，用 RISC-V 交叉 GCC。

```bash
sudo apt install -y build-essential make git
sudo apt install -y gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

```bash
cd 01-base-boot && make CROSS_COMPILE=riscv64-linux-gnu-
cd ../02-multi-cpu && make CROSS_COMPILE=riscv64-linux-gnu-
```

默认前缀也可为 `riscv64-unknown-linux-gnu-`（视本机工具链而定）。
