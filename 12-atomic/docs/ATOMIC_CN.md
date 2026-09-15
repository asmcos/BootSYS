# 12 · 原子互锁（CPU0 × 2 线程）

> **先读本文，再翻源码。**  
> 课序：**10 抢占线程** → **11 mailbox / cache 探针** → **本课**。  
> 代码底：[11-thread-comm](../../11-thread-comm/)（去 mbox，`+128` → atom arena；保留 print ring、调度器）。  
> 本课第一次系统用 RISC-V **A 扩展**（`amoswap` / `amoadd` / `lr`/`sc`）。

---

## 1. 本课要证明什么

| 菜单 | 谁跑 | 做法 | 期望 |
|------|------|------|------|
| `t` | **CPU0 两线程** | plain `ld` → 随机忙等 → `sd` | `counter < 2N`（丢更新） |
| `l` | 同上 | `amoswap` 锁 + 临界区 `+1` | `counter == 2N` |
| `a` | 同上 | `amoadd.d` | `counter == 2N` |
| `r` | CPU0 + **CPU1** | CPU0 `lr` → CPU1 `sd` → CPU0 `sc` | `sc` 失败 |
| `p` | CPU1 → print ring | 探针 | 有 `[cpu1]` 行 |

N 默认 **2000**（每线程，菜单 `n`）。`t`/`l`/`a` **不 kick CPU1**。

**为何不用大小核做 `t`/`l`/`a`：** K230 小核/大核对这块 SRAM **无硬件 cache 一致性**（见 11 菜单 `c`）。开着 D$ 跨核 AMO 不可靠；关 D$ 再跑 AMO 还会挂。同 CPU0、同 L1 才能按教科书验收锁与 `amoadd`。

---

## 2. 无锁为何会丢（单核两线程）

只有 **在 `ld` 与 `sd` 之间被切走** 才会丢。完整做完一次 `+1` 再 yield，不会丢。

```text
T0:  ld  counter → 7
     （timer / yield → T1）
T1:  ld → 7 ; sd → 8
     （切回 T0）
T0:  sd → 8          ← 盖掉 T1
```

实现要点（`src/atomic_count.c`）：

- **随机忙等**（约 2k～62k 空循环）+ 约 1/32 概率在 `ld`/`sd` 之间 `sched_yield`
- **禁止**在每次 `ld`/`sd` 之间都 `wfi`/yield——否则对端可连加很多下，再被陈旧 `sd` 整段盖掉，`lost ≈ N`（看起来像「全丢了」）
- 进度行带 `ctr=`（当前 counter）与 `sw=`（调度切换次数）

---

## 3. 锁与 AMO（菜单 `l` / `a`）

- **`l`**：`amoswap.d.aq` 抢锁；临界区普通 `+1`；`amoswap.d.rl` 放锁。自旋等待时 **`sched_yield`**，避免单核上持锁线程饿死。详解见 [SPINLOCK_CN.md](SPINLOCK_CN.md)。
- **`a`**：`amoadd.d`，硬件一次完成加。
- **`r`**：仍要第二 hart 写 `reserved`，才能清掉本核 `lr` 保留（规范不要求本核普通 store 清监视器）。

---

## 4. 内存布局

```text
0x80220000  ctrl     CPU1：alive / done / go_seen / …
0x80220040  cmd      CPU0：go / loops / mode / poke（仅 r/p）
0x80220080  atom     lock | counter | reserved（各占 64B 行）
0x80220800  print    两核 enqueue，仅 CPU0 drain
```

`t`/`l`/`a` 主要碰 atom + print；ctrl/cmd 给 `r`/`p` 用。

---

## 5. `t`/`l`/`a` 时序（CPU0）

```text
atom_reset + atom_rng_seed(mtime)
sched_init_hart
sched_create(worker_add, 0)
sched_create(worker_add, 1)
sched_start
  → T0 / T1 抢占交错累加
  → 进度: [T?] progress …/… ctr=… sw=…
sched_stop
打印 per-thread local_adds、counter、lost、switches
```

| 键 | 判定 |
|----|------|
| `t` | `lost > 0` → PASS；偶发不丢再试 |
| `l` / `a` | `counter == 2N` → PASS |
| `r` | `sc failed` → PASS |

---

## 6. 踩坑（板上实证）

1. **大小核当 SMP 用 AMO** → counter 错或挂死；改 CPU0×2 线程。  
2. **每次 RMW 中间都让出** → `lost ≈ N`；改成随机短窗。  
3. **自旋不等待 yield** → 持锁线程被切走后另一线程空转；`spin_lock` 里要 `sched_yield`。  
4. **Logo 走 print ring** → UTF-8 框线被截断；banner 用 `uart_puts`。  
5. **菜单 `r` 用本核 store 夹 `lr`/`sc`** → 在 C908 上常仍成功；必须他核写。

---

## 7. 源码地图

| 文件 | 作用 |
|------|------|
| `include/atomic.h` | arena 布局 |
| `src/spinlock.c` | `amoswap` TAS + yield |
| `src/atomic_count.c` | plain / lock / amo、随机延时、`r` |
| `src/worker.c` | CPU0 两线程 worker |
| `src/thread.c` | 调度、`sched_switches` / `sched_yield` |
| `src/main.c` | 菜单、`run_two_threads` |
| `src/hart1_main.c` | 仅 `r` / `p` |
| `src/print.c` | print ring（CPU0 drain） |

---

## 构建

```bash
cd 12-atomic && make
# → ../out/12-atomic/bootsys.bin
```

返回：[README](../README.md) · [SPINLOCK_CN.md](SPINLOCK_CN.md) · [文档索引](../../docs/README.md)
