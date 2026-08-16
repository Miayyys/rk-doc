---
title: "ISSUE-20260807-001 MaskROM 与后续 Linux 启动状态不一致"
type: issue
status: active
created: 2026-08-07
updated: 2026-08-07
tags: [rk3588, r1, boot, maskrom]
related:
  - "[[experiment/exp-20260805-001-identify-rockusb-device]]"
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
---

# ISSUE-20260807-001 MaskROM 与后续 Linux 启动状态不一致

## 现象与影响

- **已验证（2026-08-05）**：`rkdeveloptool ld` 报告该板为 `Maskrom`。
- **已验证（2026-08-07）**：重新上电后，Debug UART 出现 Linux 内核日志并进入 `root@R1:~#`。
- 这说明 MaskROM 并非当前板卡唯一、持续的启动状态；在知道进入条件前，不应以“eMMC 没有系统”作为烧录依据。

## 已知证据

- [EXP-20260805-001](../experiment/exp-20260805-001-identify-rockusb-device.md)：MaskROM 枚举。
- [EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)：Linux 启动及交互 Shell。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| 之前曾通过按键或连接顺序强制进入 MaskROM | 两次启动状态不同 | 回忆并记录两次上电时按键和线材状态 | 待验证 | 保留 |
| 当前 eMMC 含可用根文件系统 | Linux 已成功启动 | `findmnt -no SOURCE /` 与 MMC 类型检查 | eMMC 根文件系统为 `/dev/mmcblk0p6` | 已确认 |
| 启动存在偶发失败 | 尚无重复测试 | 在不改变按键与介质条件下多次观察串口启动结果 | 待验证 | 保留 |

## 根因

未知。当前证据不足以判断是按键、启动介质、供电、线材还是其他条件造成差异。

## 安全限制与下一步

在完成系统和启动介质识别前，不擦除或烧录 eMMC。下一步是从正在运行的 Linux 读取 `mmcblk0` 的设备类型，再逐步识别启动链。
