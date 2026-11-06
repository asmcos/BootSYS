# 22-spi

K230 **QSPI0**（DW_apb_ssi @ `0x91582000`）+ pinmux；读 SPI Flash JEDEC ID，或内部/外部 loopback。

## 接线（默认可与 CanMV 例程一致）

| 信号 | io | mux |
|------|-----|-----|
| CS | 14 | GPIO（软件片选） |
| CLK | 15 | QSPI0_CLK（IO_SEL=3） |
| MOSI | 16 | QSPI0_D0 |
| MISO | 17 | QSPI0_D1 |

外接 Flash：CLK/MOSI/MISO/CS/GND/3V3。无 Flash：菜单开 **loopback**，或把 MOSI–MISO 短接。

## 编译

```bash
./cp_bin.sh 22-spi
```

## 菜单

```text
1  setup pins + spi_init
2  xfer
     i  Flash RDID 0x9F
     r  read 16 bytes at 0x10000
     w  erase that 4K sector, program a0..af, read back (PASS/FAIL)
     l  loopback A5 5A (ff ff with loopback off and no short is expected)
3  config：mode 0..3 / baud_div / loopback
4  dump pinmux io14-17
5  pinmux prove（CLK 切 GPIO 再切回 QSPI）
```

推荐：`1` → `3` 里 `t` 开 loopback → `a` → `2` → `l` 应 PASS。  
有 Flash：`1` → `2` → `i`，ID 非 `00/ff` 即 PASS（如 Winbond `ef`）。

## 文档

- [docs/SPI_CN.md](docs/SPI_CN.md)
- 前置：[20-pinmux](../20-pinmux/) · [21-gpio](../21-gpio/)
