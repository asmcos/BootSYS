# 06-tlb

Sv39 页表 + TLB。M 不走 `satp`，所以 **先 `o` 开 MMU 并进 S**，再测。

## 编译

```bash
cd 06-tlb && make
# → ../out/06-tlb/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] o          装页表，satp=Sv39，进入 S
[S/mmu] 1      读已映射 VA 0x1000，应成功（魔数 TLB1）
[S/mmu] 2      读未映射 VA 0x4000，应 load page fault（mcause=13）
[S/mmu] f      清 PTE.V → 再读（可能仍命中 TLB）→ sfence.vma → 再读应 fault
[S/mmu] z      satp=0，回 M
```

| 键 | 作用 |
|----|------|
| `o` | 开 MMU（仅 M）并进 S |
| `z` | 关 MMU，回 M |
| `1` / `2` | 已映射 / 未映射 |
| `f` | 旧 TLB vs `sfence.vma` |
| `m` / `s` | 切模式（不关 satp） |

恒等映射：SRAM `0x80200000` 2MiB + UART `0x91400000` 2MiB。  
另映射 `VA 0x1000` → 物理 `tlb_page`。会清玄铁 `mxstatus.MAEE`，用标准 PTE。

## 文档

- [docs/TLB_CN.md](docs/TLB_CN.md)
- 前置：[05-pmp](../05-pmp/docs/PMP_CN.md)
- 下一课：[07-cache](../07-cache/)
- 仓库：[文档索引](../docs/README.md)
