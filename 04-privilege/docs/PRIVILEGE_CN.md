# 04 · 特权级切换（M / S / U）

> 前置：01 串口、03 trap。本课：**先切模式，再在本模式测权限；异常处理后回到本模式**。

## 行为说明

1. 启动处于 **M-mode**，串口提示符显示当前级：`[M]` / `[S]` / `[U]`  
2. 启动时配置 **PMP 全空间 R/W/X**（C908 有 PMP；不配则 S/U 默认禁访，进 S/U 后在 `0x80200000` 取指会 access fault——**不是加载地址错了**）  
3. `m` / `s` / `u` 切换特权级（S/U → 其他模式通过 `ecall` 由 M 转发）  
4. `1`/`2`/`3`/`4`/`p` 在**当前模式**访问 CSR、`ecall` 或做模式证明  
5. 若权限不足 → 进 M 的 trap → 打印异常 → **跳过指令并 mret 回原模式**

## 期望

| 当前模式 | mhartid | sstatus | mstatus | ecall `mcause` |
|----------|---------|---------|---------|----------------|
| M | 成功 | 成功 | 成功 | **11** |
| S | 异常 | 成功 | 异常 | **9** |
| U | 异常 | 异常 | 异常 | **8** |

---

## 常见问题（更细）

### 1. 有没有单独的「这是 ecall」标志位？

**没有。**  
`ecall` 和非法指令、访问故障一样，都是同步异常：硬件把原因写进 **`mcause`**，再跳到 `mtvec`（本课只在 M 收异常，`medeleg=0`）。

Trap 入口里我们这样读：

```text
cause  = csrr mcause
code   = cause 的低位异常号   （本课用 cause & 0xfff）
irq    = cause 的最高位       （1=中断，0=异常）
```

判断「是不是 ecall」=：**`irq==0` 且 `code` 属于 ecall 那几档**。

本课代码（`trap.c`）大致是：

```c
if (!interrupt && (code == 8 || code == 9 || code == 11) && !trap_expect)
    /* 当成「切模式用的 ecall」 */
```

访问测试时的 `ecall`（菜单 `4` / `p`）则走 `trap_expect` 分支，同样用 `code` 区分，并额外看 `mstatus.MPP`。

### 2. 是用 code = 8、9、10 区分的吗？

**标准 M/S/U（无虚拟化）用的是 8、9、11，不是 8、9、10。**

RISC-V 特权手册里，与 `ecall` 相关的异常号是：

| `mcause` 异常号 | 含义 |
|-----------------|------|
| **8** | Environment call from **U**-mode |
| **9** | Environment call from **S**-mode |
| **10** | （保留；若开了 Hypervisor，常为 **VS**-mode 的 ecall） |
| **11** | Environment call from **M**-mode |

本课只做 M/S/U，所以：

- U 里 `ecall` → **8**
- S 里 `ecall` → **9**
- M 里 `ecall` → **11**
- **不会**指望看到 10

同一条指令都叫 `ecall`，硬件**根据执行时的特权级**填不同的 `mcause`，用这个区分「谁发的」，而不是再设一个独立标志寄存器。

### 3. 除了 `mcause`，还能靠什么证明切模式成功？

进 M 处理异常时，还可看：

| 字段 | 含义 |
|------|------|
| **`mstatus.MPP`** | 出异常**之前**的特权级：`3`=M，`1`=S，`0`=U |

注意：运行中**没有**「当前是 S」可读 CSR；`MPP` 是 **trap 进 M 之后**才有意义的「来源特权级」。

菜单 **`p`**：在当前模式执行一次测试用 `ecall`，打印 `mcause` + `MPP`，用来证明当前级。

辅助手段（不如上面硬）：试读 `mhartid`（仅 M 成功）等。

### 4. 切模式的 ecall 和测试用的 ecall 怎么分开？

硬件上看都是 8/9/11。软件用约定区分：

| 用途 | 做法 |
|------|------|
| 切模式（`m`/`s`/`u`） | `a0` = 1/2/3（`ECALL_GOTO_*`），且 **不**置 `trap_expect` |
| 测试 / 证明（`4`/`p`） | 置 `trap_expect=1`，`a0` = 4（`ECALL_TEST`）；处理后 **跳过 ecall，mret 回原模式** |

### 5. 和非法指令（`mcause=2`）差在哪？

| | `ecall` | 非法 CSR / 非法指令 |
|--|---------|---------------------|
| `mcause` | 8 / 9 / 11 | 多为 **2** |
| 含义 | 主动「系统调用」式陷入 | 无权或编码非法 |
| 本课 | 切模式或 `p` 证明 | 菜单 `1`/`2`/`3` 的权限演示 |

---

## 关键文件

| 文件 | 作用 |
|------|------|
| `arch/riscv/priv_switch.S` | `priv_enter` / `trap_jump_shell`：`mret` 进目标 shell |
| `arch/riscv/trap_entry.S` | M-mode trap 压栈 → `trap_handler` → `mret` |
| `src/trap.c` | 用 `mcause` 区分 ecall(8/9/11)；打印 `MPP` |
| `src/priv.c` | CSR / ecall / `prove_current_mode` |
| `src/main.c` | `shell_m` / `shell_s` / `shell_u` |

## 构建

```bash
cd 04-privilege && make
```

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
