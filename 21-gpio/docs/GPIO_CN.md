# 21-gpio — 庐山派点灯

## 目标

在嘉立创·庐山派 **K230-CanMV（标准版）** 上：

1. pinmux → GPIO  
2. 输出电平点亮/熄灭板载 RGB  
3. 可选：读用户键、边沿中断自测  

## 硬件（标准版）

共阳 RGB，**低电平点亮**：

| 灯 | io / GPIO |
|----|-----------|
| R | 62 |
| G | 20 |
| B | 63 |
| KEY | 53（下拉，按下为高） |

对应控制器：20→GPIO0；62/63→GPIO1。PLIC：脚 n 对应源 `32+n`（n&lt;32）或 `64+(n-32)`。

## 推荐试验

1. `./cp_bin.sh 21-gpio` upload  
2. 顶层 **`4`**：彩虹，确认灯硬件 OK  
3. 顶层 **`5`**（pinmux prove）→ **`s`**：  
   - GPIO → 亮  
   - alt1 → 灭（gpio 仍在写）  
   - 再 GPIO → 又亮  
4. 手动：`0` / `1` / `2` 对比；`g` 换绿灯再试  

结论：**pad 复用决定信号接到谁；GPIO 寄存器 alone 不够。**

## Lite 板注意

Lite-K230D 为 GPIO65/66/71（PMU 域），本课默认标准版 io20/62/63。

## 相关

- [20 PINMUX](../../20-pinmux/docs/PINMUX_CN.md)  
- [09 IRQ](../../09-irq/docs/IRQ_CN.md)  
- 立创文档：[GPIO 和 FPIOA](https://wiki.lckfb.com/zh-hans/lushan-pi-k230/basic/gpio-fpioa.html)

## 下一课

[22-spi](../../22-spi/docs/SPI_CN.md)：同一套 pinmux 接到 QSPI0，读 Flash ID 或 loopback。
