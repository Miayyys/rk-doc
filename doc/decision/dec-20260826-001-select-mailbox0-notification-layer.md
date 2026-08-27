---
title: "DEC-20260826-001 选择 mailbox0 四通道作为 AMP 通知层"
type: decision
status: active
created: 2026-08-26
updated: 2026-08-27
tags: [rk3588, amp, mailbox, ipc]
related:
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[note/amp-shared-memory-notification-abstraction]]"
---

# DEC-20260826-001 选择 mailbox0 四通道作为 AMP 通知层

## 背景与约束

AMP 需要在 Linux 与 CPU3/Zephyr 之间传递低延迟通知，同时保留共享内存承载大量数据的能力。2026-08-26 的 R1 RAM-only candidate 已验证 mailbox0 的四个通道各自完成一次 Linux→CPU3→Linux 闭环；该证据不涉及 eMMC 持久化，也不代表高吞吐、并发或长期稳定性已经验证。12 控制器候选在 mailbox1 ch0 request 时于 `rockchip_mbox_startup` 触发 synchronous external abort，已回退 mailbox0 四通道。当前内核/Zephyr mailbox0 四通道版本已主机编译成功；协议代码 `src/amp-protocol/r1_amp_protocol.h/.c` 仅通过主机 C 单元测试，尚未集成或上板。

## 候选方案

| 方案 | 优点 | 缺点 | 风险 | 验证情况 |
| --- | --- | --- | --- | --- |
| mailbox0 四通道 | 已有四通道双向闭环证据；可提供独立通知通道 | 依赖当前硬件路由和控制器 | 固件所有权、压力边界及持久化路径未验证 | **已验证**：ch0–ch3 各一次闭环 |
| mailbox1/2 | 可能提供其他控制器资源 | 当前不可用于本项目；mailbox1 在 U-Boot 直接 MMIO 访问触发 external abort，mailbox2 未形成通知闭环 | 继续探索有硬件和固件风险 | **不可用/停止探索** |
| SGI | 通知延迟可能较低 | 需要确认安全状态、GIC 路由和双方接口 | 路由与权限契约未知 | 待验证 |
| 轮询 | 实现简单、后端依赖少 | 延迟和 CPU 占用不利 | 不适合作为默认通知机制 | 未采用 |

## 决定

采用已验证的 **mailbox0 四通道** 作为当前 AMP 通知层，优先级从高到低定义为：`critical`、`control`、`normal`、`best-effort`。共享内存队列承载实际数据，通知层只负责提示有数据或状态变化；抽象接口暂定为 `notify(priority)` 与 `receive_irq(priority)`。

协议 ABI 不绑定具体门铃后端。未来可在不改变协议 ABI 的前提下替换为 mailbox、SGI 或 poll。mailbox1/2 当前不作为探索目标；其中 mailbox1 的实际 MMIO 访问已触发 external abort，停止继续访问。

## 影响与复查条件

- 影响：后续 Linux 与 CPU3 集成优先接入 mailbox0 ch0–ch3，并把共享内存队列与通知后端分离设计。
- 当前边界：四通道闭环证据来自 RAM-only candidate；主机构建和 C 单元测试不等于上板证据。尚未验证并发、高吞吐、超时/异常恢复、长期负载、eMMC 启动或 mailbox1/2 通知。
- 复查条件：若 mailbox0 在集成构建、压力测试或固件共存验证中失败，再评估 SGI 或轮询后端；不得因未验证的协议代码而提前宣称实现完成。
