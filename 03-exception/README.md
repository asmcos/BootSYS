# 03-exception

基于 02 的异常 / trap 实验：主动触发 **访问异常**、**除零探测**、非法指令，并打印 CSR 与通用寄存器。

## 编译

```bash
cd 03-exception && make
# → ../out/03-exception/bootsys.bin   @ 0x80200000
```

## 串口菜单

| 键 | 作用 |
|----|------|
| `a` | Load access fault（读坏地址） |
| `s` | Store access fault |
| `z` | 除零：先做硬件 DIV（RISC-V **不 trap**），再 `ebreak` 练 handler |
| `i` | 非法指令 |
| `h` | 帮助 |

异常后打印 `mcause` / `mepc` / `mtval` / `mstatus` 等及部分 GPR，然后 **跳过故障指令并恢复**。

## 文档

- [docs/EXCEPTION_CN.md](docs/EXCEPTION_CN.md) — 原理与关键文件
- 前置：[01](../01-base-boot/docs/BASE_BOOT_CN.md) · [02](../02-multi-cpu/docs/MULTI_CPU_CN.md)
- 仓库：[文档索引](../docs/README.md) · [首页](../README.md)

## 下一课

[`../04-privilege/`](../04-privilege/) — M / S / U 特权级切换与权限验证。
