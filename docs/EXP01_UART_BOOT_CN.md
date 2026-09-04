# 实验 01：最小启动与串口

> **目标**：板上只跑 **CPU0**，完成复位启动与 UART0 打印。  
> **代码**：`experiments/01-uart-boot/`（自洽，无 CPU1）。  
> **标签**：`exp1-uart-boot`

## 1. 要验证什么

上电后 BOOTROM 把镜像放到 `0x80200000`，CPU0 从 `_start` 进入：

1. 清寄存器、建栈、拷 `.data`、清 `.bss`
2. `thead_cpu_init`（玄铁 C908 必要 CSR）
3. `main` → `uart_init` → 打横幅 → 回显

**不做**：解复位 CPU1、共享内存、IPI、交互菜单。

## 2. 构建与烧录

```bash
make exp1
# → out/exp01/bootsys.bin
```

J-Link / 你们现有加载路径把 bin 下到 `0x80200000`，串口 115200。

期望类似：

```text
╭──────────────────────────────────────────────╮
│  BootSYS  ·  Experiment 01                   │
│  minimal boot + UART (CPU0 only)             │
...
╰──────────────────────────────────────────────╯

cpu0 mhartid=0x0000000000000000
```

之后键入字符应原样回显。

## 3. 成功标准

| 项 | 通过条件 |
|----|----------|
| 启动 | 不 trap，能进 `main` |
| 串口 | 横幅完整可读（UTF-8 终端） |
| 回显 | 按键有回显 |
| 范围 | 镜像中无 CPU1 / shmem / CLINT 业务逻辑 |

## 4. 与实验二的关系

本实验过关后，再进入 [K230_DUAL_CORE_CN.md](K230_DUAL_CORE_CN.md) / 标签 `exp2-multi-cpu`：在同一套启动与串口之上加第二核。

实验编号与补救说明见 [EXPERIMENTS_CN.md](EXPERIMENTS_CN.md)。
