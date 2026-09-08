# 05-pmp

设一块保护区，在 **M / S / U** 各测读写；再取消保护，同样再测一遍。

启动时 **PMP: on**。取消保护不能把 PMP 表清空（S/U 会丢代码/UART），只是去掉对这块的拒绝。

## 编译

```bash
cd 05-pmp && make
# → ../out/05-pmp/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] 1/2     M 读/写：成功
[M] s → [S] 1/2     S 读/写：trap，仍回 S
[S] u → [U] 1/2     U 读/写：trap，仍回 U
[U] m → [M] c       取消保护
再进 S/U 按 1/2     读/写成功
```

`e` 可再打开保护（仅 M）。

| 键 | 作用 |
|----|------|
| `1` / `2` | 读 / 写保护区 |
| `e` / `c` | 设置 / 取消（仅 M） |
| `m` / `s` / `u` | 切模式 |
| `h` | 帮助 |

- PMP on：M 读写 OK；S/U trap（load=`5` / store=`7`）
- PMP off：M/S/U 都 OK

## 文档

- [docs/PMP_CN.md](docs/PMP_CN.md)
- 前置：[04-privilege](../04-privilege/docs/PRIVILEGE_CN.md)
- 仓库：[文档索引](../docs/README.md) · [首页](../README.md)
