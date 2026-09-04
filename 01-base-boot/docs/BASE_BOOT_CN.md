# 01 · base-boot（最小启动 + 串口）

## 目标

验证 K230 上 **CPU0** 能从 `0x80200000` 启动，并通过 **UART0**（`0x91400000` / 115200）打印。

不做：CPU1、共享内存、IPI、菜单。

## 路径

```
复位 → BOOTROM → 加载 bin → _start → thead_cpu_init → main
                                                      → uart_init / banner / echo
```

## 构建与烧录

```bash
cd 01-base-boot && make
```

加载 `../out/01-base-boot/bootsys.bin` 到 `0x80200000`。

## 期望输出

圆角横幅（Experiment / base-boot）+ `cpu0 mhartid=...`，随后键盘回显。

## 成功标准

| 项 | 条件 |
|----|------|
| 启动 | 进 `main`，不非法指令 trap |
| 串口 | 横幅可读（终端 UTF-8） |
| 回显 | 按键有回显 |

## 与 02 的关系

本目录是**独立纯净**的最小树。  
`02-multi-cpu` 提供**完整**多核固件代码；阅读文档时应先完成本课，再看 02 的阶进说明。
