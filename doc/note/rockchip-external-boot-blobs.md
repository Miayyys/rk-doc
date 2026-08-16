---
title: "Rockchip 外部启动载荷与 Binman"
type: note
status: verified
created: 2026-08-11
updated: 2026-08-11
tags: [rk3588, uboot, binman, boot-chain]
aliases: [TPL, BL31, ARM Trusted Firmware]
related:
  - "[[experiment/exp-20260809-004-acquire-upstream-uboot-source]]"
  - "[[issue/issue-20260811-002-uboot-missing-external-boot-blobs]]"
  - "[[note/uboot-fit-image]]"
---

# Rockchip 外部启动载荷与 Binman

## 学习目标

理解 U-Boot 能编译却不能形成可启动 Rockchip 镜像时，怎样从 Binman 报错倒查缺少的启动阶段。

## 前置知识

- FIT 是可包含多个启动载荷及其校验信息的容器，见[U-Boot FIT 镜像](uboot-fit-image.md)。
- `defconfig` 展开为 `.config` 后，条件编译选项决定哪些镜像节点参与打包。

## 核心概念

- **TPL**：在此 EVB 配置中是外部 DDR 初始化载荷。DRAM 尚不可用时，后续 SPL/U-Boot 不能按通常方式运行。
- **BL31**：ARM Trusted Firmware 的 EL3 固件阶段；它在进入普通世界的 U-Boot/Linux 前提供安全监控层服务。
- **OP-TEE/BL32**：安全世界载荷。本次 Binman 描述把 `tee-os` 标为 `optional`；这只说明该打包节点的必需性，不等于它对具体系统用途不重要。
- **Binman**：U-Boot 的镜像打包工具。它根据打包 DTS 和 `.config` 组合内部构建产物与外部二进制，而不会自动猜测适合板卡的 DDR 固件。

资料记载：U-Boot 的 Rockchip 板级文档说明 ARM64 Rockchip 镜像需要 TF-A；若特定 SoC 的 TF-A 未公开，可使用 Rockchip 提供的 BL31。文档将 `BL31` 与 `ROCKCHIP_TPL` 作为环境变量输入；其 RK3588 EVB 示例使用 `rk3588_bl31_v1.33.elf` 和 `rk3588_ddr_lp4_2112MHz_lp5_2736MHz_v1.09.bin`。这些名称只说明 EVB 示例的输入，不适用于 R1 的选择结论。

## 工作流程

```text
BootROM → TPL（DDR 初始化）→ SPL → BL31（EL3）→ [OP-TEE / BL32] → U-Boot（BL33）→ Linux
```

本次上游 EVB 构建的 `.config` 已验证 `CONFIG_ARM64=y`、`CONFIG_ROCKCHIP_EXTERNAL_TPL=y`、`CONFIG_SPL_ATF=y`。因此 Binman DTS 中的外部 TPL 和 AArch64/BL31 条件节点会生效。

## 实际验证

学习者重建时已越过 `pylibfdt`，随后 Binman 明确报告缺少 `rockchip-tpl` 与 `atf-bl31`；源码中的 `rockchip-u-boot.dtsi` 又显示 TPL 条件节点、BL31 的 `split-elf` 节点及 optional TEE 节点。完整证据见[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。

这只验证了**上游 EVB 学习构建**的条件和缺失输入。R1 运行时 DTS 是 EVB4 LP4X，不可据此选取或烧录任意 EVB/rkbin 文件。

进一步验证：上游默认 DTS 根节点是 `rockchip,rk3588-evb1-v10`，R1 运行时根节点是 `rockchip,rk3588s-evb4-lp4x-v10`；二者只共同列出 `rockchip,rk3588`。这个共有项是通用 SoC 兼容匹配，不能消除板级差异。

## 关联问题

[ISSUE-20260811-002](../issue/issue-20260811-002-uboot-missing-external-boot-blobs.md) 用此分层区分“主机编译问题”与“缺少外部启动载荷”，避免把 TPL、BL31、OP-TEE 混为一个文件。

## 易错点

- TPL、SPL、BL31 和 U-Boot 不是同一阶段，也不能凭同为 RK3588 就互换。
- 构建成功不等于 R1 可启动；适配板级设备树、DDR 参数及恢复方案仍是独立工作。

## 总结

- Binman 的缺失项是启动链输入，而非普通 C 源码编译错误。
- `.config` 能解释“为什么需要”，不能证明“哪个外部二进制适合 R1”。
- 先核验来源、板型、DRAM 参数、版本、大小和校验值，才讨论下载或使用外部载荷。

## 参考资料

- U-Boot v2026.07 源码：`arch/arm/dts/rockchip-u-boot.dtsi`，本地学习基线，2026-08-11 阅读。
- U-Boot v2026.07 文档：`doc/board/rockchip/rockchip.rst`，TF-A/TPL 小节，本地学习基线，2026-08-11 阅读。
