# 09-irq

串口 **RX 中断**：敲一个键 → UART → PLIC → `mtvec`。`mcause` 最高位=1、编号 **11**（机器外部中断）。  
对比 08：定时器是编号 **7**（本地 CLINT），本课是 PLIC 源 **16**（UART0）。

只在 **M**。ISR 只把字节放进环形缓冲、加计数；主循环再处理菜单。

## 编译

```bash
cd 09-irq && make
# → ../out/09-irq/bootsys.bin   @ 0x80200000
```

## 操作

```text
启动已是 IRQ 模式
敲任意键     count 应加 1，last 显示该字符
i            开 UART RX 中断
p            关中断，改回轮询；再敲键 count 不再加
h            帮助
```

## 文档

- [docs/IRQ_CN.md](docs/IRQ_CN.md)
- 前置：[08-timer](../08-timer/docs/TIMER_CN.md)
- 仓库：[文档索引](../docs/README.md)
