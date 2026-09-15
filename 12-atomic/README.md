# 12-atomic

在 **[11-thread-comm](../11-thread-comm/)** 上：mbox → atom arena。

| 菜单 | 跑法 |
|------|------|
| `t` / `l` / `a` | **CPU0 × 2 抢占线程**（同 L1；D$ 开） |
| `r` / `p` | 仍可拉起 CPU1 |

大小核无硬件一致性 → 见 11 菜单 `c`；不在双核上验收 AMO/锁。

**建议：先读文档，再看源码。**

## 文档

- **[docs/ATOMIC_CN.md](docs/ATOMIC_CN.md)** — 机制、踩坑、代码地图
- **[docs/SPINLOCK_CN.md](docs/SPINLOCK_CN.md)** — `amoswap` 自旋锁
- 前置：[11-thread-comm](../11-thread-comm/) · [10-thread](../10-thread/)

## 编译

```bash
cd 12-atomic && make
# → ../out/12-atomic/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] t    无锁累加（2 线程；ctr/sw；期望 lost>0）
[M] l    amoswap 锁（期望 counter==2N）
[M] a    amoadd.d（期望 ==2N）
[M] r    他核 store 夹 lr/sc（sc 应失败）
[M] p    print probe（CPU1）
[M] n    设置 N（每线程，默认 2000）
[M] s    状态
[M] h    帮助
```

进度示例：`[T0] progress 100/2000 ctr=187 sw=42`
