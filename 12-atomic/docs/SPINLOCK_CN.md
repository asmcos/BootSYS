# RISC-V 自旋锁教程（RV64 + A 扩展）

> 隶属 [12-atomic](../README.md)。本课菜单 `l` 的实现在 `src/spinlock.c`。  
> 下文先讲规范里常见的 **`lr`/`sc` 锁**；再说明 **K230 小核/大核上实际用的 `amoswap` 锁**。  
> 锁变量是 `atom_arena.lock`（SRAM `0x80220080`，独占一行；`counter` 在下一行）。  
> 总览见 [ATOMIC_CN.md](ATOMIC_CN.md)。

---

## 0. 板上实情（先看）

菜单 **`t`/`l`/`a` 只在 CPU0 上跑两个抢占线程**（同 L1）。此时：

| 菜单 | 做法 | 期望 |
|------|------|------|
| `t` | plain RMW + **随机忙等** + 偶发 mid-RMW yield | `got < 2N`（lost 不必很大） |
| `l` | **`amoswap` 自旋锁** + 临界区 plain 加；等待时 **yield** | `got == 2N` |
| `a` | `amoadd.d` | `got == 2N` |

K230 **小核↔大核** 对本课 SRAM **不连贯**（11 菜单 `c`）。跨核关 D$ 再跑 AMO 还会挂——所以本课不用双核验收锁/AMO。

`spinlock.c` 实装是 **`amoswap.d.aq` / `.rl`**，自旋失败路径里调用 `sched_yield()`（单核必需）。教科书 `lr`/`sc` 形式见下文。

进度打印带 `ctr=`（共享计数）与 `sw=`（线程切换次数）。

---

## 1. 教科书：`lr`/`sc` 锁（概念）

```asm
    .section .data
    .align 3                    # 8 字节对齐，lr.d/sc.d 要求
lock_var:
    .dword 0                    # 0 = 空闲，1 = 已锁定

    .text
    .globl spin_lock
    .type spin_lock, @function
spin_lock:
    # 参数：a0 = &lock_var
    li      t0, 1               # t0 = 1，准备写入锁的值
retry:
    lr.d.aq t1, (a0)            # 原子加载保留，带 acquire
    bnez    t1, retry           # 锁非 0：别人正持有，自旋
    sc.d.rl t2, t0, (a0)        # 条件存储：尝试写 1，带 release
    bnez    t2, retry           # sc 失败（t2 != 0）必须重试
    ret
    .size spin_lock, .-spin_lock

    .globl spin_unlock
    .type spin_unlock, @function
spin_unlock:
    # 参数：a0 = &lock_var
    amoswap.d.rl zero, zero, (a0)  # *lock = 0，带 release
    ret
    .size spin_unlock, .-spin_unlock
```

与本课 `spinlock.c` 的对应：

| 纯汇编 | `spinlock.c` |
|--------|----------------|
| `a0` = 锁地址 | 约束 `"r"(lock)` |
| `t1` / `t2` | 输出操作数 `tmp` / `st` |
| `retry` 标签 | 局部标签 `1:` / `1b` |

---

## 2. 逐行说明

### 2.1 数据段

```asm
    .section .data
    .align 3
lock_var:
    .dword 0
```

- `.align 3`：按 \(2^3 = 8\) 字节对齐。`lr.d` / `sc.d` 访问 8 字节，未对齐会触发异常。
- `lock_var`：0 = 空闲，非 0 = 已锁定。

### 2.2 `spin_lock`

```asm
    li      t0, 1
```

伪指令，展开为 `addi t0, zero, 1`。常数 1 供稍后 `sc.d` 写入。

```asm
retry:
    lr.d.aq t1, (a0)
```

- `lr.d`：Load-Reserved Doubleword，从 `(a0)` 读 8 字节到 `t1`，并**保留**该地址（保留集）。
- `.aq`（Acquire）：这条加载**之后**的访存，不得被重排到它之前。读到锁可用后，临界区里的读写不会提前执行。

```asm
    bnez    t1, retry
```

`t1 != 0` → 别人正持有，自旋重试。

```asm
    sc.d.rl t2, t0, (a0)
```

- 语法：`sc.d rd, rs2, (rs1)`  
  - `rd = t2`：成功写回则 0，失败非 0，且失败时**不写内存**  
  - `rs2 = t0`：要写的值（1）  
  - `(rs1) = (a0)`：目标地址
- 仅当自上次 `lr` 以来保留集仍有效（他核未改该行）时才写入。
- `.rl`（Release）：这条存储**之前**的访存，不得被重排到它之后。

```asm
    bnez    t2, retry
```

`sc` 失败必须重新 `lr`。**不可省略**，否则会误以为已拿到锁。

```asm
    ret
```

拿到锁，进入临界区。

### 2.3 `spin_unlock`

```asm
    amoswap.d.rl zero, zero, (a0)
```

- `amoswap.d rd, rs2, (rs1)`：把 `rs2` 与 `(rs1)` **原子交换**，旧值进 `rd`。
- 这里 `rd = zero`、`rs2 = zero`：把 0 写入锁，旧值丢弃。
- `.rl`：临界区内的写，在锁释放前对其他核可见。

```asm
    ret
```

---

## 3. 关键机制

### 3.1 为什么要 `lr` / `sc` 配对？

两核若同时 `lr` 到 0，都会以为可以写；但只有一个 `sc` 成功，另一个失败并重试。  
这是硬件提供的「检查并设置」，不必靠总线锁。

### 3.2 `.aq` / `.rl`

| 后缀 | 含义 | 作用 |
|------|------|------|
| `.aq` | Acquire | 后续访存不能重排到该指令之前 |
| `.rl` | Release | 前面访存不能重排到该指令之后 |

- 抢锁：`lr.d.aq` — 看见空闲后，临界区操作不提前  
- 写锁：`sc.d.rl` — 写锁前的操作已提交  
- 放锁：`amoswap.d.rl` — 临界区写在释放前对他核可见  

本课不展开完整 RVWMO；上面足够理解菜单 `l`。

### 3.3 为什么解锁用 `amoswap` 而不是 `sd`？

普通 `sd` 没有 `.rl` 后缀。若用 `sd`，通常还要加 `fence`，更长。  
`amoswap.d.rl` 一条指令完成**原子写 0** + **Release**。

### 3.4 为什么 `sc` 失败要重试？

他核改了锁地址、中断、cache 行被探听等，都会让保留集失效。  
失败后必须重新 `lr`，读最新状态再试。

---

## 4. 使用示例

### 纯汇编

```asm
    .section .text
    .globl demo
demo:
    la      a0, lock_var
    call    spin_lock
    # ---- 临界区：可安全访问共享数据 ----
    call    spin_unlock
    ret
```

### 本课 C 侧（菜单 `l`）

```c
spin_lock(&a->lock);
atom_add_plain(&a->counter);   /* 临界区里仍是普通加 */
spin_unlock(&a->lock);
```

---

## 5. 注意事项

1. **对齐**：锁必须 8 字节对齐（本课 `uint64_t lock` 在 arena 起始，满足）。
2. **RV32**：改用 `lr.w` / `sc.w` / `amoswap.w`。
3. **公平性**：此实现不保证公平，可能饥饿；生产系统常用 ticket / MCS。
4. **中断**：可抢占内核里自旋锁常需关中断，否则可能自死锁。本课两核在 M 态裸跑，菜单 `l` 不开中断抢占。
5. **A 扩展**：无 A 则指令非法；本课 `-march=rv64imac`，并用 03 风格 trap 兜底。

---

返回：[ATOMIC_CN.md](ATOMIC_CN.md) · [本目录 README](../README.md)
