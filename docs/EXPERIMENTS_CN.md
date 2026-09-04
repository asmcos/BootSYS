# BootSYS 实验序列

BootSYS 是长期规划下的 RISC-V 底层验证工程。按实验递进，而不是一次做成完整系统。

| 顺序 | 实验 | 内容 | Git 标签 |
|------|------|------|----------|
| **1** | 最小启动 + 串口 | 仅 CPU0：`_start` → UART 打印 / 回显 | `exp1-uart-boot` |
| **2** | 多核运行 | CPU0 拉起 CPU1、shmem、菜单、tick | `exp2-multi-cpu`（兼：`demo-multi-cpu`） |

## 为何实验一标签是「补打」的？

历史上先做出了多核 demo，并打了 `demo-multi-cpu`，当时没有单独冻结「仅串口」节点。

补救做法（推荐，已采用）：

1. 在仓库中放入**自洽、纯净**的实验一代码树：`experiments/01-uart-boot/`（不掺 CPU1）。
2. 写清说明：`docs/EXP01_UART_BOOT_CN.md`、本文件。
3. 打附注标签 **`exp1-uart-boot`** 指向该提交。
4. 给已有多核里程碑补打 **`exp2-multi-cpu`**（可与 `demo-multi-cpu` 指向同一提交），实验编号对齐。

这样：

- 时间线上实验一标签可能晚于实验二提交，但**语义上**仍是第一课；
- 代码上实验一目录保持最小，不会被多核逻辑污染；
- 检出任意实验：`git checkout exp1-uart-boot` / `git checkout exp2-multi-cpu`。

## 构建入口

```bash
make exp1          # 实验一 → out/exp01/bootsys.bin
make BOARD=k230    # 当前主树（多核 demo）→ out/k230/bootsys.bin
```

## 推送标签（勿忘）

```bash
git push origin exp1-uart-boot exp2-multi-cpu
# 或：git push origin --tags
```

网页查看：仓库 → **Tags**（仅 `git push` 提交不会带上 tag）。
