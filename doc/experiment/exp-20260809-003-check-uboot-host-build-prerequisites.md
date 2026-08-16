---
title: "EXP-20260809-003 检查 U-Boot 主机构建前置工具"
type: experiment
status: verified
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, uboot, arch-linux, cross-compile]
related:
  - "[[environment/software]]"
  - "[[tool/aarch64-linux-gnu-gcc]]"
  - "[[tool/dtc]]"
  - "[[status/current]]"
---

# EXP-20260809-003 检查 U-Boot 主机构建前置工具

## 目标

确认 Arch Linux 主机是否具备开始一次“仅配置和编译、不烧录”的 U-Boot 源码学习所需的基础命令。

## 环境与前置条件

- 执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588`。
- 硬件状态：R1 已能正常启动 Linux；本实验不访问板子。
- 已知工具：`aarch64-linux-gnu-gcc` 的目标三元组和版本已单独验证，见[工具记录](../tool/aarch64-linux-gnu-gcc.md)。

## 风险与恢复

- 影响范围：仅查询主机 `PATH`。
- 备份：不适用，不创建或修改文件。
- 恢复方法：不适用。

## 步骤与证据

### 步骤 1：盘点基础构建命令

目的：分别确认构建器、解析器生成工具、算术工具、加密工具、Python、设备树编译器和 AArch64 交叉编译器是否可调用。预期：每个名称输出一个绝对路径；缺失项会输出 `missing`。

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588`。

```fish
for tool in make bison flex bc openssl python dtc aarch64-linux-gnu-gcc
    command -v $tool; or echo "$tool: missing"
end
```

实际输出（学习者提供；退出码未单独记录）：

```text
/usr/bin/make
/usr/bin/bison
/usr/bin/flex
/usr/bin/bc
/usr/bin/openssl
/usr/bin/python
/usr/bin/dtc
/usr/bin/aarch64-linux-gnu-gcc
```

观察：所有受检名称均解析到 `/usr/bin` 下的可执行文件，没有 `missing` 项。该结果只证明这些命令可被当前 Shell 找到；尚未验证特定 U-Boot 版本的完整依赖、实际配置或编译成功。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 构建与解析基础工具可调用 | `make`、`bison`、`flex`、`bc` 均有路径 | 均为 `/usr/bin/...` | 通过 |
| 主机辅助工具可调用 | `openssl`、`python`、`dtc` 均有路径 | 均为 `/usr/bin/...` | 通过 |
| AArch64 交叉编译器可调用 | `aarch64-linux-gnu-gcc` 有路径 | `/usr/bin/aarch64-linux-gnu-gcc` | 通过 |

## 结论

已验证当前主机具备开始 U-Boot 源码获取、配置和首次构建尝试的基础命令。仍未验证任何源码版本、板级 defconfig、构建产物或与 R1 厂商固件的兼容性；因此不得将本结果视为可烧录条件。

## 关联知识与问题

- 关联工具：[AArch64 GNU 交叉编译器](../tool/aarch64-linux-gnu-gcc.md)、[`dtc`](../tool/dtc.md)。
- 关联启动观察：[FIT 笔记](../note/uboot-fit-image.md)。

## 后续行动

- [ ] 获取可追溯的上游 U-Boot 源码，只用于主机阅读和构建，不写入 R1。
