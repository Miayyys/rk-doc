---
title: "U-Boot SPL 启动设备顺序与 /chosen"
type: note
status: verified
created: 2026-08-10
updated: 2026-08-10
tags: [rk3588, uboot, spl, device-tree, boot]
aliases: ["spl-boot-order", "same-as-spl"]
related:
  - "[[experiment/exp-20260809-004-acquire-upstream-uboot-source]]"
  - "[[note/uboot-fit-image]]"
  - "[[status/current]]"
---

# U-Boot SPL 启动设备顺序与 /chosen

## 学习目标

能够说明 `/chosen/u-boot,spl-boot-order` 如何把设备树中的候选启动来源交给 Rockchip SPL，并知道何时应停止追踪实现细节。

## 前置知识

- `/chosen` 是启动阶段的配置节点，不是物理硬件节点。
- DTS 的 `&label` 是对设备树节点的引用；SPL 是 U-Boot 的早期阶段。

## 核心概念

`u-boot,spl-boot-order` 是 U-Boot 专用的有序设备列表。元素可为节点引用、完整路径或 alias。特殊项 `"same-as-spl"` 请求在该位置使用 SPL 的实际启动来源；是否可用取决于具体 SoC/板级实现，且可能与显式项重复。

对上游 RK3588 EVB1，配置为：

```dts
u-boot,spl-boot-order = "same-as-spl", &sdhci;
```

这表达“先考虑 SPL 来源，再考虑 `sdhci` 节点”的通用意图，不证明 R1 厂商镜像采用相同顺序。

## 工作流程

```text
/chosen 的有序条目
  → board_boot_order()
  → same-as-spl 时查询 board_spl_was_booted_from()
  → alias / 路径解析为 FDT 节点
  → spl_node_to_boot_device()
  → spl_boot_list[] 中的 BOOT_DEVICE_* 顺序
```

上游 Rockchip v2026.07 在没有 FDT、没有 `/chosen` 或没有有效匹配时回退到 `spl_boot_device()`；MaskROM USB 且满足条件时可优先加入 RAM 启动项。

## 实际验证

**已验证**：在固定的上游 U-Boot v2026.07 源码中，EVB1 的 U-Boot 专用 DTS 属性、同树文档、Rockchip `board_boot_order()` 以及两个关键转换函数的入口均已直接阅读。证据见[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。

**待验证**：R1 当前厂商 U-Boot 是 2017.09；它是否使用同一 DTS 属性、函数实现和设备映射不能由上游 v2026.07 推出。

## 易错点

- `same-as-spl` 不是固定等于 eMMC、SD 或 USB；它由运行时来源函数决定。
- `spl-boot-order` 不等于 Linux 内核的启动顺序，也不等于后续 Linux FIT 的加载顺序。
- `&sdhci` 的物理设备身份必须查看对应节点，不能仅凭名称断言。

## 总结

- `/chosen` 可携带 U-Boot 启动策略。
- 设备树属性、文档和解析代码应一起阅读。
- 当前已完成理解启动设备顺序所需的纵向证据链；只有修改启动链或定位来源异常时才继续读函数细节。

## 参考资料

- [U-Boot v2026.07 上游源码档案](../resource/u-boot-v2026-07-upstream-source.md)，U-Boot 项目，v2026.07，2026-08-10 阅读；其中 `doc/device-tree-bindings/chosen.txt` 与 `arch/arm/mach-rockchip/spl-boot-order.c`。
