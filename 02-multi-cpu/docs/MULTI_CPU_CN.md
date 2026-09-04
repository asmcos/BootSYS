# 02 · 多核启动（阶进说明）

> 前置：已跑通 **01-base-boot**（CPU0 + 串口）。  
> 本课代码在 `02-multi-cpu/`，是**完整固件**；此处只写相对 01 多出来的部分。

## 在 01 之上多了什么

| 01 | 02 新增 |
|----|---------|
| `_start` → `main` → UART | `_hart1_entry` → `hart1_main` |
| 单栈 `_sp` | CPU1 栈 `_sp1`、shmem @ `0x80220000` |
| — | RMU：`rstvec` + `CPU1_RST_CTL` 解复位 |
| — | CLINT MSIP / IPI 探测 |
| 回显 | 菜单：`s/1/2/3/i/c/q` + `[cpu1] tick` |

## 建议学习顺序

1. 对照 01 的 `start.S`，看 02 的 `_hart1_entry` 多做了哪些事  
2. 读 `boards/k230/cpu1.c` 的复位脉冲  
3. 读 `src/shmem.c` 软件协议（硬件没有「是否在 WFI」位）  
4. 细节与寄存器表：同目录 [K230_DUAL_CORE_CN.md](K230_DUAL_CORE_CN.md)

## 构建

```bash
cd 02-multi-cpu && make
```
