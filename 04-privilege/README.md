# 04-privilege

交互式 **M / S / U** 特权级切换。先看到当前模式，再在本模式里做访问测试；  
测试触发异常后由 M 处理，然后**回到原来的特权级**。也可随时切到其他模式。

## 编译

```bash
cd 04-privilege && make
# → ../out/04-privilege/bootsys.bin   @ 0x80200000
```

## 用法示例

1. 启动后在 **M-mode**，提示符 `[M] >`
2. 按 `s` → 进入 **S-mode**，提示符 `[S] >`
3. 按 `1` → 读 `mhartid`（S 无权限）→ 打印异常 → **仍留在 S**
4. 按 `u` → 经 ecall 切到 **U-mode**；按 `m` 回到 M

## 命令

| 键 | 作用 |
|----|------|
| `1` | 读 `mhartid`（仅 M 允许） |
| `2` | 读 `sstatus`（M/S 允许） |
| `3` | 读 `mstatus`（仅 M 允许） |
| `4` | `ecall`（`mcause`：U=**8** / S=**9** / M=**11**，不是 10） |
| `p` | 证明当前模式（`mcause` + `mstatus.MPP`） |
| `m`/`s`/`u` | 切换到对应模式 |
| `h` | 帮助 |

## 文档

- [docs/PRIVILEGE_CN.md](docs/PRIVILEGE_CN.md) — 含 **ecall 如何判定** 等问答
- 仓库：[文档索引](../docs/README.md) · [首页](../README.md)

## 下一课

[`../05-pmp/`](../05-pmp/) — 设 PMP 保护区，M/S/U 测完再取消。
