# 11 · 双核线程通讯

> 前置：[02 多核](../../02-multi-cpu/docs/MULTI_CPU_CN.md)、[10 线程 + print](../../10-thread/docs/THREAD_CN.md)。  
> 下一课：[12 原子互锁](../../12-atomic/docs/ATOMIC_CN.md)（以本课为底，mbox → atom）。

---

## 1. 要证明什么

| 菜单 | 现象 | 含义 |
|------|------|------|
| `t` | `[cpu1] probe` 行出现 | print 环仍可用 |
| `g` | `send ping-N` 与 `recv ping-N` 对齐，`rx_ok==N` | 写 payload → ready → **flush**；读侧 **pull** 后取走 |
| `b` | `rx_ok < N` 或 `rx_bad` 上升，或乱序/空 | 跳过 cohere 时 cache 行不可靠 |
| `p` | `ping Qn` / `pong An` 成对 | 双向 mailbox |
| `c` | `cpu1 raw` 仍是旧值 A，flush 后才见 B | **硬件不会**自动同步两核 L1；要靠软件 flush/invalidate |

本课**不**引入 `lr`/`sc`/自旋锁；单写者约定 + 标志位即可。

### 菜单 `c` 在测什么

很多人以为「cache 一致性是硬件做的」。在 **单集群、有侦听/目录协议** 的系统上，普通 `sd` 后对端 `ld` 往往能直接看见新值。  
K230 little/big **各有 L1、对本课 SRAM 不跨核侦听**。`c` 故意：

1. 两核都把 `A` 装进各自 L1  
2. CPU0 写成 `B` 但 **不** `flush` 数据行  
3. CPU1 **不** invalidate，直接再 `ld` → 若仍读到 `A`，说明硬件没同步  
4. CPU0 `flush` 后 CPU1 invalidate 再读 → 应看到 `B`（软件 cohere）

实现：`src/cache_probe.c`；握手行与数据行分开，只用行级 `invalidate`，避免误把数据行刷掉。

---

## 2. 协议（GOOD）

```text
发送方:
  slot.ready = 0
  写 seq/len/data
  fence
  slot.ready = 1
  fence
  w++
  thead_flush_range(mbox)     ← 推到内存

接收方:
  thead_flush_caches()        ← 丢掉旧 L1
  若 r==w → 空
  若 !ready → 半包/未写完，记 bad，让出
  拷走 data，ready=0，r++，flush
```

BAD 模式把 flush/pull 关掉，用来对照。

---

## 3. 和 10 / 12 的分界

- **10**：同核切换；print 环是「共享缓冲」预习  
- **11**：跨核 **消息语义**（完整送达 / 等待）  
- **12**：同一 CPU0 上两线程改同一计数器 → 丢更新 / 锁 / AMO（不用大小核验收 AMO）

---

## 4. 关键文件

| 文件 | 作用 |
|------|------|
| `src/mbox.c` | send/recv + cohere 开关 |
| `src/cache_probe.c` | 菜单 `c`：HW L1 是否一致 |
| `src/worker.c` | sender / receiver / ping-pong |
| `src/print.c` | 统一打印（CPU0 drain） |
| `src/hart1_main.c` | CPU1 按 mode 跑 |
| `arch/riscv/cache.S` | flush / `invalidate_range`（探针用） |

---

## 构建

```bash
cd 11-thread-comm && make
# → ../out/11-thread-comm/bootsys.bin
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
