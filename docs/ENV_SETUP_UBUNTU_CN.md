# BootSYS 开发环境（Ubuntu）

固件为 **纯 C**，用 RISC-V 交叉 GCC 编译。

## 安装

```bash
sudo apt update
sudo apt install -y build-essential make git
sudo apt install -y gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
# 若前缀是 riscv64-linux-gnu-：
#   make CROSS_COMPILE=riscv64-linux-gnu-
```

## 构建

```bash
cd /path/to/BootSYS
make BOARD=k230
# → out/k230/bootsys.elf / bootsys.bin
./cp_bin.sh   # 可选
```

入口：`0x80200000`（`boards/k230/linker.ld`）。

## 参考

- 双核：[K230_DUAL_CORE_CN.md](K230_DUAL_CORE_CN.md)
