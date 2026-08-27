---
title: "DEC-20260827-001 将 AMP 主协议正式命名为 MailMsg"
type: decision
status: active
created: 2026-08-27
updated: 2026-08-27
tags: [rk3588, amp, ipc, mailmsg]
related:
  - "[[decision/dec-20260826-001-select-mailbox0-notification-layer]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
---

# DEC-20260827-001 将 AMP 主协议正式命名为 MailMsg

## 背景与约束

Linux 与 Zephyr AMP 需要一套以自定义共享内存为主、通知后端可替换的主数据面协议。此前已决定当前通知后端采用 mailbox0 四通道，但协议尚未完成新的 Linux↔Zephyr 板端闭环验证。用户现将该协议正式命名为 **MailMsg**。

## 候选方案

| 方案 | 优点 | 缺点 | 风险 | 验证情况 |
| --- | --- | --- | --- | --- |
| MailMsg：共享内存 + 可替换通知层 | 数据面与门铃后端解耦，适合四优先级队列 | 需要自行完成协议集成、同步和错误处理 | 新协议板端闭环尚未验证 | 源码实现推进中，待验证 |
| RPMsg 作为主路径 | 生态和现成抽象较多 | 不符合当前主数据面与高频数据交换定位 | 引入额外依赖和路径耦合 | 不采用主路径；仅作未来参考 |

## 决定

正式将自定义共享内存+可替换通知层协议命名为 **MailMsg**，作为 AMP 主数据面。MailMsg 仅维护四个独立优先级消息队列：`critical`、`control`、`normal`、`best-effort`，以及可替换的通知接口；当前通知后端为已选定的 mailbox0。

MailMsg 不定义队列消费顺序、配额、公平性、丢弃/合并规则，也不定义应用任务调度；这些属于具体消费者或运行时策略。现有 Zephyr priority 0→3 连续 PING/PONG 仅是测试应用扫描顺序，不能解释为 MailMsg 的协议语义。

RPMsg 不作为 MailMsg 的主路径，仅保留为将来的低频控制或参考方案。当前工作范围是推进 MailMsg 源码实现；尚未完成新的 MailMsg Linux↔Zephyr 板端闭环，不能把既有 mailbox0 基线 doorbell 回归称为 MailMsg 集成验证。

## 影响与复查条件

- 影响：协议、源码、测试和后续文档统一使用 MailMsg 名称；优先实现共享内存队列与 mailbox0 通知后端的最小集成。
- 当前边界：已有一次 RAM-only MailMsg V1 PING/PONG 及一次连续 priority 0–3 测试应用回归；不代表协议定义了消费顺序，也不证明并发、吞吐、队列策略、异常恢复和长期稳定性。
- 复查条件：若共享内存一致性、通知抽象或 mailbox0 集成无法满足需求，再重新评估后端或是否引入 RPMsg；RPMsg 的参考用途不自动升级为主路径。
