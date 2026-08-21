# BootSYS 开发环境安装说明（Ubuntu）

> **验证平台**：Ubuntu 24.04.1 LTS（x86_64）  
> **验证日期**：2026-08-21  
> **验证结果**：`rustup` / `cargo` / `riscv64imac-unknown-none-elf` 安装成功，`make BOARD=k230` 可生成 `out/k230/bootsys-k230.elf`（入口 `0x80200000`）

BootSYS 是 **Rust 裸机底层验证系统**，不是操作系统，也不是 bootloader。  
构建依赖：

| 组件 | 用途 | 是否必须 |
|------|------|----------|
| `rustup` + `cargo` + `rustc` | 编译 BootSYS | **必须** |
| target `riscv64imac-unknown-none-elf` | RISC-V 64 裸机目标 | **必须** |
| `riscv64-*-objcopy` | ELF → `.bin` | 建议（`make` 需要） |
| `make` / `curl` / `build-essential` | 构建与下载 | **必须** |

---

## 0. 系统准备

```bash
sudo apt update
sudo apt install -y curl build-essential make git pkg-config
```

确认发行版（本说明在 Ubuntu 上测试）：

```bash
lsb_release -a
# 期望类似：
# Distributor ID: Ubuntu
# Description:    Ubuntu 24.04.x LTS
```

---

## 1. 安装 Rust（rustup / cargo / rustc）

### 1.1 官方安装（网络畅通时推荐）

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
source "$HOME/.cargo/env"
```

安装完成后，当前 shell 以及新开终端应能找到：

```bash
rustc --version
cargo --version
rustup --version
```

若新终端找不到命令，把下面一行写入 `~/.bashrc`（或 `~/.zshrc`）后 `source`：

```bash
source "$HOME/.cargo/env"
```

### 1.2 国内网络较慢时（镜像）

官方源下载慢时，可先设置镜像再安装：

```bash
export RUSTUP_DIST_SERVER=https://mirrors.ustc.edu.cn/rust-static
export RUSTUP_UPDATE_ROOT=https://mirrors.ustc.edu.cn/rust-static/rustup

curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
source "$HOME/.cargo/env"
```

可选：把镜像变量写入 `~/.bashrc`，避免每次重新 export。

也可用清华大学镜像（任选其一）：

```bash
export RUSTUP_DIST_SERVER=https://mirrors.tuna.tsinghua.edu.cn/rustup
export RUSTUP_UPDATE_ROOT=https://mirrors.tuna.tsinghua.edu.cn/rustup
```

### 1.3 官方安装脚本卡住时的备用方式

若 `sh.rustup.rs` 长时间停在 `downloading installer`，可直接下载 `rustup-init`：

```bash
mkdir -p /tmp/rustup-dl && cd /tmp/rustup-dl
curl -L -o rustup-init \
  https://static.rust-lang.org/rustup/dist/x86_64-unknown-linux-gnu/rustup-init
chmod +x rustup-init

# 可选镜像
export RUSTUP_DIST_SERVER=https://mirrors.ustc.edu.cn/rust-static
export RUSTUP_UPDATE_ROOT=https://mirrors.ustc.edu.cn/rust-static/rustup

./rustup-init -y
source "$HOME/.cargo/env"
```

---

## 2. 安装 RISC-V 裸机编译目标

BootSYS 使用目标三元组：

```text
riscv64imac-unknown-none-elf
```

含义简要说明：

- `riscv64`：64 位 RISC-V
- `imac`：整数 + 乘除 + 原子 + 压缩指令
- `unknown-none-elf`：无操作系统裸机 ELF

安装：

```bash
source "$HOME/.cargo/env"
rustup target add riscv64imac-unknown-none-elf
```

确认：

```bash
rustup target list --installed
# 至少应包含：
#   riscv64imac-unknown-none-elf
#   x86_64-unknown-linux-gnu
```

---

## 3. 安装 RISC-V binutils（生成 `.bin`）

`cargo` / `rustc` 负责编出 ELF；`make` 默认用交叉 `objcopy` 生成可下载的 raw binary。

### 方案 A：已有玄铁 / 自建交叉工具链（本机验证环境）

若 PATH 中已有例如：

```text
riscv64-unknown-linux-gnu-objcopy
```

则无需再装。可临时加入 PATH：

```bash
export PATH="/path/to/your/riscv-toolchain/bin:$PATH"
```

或在构建时指定前缀：

```bash
make CROSS_COMPILE=riscv64-unknown-linux-gnu-
```

### 方案 B：Ubuntu 软件源（简单）

```bash
sudo apt install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
```

然后：

```bash
make CROSS_COMPILE=riscv64-unknown-elf-
```

### 方案 C：只验证能编过 ELF（不生成 bin）

```bash
source "$HOME/.cargo/env"
cd /path/to/BootSYS
cargo build -p bootsys-k230 --release
# 产物：
# target/riscv64imac-unknown-none-elf/release/bootsys-k230
```

---

## 4. 一键自检（推荐按顺序执行）

```bash
source "$HOME/.cargo/env"

echo "=== versions ==="
rustc --version
cargo --version
rustup --version
rustup target list --installed

echo "=== objcopy ==="
command -v riscv64-unknown-linux-gnu-objcopy \
  || command -v riscv64-unknown-elf-objcopy \
  || echo "WARN: no riscv objcopy in PATH (cargo ELF still OK)"

echo "=== build BootSYS K230 ==="
cd /path/to/BootSYS
make BOARD=k230
# 或：
# make BOARD=k230 CROSS_COMPILE=riscv64-unknown-elf-

ls -lh out/k230/
# 期望：
#   bootsys-k230.elf
#   bootsys-k230.bin

# 检查入口地址（需带对应前缀的 readelf）
riscv64-unknown-linux-gnu-readelf -h out/k230/bootsys-k230.elf | grep Entry
# 期望：Entry point address: 0x80200000
```

本说明在 Ubuntu 24.04 上实测通过时的版本示例：

```text
rustc 1.98.0
cargo 1.98.0
target: riscv64imac-unknown-none-elf
Entry point: 0x80200000
```

---

## 5. 日常构建命令

在仓库根目录：

```bash
source "$HOME/.cargo/env"

# 默认板卡 K230
make

# 显式指定板卡 / 工具链前缀
make BOARD=k230
make BOARD=k230 CROSS_COMPILE=riscv64-unknown-elf-

# 调试版
make debug

# 反汇编
make disasm

# 清理
make clean
```

纯 Cargo：

```bash
cargo build -p bootsys-k230 --release
cargo build -p bootsys-k230          # debug
```

产物路径：

| 方式 | 路径 |
|------|------|
| `make` | `out/k230/bootsys-k230.elf` / `.bin` |
| `cargo` | `target/riscv64imac-unknown-none-elf/release/bootsys-k230` |

---

## 6. 常见问题

### 6.1 `cargo: command not found`

```bash
source "$HOME/.cargo/env"
# 或重新打开终端；确认 ~/.bashrc 已 source cargo env
```

### 6.2 `error: can't find crate for target riscv64imac-unknown-none-elf`

```bash
rustup target add riscv64imac-unknown-none-elf
```

### 6.3 `make: riscv64-unknown-linux-gnu-objcopy: No such file`

安装方案 B 的 binutils，或：

```bash
make CROSS_COMPILE=riscv64-unknown-elf-
```

或把已有交叉工具链 `bin` 目录加入 `PATH`。

### 6.4 rustup 下载极慢 / 超时

使用第 1.2 / 1.3 节镜像或直连 `rustup-init`。

### 6.5 链接报 `unknown argument '-nostartfiles'`

BootSYS 使用 `rust-lld`，不要给链接器传 GCC 专用的 `-nostartfiles`。  
当前仓库已通过 `boards/k230/build.rs` 注入 `-Tlinker.ld`，无需该参数。

### 6.6 权限 / 代理

公司网络若强制 HTTP 代理，需同时配置 `http_proxy` / `https_proxy`，否则 `curl` 与 `rustup` 会失败。

---

## 7. 环境变量速查

```bash
# Rust
source "$HOME/.cargo/env"

# 可选：国内 rustup 镜像
export RUSTUP_DIST_SERVER=https://mirrors.ustc.edu.cn/rust-static
export RUSTUP_UPDATE_ROOT=https://mirrors.ustc.edu.cn/rust-static/rustup

# 可选：交叉工具链
export PATH="/path/to/riscv-toolchain/bin:$PATH"
export CROSS_COMPILE=riscv64-unknown-linux-gnu-
```

---

## 8. 与板级运行的关系（K230）

环境装好并编出镜像后：

- 加载 / 入口地址：`0x80200000`
- UART0：`0x91400000`，波特率 `115200`
- 串口预期输出：

```text
BootSYS / K230
UART0 @ 0x91400000, baud 115200
ready - RISC-V low-level verification system
```

下载与调试方式取决于你的板端工具（OpenOCD / 厂商下载器 / 既有 BootROM 流程），本文件只覆盖 **主机侧编译环境**。
