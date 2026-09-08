# 07 · L1 cache tag dump

> 前置：03 异常（非法指令会打印）、06 TLB（知道 cache 和页表不是一回事）。  
> 本课：**用核里的诊断 CSR 把 L1 tag 读出来**，看 touch / flush 之后 tag 变不变。

## 为什么能 dump

标准 RISC-V **没有**“读 cache tag”的指令。玄铁 C9xx 在 M-mode 提供：

| CSR | 编号 | 作用 |
|-----|------|------|
| `MCINDEX` | `0x7d3` | 选 RID / way / index |
| `MCINS` | `0x7d2` | 写 `1` 发起一次阵列读 |
| `MCDATA0/1` | `0x7d4` / `0x7d5` | 读回 tag 或 data |

`MCINDEX`：

| 位 | 含义 |
|----|------|
| `[31:28]` | RID：`0` I-tag，`2` D-tag（`1`/`3` 是 data，本课不读） |
| `[24:21]` | way（C906；C908 上当 L1 读时可不理 L2） |
| `[20:17]` | L1 way（C908 / xuantie） |
| `[16:0]` | 地址低位；64B 行会忽略 `[5:0]` |

本课把 way 同时写进 `[24:21]` 和 `[20:17]`，两种译码都能对上。

## K230 C908 L1

32KiB、4 路、64B 行 → **128 set**。  
set = `PA[12:6]`。tag 按 C9xx：`MCDATA0` 的 `V=bit0`，`D=bit2`（D-cache），`bits[39:12] ≈ PA[39:12]`。

C908 和开源 C906 的 tag 位可能略有差别，所以菜单里 **raw 必打**。对上探针地址（或同一 4K）就算成功。

## 期望

| 键 | 期望 |
|----|------|
| `1` | 探针行所在 set 至少一路 `V=1`，`pa~` 对上探针（或同 4K） |
| `2` | 目标函数所在 set 至少一路 `V=1` |
| `f` | flush 后探针行不应再 `V=1`（UART/栈可能占别的 set） |
| `s` | 列出当前有效 D 行，行数随程序活动变化 |

`thead_cpu_init` 已开 L1（`mhcr` 非 0）。若 `1` 四路全 0 且 raw 也是 0，先看是不是非法指令 trap（CSR 不可用），或 way/index 编码和这颗 C908 不一致。

## 构建

```bash
cd 07-cache && make
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
