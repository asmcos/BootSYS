# 10-thread

大小核都用 **CLINT 机器定时器中断（MTIP）** 做抢占式线程切换。  
每个 hart 自己的 `mtimecmp`、自己的就绪队列；**线程不跨核迁移**。

## 编译

```bash
cd 10-thread && make
# → ../out/10-thread/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] t    先测 CPU1 消息环（无调度，不起 CPU0 worker）
[M] p    跑一批：CPU0 直接打印；CPU1 写入 shmem，由 CPU0 取出打印
[M] n    设置 N（默认 5）
[M] s    状态
[M] h    帮助
```

跑完 N 次后回菜单。

**CPU1 消息机制**（大核禁止直接写 UART）：

```text
CPU1  cpu1_msg() / cpu1_msg_worker()
        ↓  shmem log ring @ SHMEM+128
CPU0  shmem_log_drain() → 串口打印 [cpu1] ...
```

菜单 `t` 只测这条通路；`p` 再跑双核线程。

## 文档

- [docs/THREAD_CN.md](docs/THREAD_CN.md)
- 前置：[08-timer](../08-timer/) · [02-multi-cpu](../02-multi-cpu/)
- 后续：[11-thread-comm](../11-thread-comm/) · [12-atomic](../12-atomic/)
