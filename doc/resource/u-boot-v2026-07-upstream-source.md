---
title: "U-Boot v2026.07 上游源码"
type: note
status: active
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, uboot, source, resource]
related:
  - "[[experiment/exp-20260809-004-acquire-upstream-uboot-source]]"
  - "[[experiment/exp-20260809-003-check-uboot-host-build-prerequisites]]"
  - "[[note/uboot-fit-image]]"
---

# U-Boot v2026.07 上游源码

## 身份与来源

- 项目：Das U-Boot 上游源码。
- 获取地址：`https://github.com/u-boot/u-boot.git`；[上游文档](https://docs.u-boot.org/en/latest/build/source.html) 将其列为源码镜像。
- 发布标签：`v2026.07`；[发布周期](https://docs.u-boot.org/en/stable/develop/release_cycle.html)记载该稳定版于 2026-07-06 发布。
- 已检出提交：`ece349ade2973e220f524ce59e59711cc919263f`。
- 本地位置：`src/u-boot-upstream/`（父仓库已忽略该外部 checkout，不作为本仓库源码提交）。
- 获取日期：2026-08-09；精确克隆完成时间未保存。
- 许可证：上游仓库标注 GPL-2.0+；本地许可证文件内容尚未单独核对。

## 适用范围与边界

这是用于学习主线 U-Boot 构建系统和通用 RK3588 支持的上游基线，不是当前 R1 厂商 U-Boot 的已验证源码。当前板端 U-Boot 日志为厂商修改的 2017.09 系列，构建器也不同；不能把本源码的构建产物直接烧录到 R1。

## 完整性与验证状态

- **已验证**：本地 Git 状态干净、`git describe --tags --exact-match` 输出 `v2026.07`，且 HEAD 为上述提交。
- **待确认**：未验证 tag 的 GPG 签名；未计算工作树归档的 SHA-256。Git 提交 ID是当前可复现的源码身份，不等同于 SHA-256 校验文件。
- 文件大小：工作树不是单一发布文件，尚未统计。

## 关联

- 获取和身份验证：[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- 主机构建依赖：[EXP-20260809-003](../experiment/exp-20260809-003-check-uboot-host-build-prerequisites.md)。
