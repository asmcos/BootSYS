# 环境（Ubuntu）

固件为 **纯 C + ASM**，使用 RISC-V 交叉 GCC。

```bash
sudo apt install -y build-essential make git
sudo apt install -y gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

## 编译示例

```bash
cd 01-base-boot && make CROSS_COMPILE=riscv64-linux-gnu-
cd ../02-multi-cpu && make CROSS_COMPILE=riscv64-linux-gnu-
cd ../03-exception && make CROSS_COMPILE=riscv64-linux-gnu-
```

默认前缀也可能是 `riscv64-unknown-linux-gnu-`（视本机工具链而定）。  
产物在仓库根下 `out/<项目>/bootsys.bin`，加载地址一般为 `0x80200000`。

返回：[文档索引](README.md) · [仓库首页](../README.md)
