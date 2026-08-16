---
title: "Linux 内核启动参数：从 U-Boot 到 Ubuntu"
type: note
status: verified
created: 2026-08-07
updated: 2026-08-07
tags: [rk3588, uboot, kernel, bootargs]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
---

# Linux 内核启动参数：从 U-Boot 到 Ubuntu

## 学习目标

理解 `/proc/cmdline` 是什么，能区分根文件系统、串口控制台、厂商兼容字段与内核行为参数。

## 核心概念

启动加载器通常将一串文本参数交给内核；Linux 启动后可通过 `/proc/cmdline` 查看。它主要反映启动配置，但部分参数也可能来自内核编译时的默认配置。

R1 当前参数节选：

```text
androidboot.storagemedia=emmc androidboot.mode=normal
androidboot.verifiedbootstate=orange
rw rootwait earlycon=uart8250,mmio32,0xfeb50000
console=ttyFIQ0 root=PARTUUID=614e0000-0000
```

| 参数 | 当前含义 | 结论边界 |
| --- | --- | --- |
| `root=PARTUUID=...` | 以内核分区 UUID 指定根文件系统 | 当前被解析为 eMMC 的 `/dev/mmcblk0p6` |
| `rw rootwait` | 以可写方式挂载根分区，并等待存储设备就绪 | 不说明具体文件系统类型 |
| `console=ttyFIQ0` | Rockchip 厂商 FIQ 串口控制台 | 与实际 Debug UART 日志一致 |
| `earlycon=...0xfeb50000` | 在完整串口驱动加载前，以该 MMIO UART 输出早期日志 | 地址本身不能替代接口针脚图 |
| `storagemedia=emmc` | 厂商启动脚本的存储介质提示 | 与 eMMC 证据一致 |
| `androidboot.*` | 为 Android 用户空间保留的启动属性命名空间 | 出现在 Ubuntu 中不表示正在运行 Android |

## 为什么会有 Android 字段

Rockchip 厂商 BSP 往往复用一套 U-Boot、分区和启动脚本来启动 Android 与 Linux。Ubuntu 根文件系统可以沿用同一启动加载器，因此未被 Ubuntu 使用的 `androidboot.*` 字段仍会出现在命令行中。

Android 的 Verified Boot 约定中，`androidboot.verifiedbootstate=orange` 表示“解锁”状态；但在本板 Ubuntu 启动链中，仅凭这个遗留参数不足以证明安全启动是否启用或系统完整性状态。需要后续核对 U-Boot 配置、签名策略和硬件安全配置。

## 实际验证

- 根文件系统挂载源和介质：见 [EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- Android Verified Boot 的字段定义：[AOSP Boot flow](https://source.android.com/docs/security/features/verifiedboot/boot-flow)，访问于 2026-08-07。

## 易错点

- 不要把 `androidboot.*` 当作“当前系统是 Android”的证据。
- 不要把 `orange` 单独当作本板 Ubuntu 安全状态的最终结论。
- `root=PARTUUID=...` 与 `/dev/mmcblk0p6` 不冲突：前者是稳定分区标识，后者是内核运行时的设备名。
