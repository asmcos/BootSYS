# 10-thread

大小核都用调度切换本核线程；**打印走统一 ring**，只有 CPU0 在中断/空闲里 drain 到 UART。

## 编译

```bash
cd 10-thread && make
# → ../out/10-thread/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] t    先测 CPU1 → print ring → CPU0 drain（探针，无调度）
[M] p    跑一批：两核 worker 都 print_printf 入环
[M] n    设置 N（默认 5）
[M] s    状态
[M] h    帮助
```

## 打印系统

```text
CPU0 / CPU1   print_puts / print_printf
                 ↓  SHMEM+128 print ring (amo 入队)
CPU0 only     print_drain_isr()  ← sched_on_timer (MTIP)
              print_drain_all()  ← 菜单 / 等待循环
                 ↓
              UART
```

- 大核 **禁止** 直接 `uart_*`（CSR `mhartid` 两边常为 0，用 `tp`/`bootsys_cpuid` 区分）
- 格式化在线程里完成；ISR 只搬行到串口

## 文档

- [docs/THREAD_CN.md](docs/THREAD_CN.md)
- 前置：[08-timer](../08-timer/) · [02-multi-cpu](../02-multi-cpu/)
- 后续：[11-thread-comm](../11-thread-comm/) · [12-atomic](../12-atomic/)
