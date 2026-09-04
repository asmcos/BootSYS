# K230：从单 CPU 到多 CPU（含 CPU1 启动原理）

> **范围**：BootSYS 在 Canaan K230 上的裸机双核实验  
> **对应代码**：`02-multi-cpu/`（`arch/riscv/start.S`、`src/main.c`、`boards/k230/cpu1.c`、`src/hart1_main.c`）  
> **前置**：[`01-base-boot`](../../01-base-boot/docs/BASE_BOOT_CN.md)（仅 CPU0 + 串口）  
> **参考**：K230 TRM；Canaan U-Boot `de_reset_big_core` / `boot_baremetal`

本文说明：**为什么上电只有一核在跑**，以及 **CPU0 如何把 CPU1 放到指定地址执行**。  
BootSYS 不是 OS / 不是 OpenSBI；这里讲的是 **SoC 复位控制下的早起双核**，不是 Linux SMP。  
构建：`make BOARD=k230` → `out/k230/bootsys.bin`。

---

## 1. 单 CPU 时实际发生了什么

上电后典型路径：

```
复位 → BOOTROM（CPU0）→ 加载 BootSYS 到 SRAM → 跳到 0x80200000 → _start → main
```

| 项 | K230 上的含义 |
|----|----------------|
| 启动核 | **CPU0**（小核 C908，约 800 MHz） |
| 另一核 | **CPU1**（大核 C908，约 1.6 GHz）上电后 **保持复位** |
| RISC-V 术语 | 一颗可独立取指执行的核常叫 **hart**；此处 CPU0 / CPU1 ≈ hart0 / hart1 |

单核阶段只有 CPU0 有有效 PC 和栈。CPU1 **不会**自动走进同一个 `_start`：它还没被解复位，也没有自己的复位向量。

BootSYS 单核时做的事：

1. `_start`：清寄存器、设栈、拷 `.data`、清 `.bss`、`thead_cpu_init`
2. `main`：UART、shmem、唤醒 CPU1、菜单

这与「镜像里写了多核代码」无关——**镜像被加载 ≠ 所有核都在跑这段镜像**。

---

## 2. 多 CPU 要解决的问题

从单核到双核，本质是补齐四件事：

| # | 问题 | 要求 |
|---|------|------|
| 1 | **谁启动谁** | 必须有一核（CPU0）去解复位另一核 |
| 2 | **从哪开始跑** | 给 CPU1 一个入口 PC（复位向量） |
| 3 | **跑什么** | 入口处的代码 / 栈 / 数据对 CPU1 可见且正确 |
| 4 | **如何共存** | 各核独立栈；共享外设要约定（锁 / 分工） |

RISC-V 上常见的几种「叫醒另一核」路径：

| 方式 | 何时用 | K230 早起裸机 |
|------|--------|----------------|
| **SoC 复位 + rstvec MMIO** | 第二核一直关在复位里 | **就是这条** |
| CLINT `msip` / IPI | 两核都已在跑，发核间中断 | 不解复位，叫醒不了「还在复位里」的核 |
| OpenSBI / `sbi_hart_start` | 已有 SBI，S/U 态 OS | BootSYS 早起没有 SBI |
| 邮箱 / IPC | 两核已跑起来后的通信 | 不是启动手段 |

结论：**K230 把 CPU1 从「死」到「活」，靠的是 CPU0 写系统控制寄存器，不是 SBI，也不是邮箱。**

---

## 3. K230 双核拓扑（不对称启动）

```
                    ┌─────────────────┐
   上电复位 ───────►│ CPU0 (小核)      │──── 跑 BOOTROM / BootSYS
                    │ 始终可运行       │
                    └────────┬────────┘
                             │ 写 rstvec + 脉冲复位
                             ▼
                    ┌─────────────────┐
                    │ CPU1 (大核)      │──── 从 rstvec 取指开始跑
                    │ 默认关在复位里   │
                    └─────────────────┘
```

TRM 要点：

- BOOTROM / 早期固件从 **CPU0** 起来
- **CPU1 的解复位由 CPU0 控制**

因此 BootSYS 当前模型是：

- **CPU0**：完整 `_start` → `main`（主控、UART、启动 CPU1）
- **CPU1**：不走 `_start`，只走专用入口 `_hart1_entry` → `hart1_main`

---

## 4. CPU1 启动原理（寄存器级）

与 Canaan U-Boot 一致，两步：

### 4.1 设置复位向量 `cpu1_hart_rstvec`

| 项 | 值 |
|----|-----|
| 地址 | `0x91102104`（bootctl / `SYSCTL_BOOT` 区域） |
| 写入 | CPU1 解复位后 **第一条指令的地址**（如 `_hart1_entry` ≈ `0x802000c8`） |

含义：CPU1 一出复位，PC 就从这里取指，**不会**自动等于 `0x80200000` 的 `_start`，除非你故意把 rstvec 写成 `_start`。

### 4.2 脉冲 `CPU1_RST_CTL`（写使能语义）

| 项 | 值 |
|----|-----|
| 地址 | `0x9110100c` |

该寄存器带 **高 16 位写使能**：改 bit N 时必须同时置 bit (N+16)。Linux `reset-k230.c` / TRM：

| 操作 | 写入含义 |
|------|----------|
| 断言复位 | `bit0=1` + `bit16=1` → `0x00010001` |
| 解除复位 | `bit0=0` + `bit16=1` → `0x00010000` |
| 清 done | `bit12=1` + `bit28=1` → `0x10001000` |

上电默认 `CPU1_RST_CTL ≈ 0x2001`（大核关在复位里）。  
仅写 `0x10001000` **不会**解复位——这解释了此前 `rst_ctl=0x10002001`（bit0 仍为 1）时看不到 `CPU1`。

另外可能需要先给 CPU1 **上电**（`SYSCTL_PWR` `cpu1_pwr_lpi_ctl` @ `0x91103018`）。

BootSYS：`boards/k230/cpu1.c` 的 `cpu1_boot()` 按 Linux/U-Boot 路径：上电 → 写 rstvec → clear done → assert → deassert+等 done。

### 4.3 时序示意

```
CPU0:  … flush cache …
CPU0:  write rstvec = _hart1_entry
CPU0:  pulse CPU1_RST_CTL
                    ┌──────────────────────────────────────┐
CPU1:  (reset) ──►  │ PC ← rstvec → _hart1_entry → hart1_main │
                    └──────────────────────────────────────┘
```

---

## 5. 为什么解复位前要刷 Cache

CPU0 与 CPU1 **各有自己的 cache**。CPU0 改过的 `.data` / `.bss` / 栈区，可能还只在 CPU0 的 D-cache 里，SRAM 里仍是旧值。

若此时释放 CPU1：

- CPU1 按 **总线/内存** 取指、读数据
- 可能看到 **未写回** 的错误内容 → 跑飞 / 怪 trap

因此 BootSYS 在 `start_cpu1` 之前调用 `thead_flush_caches()`（`arch/riscv/cache.S`，T-Head C908：D-cache clean+invalidate、I-cache invalidate、L2 维护，再 `fence.i` 等）。

原则：**凡是 CPU1 会读的、且由 CPU0 写过的内存，解复位前必须对 CPU1 可见（写回 + 必要的 I-cache 失效）。**

---

## 6. CPU1 入口与资源隔离

### 6.1 为什么不用同一个 `_start`

若 rstvec 指向 `_start`，CPU1 会再次：

- 清 `.bss` → **毁掉 CPU0 正在用的全局状态**
- 使用同一个 `_sp` → **两核抢同一栈** → 必崩

所以 CPU1 使用独立入口 `_hart1_entry`（`arch/riscv/boot.S`）：

1. 轻量 cache/fence 初始化  
2. 设 `mtvec`、`gp`  
3. **`sp = _sp1`**（独立栈）  
4. 调用 `hart1_main`  
5. **不再**拷 `.data`、清 `.bss`

### 6.2 链接脚本中的双栈

`boards/k230/linker.ld`（SRAM `0x80220000` 起 128 KiB）大致为：

| 符号 | 用途 |
|------|------|
| `_sp` | CPU0 栈顶 |
| `_sp1` | CPU1 栈顶（另 4 KiB） |

堆在两栈之下，中间留 guard。

### 6.3 CPU1 心跳（顺序打印）

`hart1_main` 用普通换行打 `[cpu1] tick N`（不要用 minicom 固定行/滚动域）。

**踩过的坑**：若在**第一次打印之前**用 `mcycle` 做延时，而大核上该计数器不递增，会永远空转，表现为「完全没有 cpu1 打印」。  
第一版能看见 tick，是因为 **先打印再延时**。当前：首次延时约 2 秒用**软件循环**（不依赖 `mcycle`），间隔约 10 秒同样用软件循环。

### 6.4 已去掉的分区标题

`── cpu1 ──` / `── cpu 0 ──` 与 ANSI 固定区已移除，避免干扰排查。

---

## 7. BootSYS 中的调用链（实验对照）

```
CPU0 _start
  → main
       → UART / shmem init
       → thead_flush_range / flush
       → cpu1_boot(_hart1_entry)
       → 「cpu0: cpu1 released」
       → 菜单循环

CPU1（被解复位后）
  _hart1_entry
  → hart1_main
       → 循环：[cpu1] tick N ；busy-wait ~10s
```

串口上预期类似：

```
┌ BootSYS 面板 … ╯
── cpu1 ──
(ticks every ~10s appear below)

── cpu 0 ──
cpu0: start cpu1 @ 0x…
cpu0: cpu1 released
[cpu1] tick 0
…（约 10 秒）…
[cpu1] tick 1
```

若只有 cpu0 行、始终没有 `[cpu1] tick`，再查 rstvec / 复位脉冲 / cache / `_hart1_entry`。

---

## 8. 和「Linux 多核 / OpenSBI」的差别

| 阶段 | 启动方式 | 状态 |
|------|----------|------|
| BootSYS 早起（本文） | CPU0 MMIO：rstvec + `CPU1_RST_CTL` | M-mode 裸机实验 |
| U-Boot `boot_baremetal 1 <addr> <size>` | 同上 | 官方同路径 |
| 完整 Linux | 通常 OpenSBI + DT + `sbi_hart_start` / park 协议 | 已有运行时与权限模型 |

可以粗记：

1. **冷启动第二核** → SoC 复位控制（本文）  
2. **热路径上调度/唤醒已运行的 hart** → IPI / SBI  

BootSYS 验证的是第 1 步。

---

## 9. 相关文件一览

| 文件 | 作用 |
|------|------|
| `arch/riscv/start.S` | `_start`（CPU0）、`_hart1_entry`（CPU1） |
| `arch/riscv/cache.S` | `thead_cpu_init` / cache flush / `thead_cache_off` |
| `src/main.c` | CPU0：banner、唤醒 CPU1、菜单 |
| `boards/k230/cpu1.c` | 写 rstvec、脉冲 `CPU1_RST_CTL` |
| `src/hart1_main.c` | CPU1 状态机 + tick |
| `boards/k230/linker.ld` | `_sp` / `_sp1`、shmem @ `0x80220000` |

---

## 10. 一句话总结

**单 CPU**：只有 CPU0 从复位走出并执行 BootSYS。  
**多 CPU（K230）**：CPU0 把 CPU1 的入口写入 `0x91102104`，再脉冲 `0x9110100c` 解复位；CPU1 从该入口用**自己的栈**开始跑。  
**Cache 与独立栈**是双核能稳定跑起来的两个硬条件；SBI / 邮箱是更后面的层次，不是这条早起路径的必要条件。
