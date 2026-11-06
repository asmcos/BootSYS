# 21-gpio

嘉立创·庐山派 K230-CanMV：**点灯** + **pinmux 对照**（把 20 的结论用灯验证）。

## 板级

| 灯 | io | 有效 |
|----|-----|------|
| R | 62 | 低电平亮 |
| G | 20 | 低电平亮 |
| B | 63 | 低电平亮 |
| KEY | 53 | 按下=1 |

## 编译

```bash
./cp_bin.sh 21-gpio
```

## 菜单

```text
1  LED 亮灭闪
2  按键
3  全灭
4  彩虹
5  pinmux prove   ← 重点：GPIO vs 非 GPIO
9  IRQ 自测
```

### 5 pinmux prove（推荐）

同一脚、同一 `gpio_write(ON)`：

| 键 | mux | 预期 |
|----|-----|------|
| `0` | alt0 = GPIO | **灯亮** |
| `1` | alt1（如 G→OSPI_D4） | **灯灭**（GPIO 写了但 pad 已切走） |
| `2` | alt2 | **灯灭** |
| `s` | 自动：亮→灭→亮 | 眼看三步 |

也可先 `g` 选绿灯再 `s`。

这就是 20 要证明的事：不配成 GPIO，控制器白写。

## 文档

- [docs/GPIO_CN.md](docs/GPIO_CN.md)
- 前置概念：[20-pinmux](../20-pinmux/)
