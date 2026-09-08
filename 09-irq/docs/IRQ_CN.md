# 09 · UART RX / PLIC 外部中断

> 前置：08 本地定时器中断。本课：**敲键产生外部中断**，不是轮询 `LSR`。

## 和 08 的差别

| | 08-timer | 09-irq |
|--|----------|--------|
| 来源 | CLINT `mtime` | UART0 收满一字节 |
| `mcause` | `1<<63 \| 7` | `1<<63 \| 11` |
| 清 pending | 推高 `mtimecmp` | 读空 RBR + PLIC complete |

路径：`IER.ERBFI` → PLIC 源 16（level）→ hart0 **M** 上下文 → `mie.MEIE` → `mtvec`。

PLIC `@ 0xf00000000`（`thead,c900-plic`）。claim/complete 用 context 0。level 中断必须把 FIFO/RBR 抽干，否则会连打。

`mret` **不改 mepc**（和 08 一样）。ISR **不打串口**，避免和 TX 轮询搅在一起。

## 期望

| 操作 | 期望 |
|------|------|
| IRQ 模式敲键 | `count` 增加，`last` 为该字符 |
| `p` 后再敲 | 键仍可用（轮询），`count` 不再增加 |
| `i` 再敲 | `count` 又开始增加 |
| 误进 UNEXPECTED | 应是 `irq?=yes` `code=11`，不是 7 |

若完全没有中断：PLIC 使能/优先级/threshold、UART `IER`、`mie.MEIE`。源号按设备树 UART0=**16**。

## 构建

```bash
cd 09-irq && make
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
