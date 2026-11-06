# 20-pinmux — IOMUX 复用与 pad 配置

## 目标

在写任何 GPIO/SPI 控制器寄存器之前，先搞清楚：

1. **复选（mux）**：这根脚接到哪个功能（GPIO / UART / QSPI…）
2. **配置（pad）**：输入使能、输出使能、上下拉、驱动强度等

## 硬件

| 模块 | 基址 | 说明 |
|------|------|------|
| 主 IOMUX | `0x91105000` | io0–io63，每脚一个 32-bit 寄存器，偏移 `pin×4` |
| PMU IOMUX | `0x91000080` | 另 8 脚（本课编号 64–71） |
| 时钟门 | `0x91100050` bit20 | IOMUX APB |

位域（与 Linux `canaan,k230-iomux` / TRM 一致）：

| 位 | 名 | 含义 |
|----|----|------|
| 13:11 | IO_SEL | 功能 0–7（**0=alt0=GPIO**） |
| 8 | IE | 输入使能 |
| 7 | OE | 输出使能（pad 级；GPIO 方向还看控制器 DDR） |
| 6/5 | PU/PD | 上拉 / 下拉 |
| 4:1 | DS | 驱动强度 0–15 |
| 10/0 | SL/ST | slew / schmitt |
| 31 | DI | 输入数据（只读） |

**勿乱动 io38/io39**：默认 UART0。误改后用菜单 `r` 恢复。

## 代码

- [`src/pinmux.c`](../src/pinmux.c) — `pinmux_set_sel` / `pinmux_as_gpio` / `pinmux_restore_uart0`
- 默认演示脚：**io20**（避开启动/调试常用脚）

## 板上怎么验（递进菜单）

1. 顶层 `1` 选脚（默认可 `q`→io20），`b` 回顶层  
2. `3` 进 mux：`0` 设 GPIO，`d` dump，`b` 返回  
3. `4` 进 pad：调 IE/OE/上下拉；`s` 再设 DS  
4. `2` dump 当前脚 + UART0；串口挂了用 `6` 恢复  

勿乱动 io38/io39。

## 下一课

[21-gpio](../../21-gpio/docs/GPIO_CN.md)：在 alt0 上操作 DW_apb_gpio，并接 PLIC 边沿中断。
