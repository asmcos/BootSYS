# 03 · 异常打印

> 前置：01 串口、02 多核任选。本课聚焦 **M-mode trap**。

## 目标

1. 安装 `mtvec` → `trap_entry`  
2. 同步异常时打印：`mcause`、`mepc`、`mtval`、`mstatus`、`mhartid` 及 GPR  
3. 演示：
   - **Load / Store access fault**
   - **除零**：RISC-V 整数除零 **不产生异常**（商为全 1）；本课再强制 `ebreak` 走 handler  
   - **Illegal instruction**

## 关键文件

| 文件 | 作用 |
|------|------|
| `arch/riscv/trap.S` | 压栈 / 调 C / `mret` |
| `src/trap.c` | 寄存器转储，`mepc += insn_len` 后恢复 |
| `src/main.c` | 菜单触发故障 |

## 构建

```bash
cd 03-exception && make
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)