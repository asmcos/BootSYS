# 07-cache

C908 L1 **tag dump**。用玄铁诊断 CSR 读出 I/D-cache 某一组的 tag，对照一次 touch / flush。

只在 **M**。不走页表。

## 编译

```bash
cd 07-cache && make
# → ../out/07-cache/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] 1      写+读探针行，dump 该 set 的 D-tag（应有一路 V=1）
[M] 2      执行一小段代码，dump 其 set 的 I-tag
[M] f      先 touch，再 flush L1，再 dump 探针 set（探针行应 V=0）
[M] s      扫全部 D-cache，只打印 V=1
```

| 键 | 作用 |
|----|------|
| `1` | touch + D-tag |
| `2` | I-tag |
| `f` | flush 后再看 D-tag |
| `s` | 扫描有效 D-tag |
| `h` | 帮助 |

几何按 K230 C908：L1 **32KiB / 4-way / 64B** → 128 set。  
读法：`MCINDEX`(0x7d3) → `MCINS`(0x7d2)=1 → `MCDATA0/1`(0x7d4/0x7d5)。  
解码按 C9xx：`V=bit0`，`D=bit2`，`tag=bits[39:12]`；**raw 始终打印**，便于和手册对。

## 文档

- [docs/CACHE_CN.md](docs/CACHE_CN.md)
- 前置：[06-tlb](../06-tlb/docs/TLB_CN.md)
- 下一课：[08-timer](../08-timer/)
- 仓库：[文档索引](../docs/README.md)
