# 20-pinmux

K230 **IOMUX**：引脚功能复用（`IO_SEL`）与 pad 电气配置。  
菜单是**递进选择**（先顶层，再进子菜单），不是一长串热键。

## 编译

```bash
cd 20-pinmux && make
./cp_bin.sh 20-pinmux
```

## 操作（递进）

```text
顶层
  1  pick pin     → 选脚（数字+回车 / +/- / 预设 qwer）
  2  dump         → 打印当前脚 + UART0 38/39
  3  set mux      → 0..7 设 IO_SEL（0=GPIO）
  4  set pad      → IE/OE/上下拉；s 再进 DS 子菜单
  5  as GPIO      → alt0 + IE + ds8
  6  restore UART0
  h  重打当前菜单
  b  子菜单返回上一级
```

推荐路径：`1` 选 io20 → `3` 设 mux → `4` 调 pad → `2` 看结果。  
勿乱动 38/39；弄坏串口用顶层 `6` 恢复。

## 文档

- [docs/PINMUX_CN.md](docs/PINMUX_CN.md)
- 下一课：[21-gpio](../21-gpio/)
