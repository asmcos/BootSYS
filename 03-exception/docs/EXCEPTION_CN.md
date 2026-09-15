# 03 · 异常打印（同步 trap）

> 前置：01 串口。02 多核任选（本课只跑 CPU0）。  
> 本课第一次装 **`mtvec`**，把同步异常打印出来，然后**跳过故障指令**继续跑。

08 / 09 的中断也走 `mtvec`，但那是异步的（`mcause[63]=1`）。本课全部是同步异常（最高位 0）。

---

## 要证明的对比

| 键 | 触发 | `mcause` 异常号 | 期望 |
|----|------|-----------------|------|
| `a` | 读 `0x1` | **5** Load access fault | dump 后 `mepc+=2/4`，打印 resumed |
| `s` | 写 `0x8` | **7** Store access fault | 同上 |
| `z` 前半 | 整数 `100 / 0` | **不 trap** | 商多为全 1（`0xfff…f`） |
| `z` 后半 | `ebreak` | **3** Breakpoint | 走 handler |
| `i` | `.word 0x00000000` | **2** Illegal instruction | dump 后恢复 |

RISC-V **整数除零不产生异常**。若 `z` 的 DIV 进了 trap，说明你看的不是这颗核的整数除法，或编译器没发出 `div`。

---

## 硬件路径

```text
故障指令
  → 硬件写 mcause / mepc / mtval / mstatus
  → PC ← mtvec（本课 Direct：trap_entry）
       压 GPR
       trap_handler 打印
       mepc += 指令长度（2 或 4）
       mret → 下一条用户指令
```

| CSR | 含义 |
|-----|------|
| `mtvec` | trap 入口；本课 Direct，低 2 位为 0 |
| `mcause` | `[63]=0` 同步；低位是异常号 |
| `mepc` | **那条故障指令**的 PC（不是下一条） |
| `mtval` | 坏地址或坏指令编码（视原因） |
| `mstatus.MPP` | 进 trap **之前**的特权级（本课一直是 M=3） |

指令长度：看 `mepc` 处半字，低 2 位 ≠ `0b11` 就是 16-bit C 扩展，否则 32-bit。`ebreak` 可能是 2 字节 `c.ebreak`。

`medeleg` / `mideleg` 本课写 0：所有 trap 留在 M。

```mermaid
flowchart LR
  insn[故障指令] --> csr[mcause mepc mtval]
  csr --> mtvec[trap_entry]
  mtvec --> dump[打印 CSR/GPR]
  dump --> skip["mepc += 2/4"]
  skip --> mret[mret 回 main]
```

---

## 菜单

```text
[M] a    load access fault
[M] s    store access fault
[M] z    DIV by 0（无 trap）然后 ebreak
[M] i    非法指令
[M] h    帮助
```

### 期望

打印里应看到 `!!!! TRAP !!!!`、`mcause`、`mepc`、`mtval`、部分 GPR，然后 `resumed`。  
`a`/`s` 的 `mtval` 应接近 `0x1` / `0x8`。

若按键后死机、不再回提示符：`mtvec` 没装上，或 handler 里又 fault（例如 UART 基址错）。  
若 `i` 的 cause 不是 2：可能执行的不是 `0x00000000`（被当 NOP 的实现极少，C908 会非法）。

---

## 常见误区

1. **「除零一定会 trap」**  
   那是 x86 习惯。RISC-V 整数 DIV 除 0 写全 1，不进 `mtvec`。浮点另说，本课不用 F。

2. **「`mepc` 是下一条」**  
   同步异常指向**故障指令**。中断才指向「被打断后要执行的那条」（08）。

3. **「恢复时不改 `mepc`」**  
   本课若原样 `mret`，会立刻再执行同一条故障指令 → 死循环。所以要 skip。`ecall`（04）同样要 skip；**定时器中断不要 skip**。

4. **`mcause` 当有符号数打印**  
   同步时最高位是 0，看起来就是 2/3/5/7。中断时是 `1<<63 | code`，务必先拆 irq 位。

5. **JTAG 重载后 `csrr mstatus` 变 illegal**  
   核可能停在 U。先复位，再加载。

---

## 关键文件

| 文件 | 作用 |
|------|------|
| `arch/riscv/trap_entry.S` | 压栈 / 调 C / `mret` |
| `src/trap.c` | dump + `insn_len` + skip |
| `src/main.c` | 菜单触发故障 |

---

## 不做

特权级切换、PMP、中断使能。本课只建立「能看见 trap 并活着回来」。

---

## 构建

```bash
cd 03-exception && make
```

下一课：[04-privilege](../../04-privilege/docs/PRIVILEGE_CN.md)

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
