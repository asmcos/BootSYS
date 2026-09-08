# 08 · CLINT 定时器中断 / 秒表

> 前置：03 trap。本课第一次走 **异步 trap**（中断），不是缺页或 ecall。

## 和同步异常的差别

| | 同步（03/04 ecall） | 本课定时器 |
|--|---------------------|------------|
| `mcause[63]` | 0 | **1** |
| 编号 | 8/9/11… | **7**（Machine timer） |
| `mepc` | 那条故障指令 | **被打断的下一条要执行的指令** |
| `mret` | 软件常 `mepc+=2/4` 跳过 ecall | **不要改 mepc**，否则会丢一条指令 |

`mtime` 是 SoC 实时计数（K230 设备树 **27 MHz**），不是 `mcycle`（CPU 周期）。CPU1 上 `mcycle` 可能不走；本课只跑 CPU0。

## 硬件

CLINT `0xf04000000`（与 02 MSIP 同一块）：

- `mtime` @ `+0xbff8`
- `mtimecmp[0]` @ `+0x4000`

K230 标注 `clint,has-no-64bit-mmio`：64 位数拆成两次 32-bit。写比较器顺序：低半 `0xffffffff` → 高半 → 低半，避免撕裂后立刻再中断。

上电 `mtimecmp` 常为 0，而 `mtime` 已经在走 → **MTIP 一上电就挂起**。`timer_init` 先把比较器写到 `~0`，再开 `mie.MTIE`。

清 pending：**不能写 mip**。只能让 `mtimecmp` 再次大于 `mtime`（本课 `g_cmp += period`）。

## 期望

| 键 | 期望 |
|----|------|
| `s` | 约 0.1s 跳一位；显示 `run` |
| `p` | 数字停 |
| `r` | `00:00.0` `stop` |

若完全不跳：先确认 `mtime` 在递增。不递增就查 CLINT 偏移，不要改成 `mcycle` 轮询（那就不再是中断实验）。

若误进「UNEXPECTED TRAP」：应是 `irq?=yes`、`code=7`。若变成 illegal，多半是 CSR/`mtvec` 没设好。

周期按 27 MHz / 10 Hz。若走时明显快慢，用实测 `mtime` 斜率改 `CLINT_TIMEBASE_HZ`。

## 构建

```bash
cd 08-timer && make
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
