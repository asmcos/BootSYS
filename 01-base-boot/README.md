# 01-base-boot

最小系统：**仅 CPU0** 启动 + UART 打印 / 回显。  
这是 BootSYS 的第一课；不含多核。

## 编译

```bash
cd 01-base-boot
make
# → ../out/01-base-boot/bootsys.bin   @ 0x80200000
```

## 文档

- [docs/BASE_BOOT_CN.md](docs/BASE_BOOT_CN.md) — 原理与验收
- [docs/README.md](docs/README.md) — 本目录文件说明
- 仓库：[文档索引](../docs/README.md) · [首页](../README.md)

## 下一课

[`../02-multi-cpu/`](../02-multi-cpu/) — 在本能力上叠加 CPU1。
