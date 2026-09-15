# 01 · 最小启动 + UART

> 本课是整套实验的底板：**只有 CPU0**，从 SRAM 起来，把字打到串口，并把键盘回显回去。  
> 不做 CPU1、shmem、trap、菜单。那些从 02 / 03 才开始。

---

## 要证明的对比

| 做法 | 期望 |
|------|------|
| JTAG / BOOTROM 把 bin 放到 `0x80200000` 再跳入 | 能进 `_start` → `main`，不非法指令 |
| `uart_init` 后打印横幅 | 终端 UTF-8 能读到圆角框和 `cpu0 mhartid=` |
| 轮询 `LSR.DR` 取键、写 `THR` | 按键回显；回车变成 `\r\n` |

本课验收的是 **CPU0 + UART0 这条最短通路**，不是「系统起来了」。

---

## 硬件路径

K230 上电后只有 **CPU0**（小核 C908）从复位走出。BOOTROM 或 J-Link 把镜像放到片上 SRAM，PC 从 `0x80200000` 取指。

```text
复位 → BOOTROM / JTAG 加载
     → 0x80200000  _start
          清 GPR、设 sp、拷 .data、清 .bss
          thead_cpu_init（玄铁 0x7c0–0x7c5 / 0x7f3）
     → main
          uart_init @ 0x91400000  115200 8N1
          banner + echo
```

| 项 | 值 |
|----|-----|
| 加载地址 | `0x80200000`（SRAM；本课无 `LOAD=ddr`） |
| UART0 | `0x91400000`，时钟 **48.6 MHz**，波特率 115200 |
| 分频 | `DLL/DLM = clock / (16 * baud)`，先置 `LCR.DLAB` |
| 发送 | 等 `LSR.THRE`，写 `THR` |
| 接收 | 看 `LSR.DR`，读 `RBR`（本课 **IER=0**，纯轮询） |

16550 风格：数据口 `+0x00`，`IER +0x04`，`LCR +0x0c`，`LSR +0x14`。本课关中断、开 FIFO，只为把字符弄出去。

`thead_cpu_init` 会开 L1 / 写玄铁 CSR。本课不读 cache tag（那是 07），但若不做这一步，后面几课的一致性假设不成立。

---

## 菜单 / 交互

没有菜单。启动后：

```text
╭── BootSYS · 01-base-boot ──╮
│  load    0x80200000        │
│  uart0   0x91400000  115200│
╰────────────────────────────╯
cpu0 mhartid=0x0
```

之后键盘原样回显。

### 期望

| 现象 | 判定 |
|------|------|
| 横幅乱码 | 终端不是 UTF-8，或波特率不是 115200 |
| 完全无输出 | 加载地址错、UART 基址错、或还停在 BOOTROM |
| 有横幅无回显 | TX 通、RX 不通：查接线 / `LSR.DR`，不是 CPU 没起来 |
| `mhartid` 不是 0 | 你跑到了别的 hart（本课不应发生） |

---

## 常见误区

1. **「烧进 Flash 才算启动」**  
   BootSYS 默认是 **RAM 里执行**。JTAG 放到 `0x80200000` 即可。

2. **「镜像在内存里，两核都会跑」**  
   只有 CPU0 被复位向量带到这里。CPU1 仍关在 RMU 复位里（02）。

3. **「串口中断没开所以收不到键」**  
   本课故意轮询。中断是 09。

4. **JTAG 重载但不复位**  
   可能停在 U-mode 或旧 `mtvec` 上。01 不碰特权 CSR；若重载后乱跳，先硬件复位。

---

## 关键文件

| 文件 | 作用 |
|------|------|
| `start.S` | CPU0 `_start`：栈 / `.data` / `.bss` / `thead_cpu_init` |
| `cache.S` | 玄铁 CPU 初始化 |
| `uart.c` | 16550 轮询收发 |
| `main.c` | 清屏、横幅、echo |
| `linker.ld` | SRAM，单栈 `_sp` |
| `board.h` | UART / `LOAD_BASE` |

---

## 不做

CPU1、共享内存、IPI、`mtvec`、特权级。本目录是**独立最小树**，不要从 02 倒着读。

---

## 构建

```bash
cd 01-base-boot && make
```

加载 `../out/01-base-boot/bootsys.bin` 到 `0x80200000`。

下一课：[02-multi-cpu](../../02-multi-cpu/docs/MULTI_CPU_CN.md)

返回：[本目录 README](../README.md) · [文档索引](../../docs/README.md)
