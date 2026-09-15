# 11-thread-comm

双核线程 **mailbox 通讯**：谁发给谁、就绪标志、D-cache flush 后对端可见。  
打印复用 10 的 **print ring**（仅 CPU0 drain）。不做锁 / AMO（→12）。

## 编译

```bash
cd 11-thread-comm && make
# → ../out/11-thread-comm/bootsys.bin   @ 0x80200000
```

## 操作

```text
[M] t    print 探针（确认 CPU1→print ring）
[M] g    mailbox GOOD：send/recv 都做 flush/pull
[M] b    mailbox BAD：跳过 cohere —— 可能丢/半包/旧行
[M] p    ping-pong：CPU0 Qn → CPU1 An
[M] c    cache 探针：CPU0 写、CPU1 不 pull 再读（测硬件 L1 是否一致）
[M] n    设置 N（默认 5）
[M] s    状态
[M] h    帮助
```

## 布局

```text
SHMEM+0      ctrl   (alive / done / rx_ok)
SHMEM+64     cmd    (go / loops / mode)
SHMEM+128    mbox   c0→c1 + c1→c0
SHMEM+0x700  cache probe word
SHMEM+0x740  cache probe sync
SHMEM+0x800  print  ring
```

## 文档

- [docs/THREAD_COMM_CN.md](docs/THREAD_COMM_CN.md)
- 前置：[10-thread](../10-thread/) · [02-multi-cpu](../02-multi-cpu/)
- 后续：[12-atomic](../12-atomic/)
