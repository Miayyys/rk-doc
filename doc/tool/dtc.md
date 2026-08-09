---
title: "Device Tree Compiler（dtc）"
type: tool
status: verified
created: 2026-08-07
updated: 2026-08-07
tags: [rk3588, arch-linux, devicetree, tool]
aliases: ["Device Tree Compiler", "DTC", "libfdt"]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[environment/software]]"
---

# Device Tree Compiler（dtc）

## 用途与系统位置

`dtc` 用于在设备树源码（DTS）与二进制设备树（DTB/FDT）之间转换。当前只在 Arch Linux 主机验证可用；板端 Root Shell 的 `PATH` 中没有可直接调用的 `dtc`。它将用于分析 R1 当前运行时 FDT，不能据此改变板端设备树或启动配置。

## 安装与版本证据

- 安装来源：Arch Linux 已安装包 `dtc`；上游 URL 为 `https://www.devicetree.org/`。
- 已安装版本：`1:1.8.1-1`；命令版本：DTC v1.8.1。
- 安装日期：2026-08-07T20:53:39+08:00。
- 安装原因：单独指定安装。
- 打包者：George Rawlinson `<grawlinson@archlinux.org>`。
- 安装后大小：685.73 KiB；运行时依赖：`bash`、`glibc`、`libyaml`；可选 Python 绑定已安装。
- 完整命令输出见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 已验证的最小用法

执行端：Arch Linux 主机 Shell；当前目录：`~`。

```sh
dtc -v
```

实际输出：

```text
Version: DTC v1.8.1
```

该命令只输出工具版本，不读取或修改 DTB。

## 权限、影响与边界

- 权限要求：版本查询不需要 root；后续读取板端 `/sys/firmware/fdt` 时由板端权限决定。
- 影响范围：`dtc -v` 仅查询；将来反编译应把输入 DTB 与输出 DTS 写入主机工作目录，不覆盖原始 FDT。
- 已验证范围：Arch 主机上的版本查询和包元数据查询。
- 未验证用法：DTB 复制、DTB→DTS 反编译、DTS→DTB 编译、覆盖或烧录设备树。

## 关联记录

- 运行时 FDT 与 `model`/`compatible`：[设备树笔记](../note/device-tree-model-and-compatible.md)。
- 工具安装和版本证据：[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
