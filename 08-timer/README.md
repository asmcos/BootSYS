# 08-timer

CLINT **机器定时器中断** + 串口秒表。只在 **M**、只跑 CPU0。

`mtime >= mtimecmp` → `mip.MTIP` → `mtvec`。`mcause` 最高位=1、编号 **7**。  
推高 `mtimecmp` 才能清 MTIP；`mret` **不会** `pc+=4`（那是同步异常软件跳过）。

## 编译

```bash
cd 08-timer && make
# → ../out/08-timer/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] s      开：mtimecmp=mtime+周期，mie.MTIE + mstatus.MIE
[M] p      停：清 mie.MTIE
[M] r      停并清零
```

一行刷新 `MM:SS.d`（内部 10 Hz）。ISR 只加计数、重装比较器，不打串口。

## 文档

- [docs/TIMER_CN.md](docs/TIMER_CN.md)
- 前置：[03-exception](../03-exception/docs/EXCEPTION_CN.md) · [07-cache](../07-cache/)
- 仓库：[文档索引](../docs/README.md)
