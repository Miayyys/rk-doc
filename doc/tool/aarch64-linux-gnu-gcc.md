---
title: "AArch64 GNU 交叉编译器"
type: tool
status: verified
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, arch-linux, cross-compile, uboot, toolchain]
aliases: ["aarch64-linux-gnu-gcc", "AArch64 cross compiler"]
related:
  - "[[environment/software]]"
  - "[[status/current]]"
  - "[[note/uboot-fit-image]]"
---

# AArch64 GNU 交叉编译器

## 用途与系统位置

`aarch64-linux-gnu-gcc` 在 Arch Linux 主机上生成 AArch64（64 位 ARM）目标代码。它可用于后续的 U-Boot、Linux 内核和目标用户程序构建；当前仅验证编译器身份，不构建或写入任何板端组件。

## 安装与版本证据

- 安装来源：Arch Linux `extra` 仓库包 `aarch64-linux-gnu-gcc`。
- 已安装包/命令版本：`16.1.0-1` / GCC 16.1.0。
- 安装时间：2026-08-09T21:59:57+08:00；安装原因：显式安装；包签名已验证。
- 依赖：`aarch64-linux-gnu-binutils` 与 `aarch64-linux-gnu-glibc`。
- 与厂商日志的差异：当前 U-Boot 日志记录其构建器为 `aarch64-none-linux-gnu-gcc` 10.3.1；前缀和版本均不同。当前工具链适合学习和一般构建，不保证产生与厂商二进制一致的结果。

## 已验证的最小用法

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588`。

```fish
aarch64-linux-gnu-gcc -dumpmachine
aarch64-linux-gnu-gcc --version
```

实际输出：

```text
aarch64-linux-gnu
aarch64-linux-gnu-gcc (GCC) 16.1.0
```

`-dumpmachine` 输出目标三元组，用于确认生成目标是 AArch64 GNU/Linux；`--version` 只查询版本。二者均不读取开发板，也不产生目标文件。

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
pacman -Q aarch64-linux-gnu-gcc
pacman -Qi aarch64-linux-gnu-gcc
```

实际结果：已安装包为 `aarch64-linux-gnu-gcc 16.1.0-1`；包元数据记录其于 2026-08-09T21:59:57+08:00 显式安装、安装后大小为 410.87 MiB，且经签名验证。该查询仅读取本机包数据库。

## 权限、影响与边界

- 权限要求：查询与普通目录中的编译不需要 root；安装由包管理器决定。
- 影响范围：当前验证只读取主机程序信息。
- 已验证范围：AArch64 目标三元组和 GCC 版本查询。
- 未验证用法：编译 U-Boot、内核、设备树或用户程序；任何构建产物烧录到 eMMC。

## 关联记录

- 当前阶段和下一步：[当前状态](../status/current.md)。
- 当前厂商 U-Boot/FIT 证据：[FIT 笔记](../note/uboot-fit-image.md)。
