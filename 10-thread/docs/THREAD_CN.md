# 10 · 双核定时器抢占线程

> 前置：[08 定时器](../../08-timer/docs/TIMER_CN.md)、[02 多核](../../02-multi-cpu/docs/MULTI_CPU_CN.md)。  
> 下一课：[11 双核线程通讯](../../11-thread-comm/docs/THREAD_COMM_CN.md) → [12 原子](../../12-atomic/docs/ATOMIC_CN.md)。

本课证明：小核（CPU0）和大核（CPU1）都可以在 **各自的时钟中断** 里切换本核上的软件线程。

---

## 1. 要证明什么

| 现象 | 含义 |
|------|------|
| 按 `p` 后出现 `[cpu0] T0` / `T1` | CPU0 上两线程被 `mtime` 打断轮转 |
| 同时出现 `[cpu1] T0` / `T1` | CPU1 已解复位，且用 **自己的** `mtimecmp[1]` 调度 |
| 线程函数里 **没有** `yield` | 切换来自 MTIP，不是协作 |

---

## 2. 设计

### 2.1 每 hart 一份调度器

```text
g_sched[0]  — CPU0：threads[0..1] + idle_frame
g_sched[1]  — CPU1：threads[0..1] + idle_frame
```

- `mtime`：全局一份  
- `mtimecmp[hart]`：每核一份（到点只打扰该核）  
- 本课 **不做** 线程从 CPU0 迁到 CPU1  

### 2.2 中断里切换

```text
mtime ≥ mtimecmp[hart]
  → trap_entry 把当前 GPRs 压到当前栈上的 trap_frame
  → sched_on_timer：
       推高本核 mtimecmp
       把 trap_frame + mepc 存进当前 TCB（或 idle）
       选下一个 READY
       把下一线程的 frame 写回栈上的 trap_frame，csrw mepc
  → trap_entry 按 frame 恢复（含 sp），mret
```

`trap_entry` 恢复时 **最后加载 `sp`**，这样可以把「另一线程的完整上下文」写进当前中断帧再返回。

### 2.3 启动顺序

```text
CPU0 main
  → 菜单 p
  → flush 镜像；cpu1_boot(_hart1_entry)
  → 本核 create 两个 worker；sched_start()（开 MTIE/MIE）
  → idle：wfi（第一次 MTIP 会切到 worker）

CPU1 _hart1_entry
  → hart1_main
  → create 两个 worker；sched_start()
  → idle：wfi
```

---

## 3. 关键文件

| 文件 | 作用 |
|------|------|
| `src/thread.c` | 每 hart 队列、`sched_on_timer` |
| `arch/riscv/trap_entry.S` | 进/出陷阱；按 frame 恢复 `sp` |
| `src/trap.c` | MTIP → `sched_on_timer` |
| `src/hart1_main.c` | CPU1：建线程 + 开定时器 |
| `src/main.c` | CPU0：解复位 CPU1 + 菜单 |
| `boards/k230/cpu1.c` | 与 02/12 相同的解复位 |

---

## 4. 和旧版协作 `yield` 的关系

早期 10 只有 CPU0 + `thread_yield`。现已改成 **双核定时器抢占**。  
协作切换不再作为主路径；需要通讯/原子时见 11、12。

---

## 5. 常见误区

1. **「两个 CPU 共用一个 mtimecmp」**  
   否。比较寄存器按 hart 分开；本课每核写自己的。

2. **「有 `[cpu1]` 打印就是线程迁过去了」**  
   否。CPU1 跑的是 **自己创建的** 线程。

3. **「ISR 里可以随便 `uart_puts`」**  
   本课打印在线程上下文，并短暂关 `MIE`，避免重入；ISR 本身只做调度。

---

## 构建

```bash
cd 10-thread && make
# → ../out/10-thread/bootsys.bin
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
