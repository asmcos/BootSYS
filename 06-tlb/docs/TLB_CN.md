# 06 · Sv39 / TLB

> 前置：04 特权级、05 PMP。本课：**开页表，在 S 里看映射、缺页、以及 TLB 要不要 `sfence.vma`。**

## 为什么必须进 S

M-mode **不查 `satp`**（除非设 `MPRV`）。所以 `o` 会：写三级页表 → `satp=Sv39` → `mret` 进 S。  
异常仍在 M 处理（`medeleg=0`），打印后 `mret` 回 S。

## 映射

| VA | 含义 |
|----|------|
| `LOAD_BASE` 起 2MiB | 代码（恒等；SRAM 上是 `0x80200000`） |
| `SHMEM_BASE` 起 2MiB | RAM + 栈（SRAM 上与上一行同一页） |
| `0x91400000` 起 2MiB | UART（恒等） |
| `0x1000` | 探针页 → 物理 `tlb_page`（魔数 `0x544c4231`） |
| `0x4000` | **不映射**，用来制造 load page fault |

`mxstatus.MAEE`（玄铁扩展 PTE）在开 MMU 时关掉，避免和标准 Sv39 编码打架。

## 期望

| 键 | 期望 |
|----|------|
| `1` | 读到魔数，无 trap |
| `2` | `mcause=13`（Load page fault），`mtval=0x4000` |
| `f` | 清 `PTE.V` 后不 fence：可能仍成功（旧 TLB）；`sfence.vma` 后再读应 fault 13 |

`f` 第一步若不 trap，就是 TLB 还留着旧项——这正是本课要证明的。有的实现会立刻 refill，那第一步就 fault，也说得通。

## 构建

```bash
cd 06-tlb && make
```

下一课：[07-cache](../../07-cache/)

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
