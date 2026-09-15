# 07 · L1 cache tag dump

> 前置：03（非法指令会打印）、06（cache 和页表不是一回事）。  
> 本课：**用核里的诊断 CSR 把 L1 tag 读出来**，看 touch / flush 之后 tag 变不变。

标准 RISC-V **没有**「读 cache tag」的指令。这是玄铁 C9xx 的 M-mode 诊断，不是 Sv39，也不是 02 的 `thead_flush_range` 本身。

---

## 要证明的对比

| 键 | 做法 | 期望 |
|----|------|------|
| `1` | 写探针再读该 set 的 D-tag | 至少一路 `V=1`，`pa~` 对上探针（或同一 4K） |
| `2` | 调用一小段代码，读 I-tag | 该函数所在 set 至少一路 `V=1` |
| `f` | `thead_flush_caches` 后再 dump 探针 | 探针行不应再 `V=1`（UART/栈可能占别的 set） |
| `s` | 扫描所有 D-set，列出 `V=1` | 行数随程序活动变化；raw 必打 |

若 `1` 四路全 0 且 raw 也是 0：要么 CSR 非法（没在 M / 这颗核没这组 CSR），要么 way/index 编码和这颗 C908 不一致。

---

## 为什么能 dump

| CSR | 编号 | 作用 |
|-----|------|------|
| `MCINDEX` | `0x7d3` | 选 RID / way / index |
| `MCINS` | `0x7d2` | 写 `1` 发起一次阵列读 |
| `MCDATA0/1` | `0x7d4` / `0x7d5` | 读回 tag 或 data |

`MCINDEX`：

| 位 | 含义 |
|----|------|
| `[31:28]` | RID：`0` I-tag，`2` D-tag（`1`/`3` 是 data，本课不读） |
| `[24:21]` | way（C906 风格） |
| `[20:17]` | L1 way（C908 / xuantie） |
| `[16:0]` | 地址低位；64B 行忽略 `[5:0]` |

本课把 way 同时写进 `[24:21]` 和 `[20:17]`，两种译码都能对上。

---

## K230 C908 L1

32KiB、4 路、64B 行 → **128 set**。  
set = `PA[12:6]`。tag 按 C9xx：`MCDATA0` 的 `V=bit0`，`D=bit2`（D-cache），`bits[39:12] ≈ PA[39:12]`。

C908 和开源 C906 的 tag 位可能略有差别，所以菜单里 **raw 必打**。对上探针地址（或同一 4K）就算成功，不要死扣某一 bit 的教科书含义。

`thead_cpu_init` 已开 L1（`mhcr` 非 0）。本课只在 M；S/U 写这些 CSR 会 illegal。

```mermaid
flowchart LR
  touch[store 探针] --> set["set = PA[12:6]"]
  set --> idx[写 MCINDEX]
  idx --> ins[写 MCINS=1]
  ins --> data[读 MCDATA0]
  data --> tag[V / D / PA]
```

---

## 菜单

```text
[M] 1    touch 探针，dump 该 D-set 四路
[M] 2    跑一小段代码，dump I-set
[M] f    flush L1，再看探针 D-tag
[M] s    扫描有效 D 行
[M] h    帮助
```

### 期望

`1` 打印 raw + 解码后的 V/PA。  
`f` 后探针 set 的 V 应掉下去；别的 set 仍可能有 UART/栈。  
误进 trap 且 cause=2：诊断 CSR 不可用，或当前不是 M。

---

## 常见误区

1. **「这就是 MMU / TLB」**  
   tag 里是**物理**行。06 的 TLB 是 VA→PA。flush 页表不会清 L1 tag。

2. **「flush 后所有 V 都必须是 0」**  
   打印、读 CSR 本身又会填 D-cache。只盯探针那一行。

3. **「标准 `fence` 能读出 tag」**  
   `fence` 排访存顺序；读阵列必须走 `MCINDEX`/`MCINS`。

4. **在 S 里 dump**  
   这些 CSR 是 M 的。先回 `[M]`。

5. **把 C906 手册的位域当成 C908 真理**  
   所以保留 raw。对上 PA 即可。

---

## 关键文件

| 文件 | 作用 |
|------|------|
| `src/cache_tag.c` | 写 MCINDEX/MCINS，解码 MCDATA |
| `src/main.c` | `1`/`2`/`f`/`s` |
| `arch/riscv/cache.S` | `thead_cpu_init` / `thead_flush_caches` |
| `src/trap.c` | CSR 非法时 dump |

---

## 不做

L2 阵列、改 cache 内容、用 tag 做侧信道。本课只读 L1 tag，证明「摸过的行能看见、flush 后能消失」。

---

## 构建

```bash
cd 07-cache && make
```

下一课：[08-timer](../../08-timer/docs/TIMER_CN.md)

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
