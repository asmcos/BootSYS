# 22-spi — QSPI0 与 Flash ID

## 目标

1. 用 **20** 的 pinmux 把脚接到 QSPI0  
2. 配置 DW SSI：mode / baud、全双工收发  
3. 读 Flash **JEDEC ID（0x9F）**；无器件时用 **loopback** 自测  
4. 在 `0x10000` 做 4K 擦除、写入 `a0..af`、读回比对  
5. 对照实验：CLK 误 mux 成 GPIO 时通信失败（接 21 的 pinmux prove）

## 硬件

| 项 | 值 |
|----|-----|
| 控制器 | QSPI0 / SSI1 `0x91582000` |
| 时钟门 | `0x91100018` bit7/25/28/29 |
| 默认脚 | io14 CS(GPIO)，io15–17 QSPI0 |

## 试验路径

1. 无 Flash：`1` → config `t`+`a` → `2`/`l` → PASS  
2. 有 Flash：`1` → `2`/`i` → 三字节 ID；`w` 擦写读，期望 `a0..af` 后 PASS  
3. `l` 且未开 loopback、未短接 MOSI-MISO：`rx ff ff` 是 MISO 上拉空闲，不是控制器坏了  
4. `5`：破坏 CLK mux 再恢复，理解 pad 复用  

## 下一课

**23-sd**：板载 TF 卡。SD1 PIO 读扇区；`w` 把图案写到当前 LBA 并留下，再用读命令自己核对。扇区 0 只读。
