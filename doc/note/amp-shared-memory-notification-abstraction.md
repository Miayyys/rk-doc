---
title: "AMP 共享内存队列与通知后端分层"
type: note
status: draft
created: 2026-08-26
updated: 2026-08-27
tags: [rk3588, amp, ipc, shared-memory, mailbox]
aliases: ["AMP notification abstraction"]
related:
  - "[[decision/dec-20260826-001-select-mailbox0-notification-layer]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
---

# AMP 共享内存队列与通知后端分层

## 学习目标

能够区分 AMP IPC 中“数据放在哪里”和“如何提示对端”，并说明为什么通知后端不应写入协议 ABI。

## 前置知识

- 共享内存、cache maintenance 与 Linux/Zephyr CPU 隔离。
- 关联笔记：[RKLLM CPU mask 配置](rkllm-cpu-mask-configuration.md)。

## 核心概念

共享内存队列负责保存消息数据、序号和状态；通知后端只负责让对端尽快检查队列。这样可以把高频、大量数据交换与低带宽 doorbell 分开，降低协议对硬件中断控制器的耦合。

当前设计以 `notify(priority)` 发出通知，以 `receive_irq(priority)` 接收并转入对应队列。MailMsg 只维护四个独立优先级消息队列：`critical`、`control`、`normal`、`best-effort`；这些优先级是队列标识，不等同于硬件 IRQ 号或 Linux mailbox channel 号。协议不规定队列消费顺序、配额、公平性、丢弃/合并策略或应用任务调度。

## 工作流程

```text
发送方：写共享内存队列 → cache clean/同步 → notify(priority)
                                              ↓
接收方：receive_irq(priority) → cache invalidate/同步 → 读队列并确认
```

当前 R1 原型选择 mailbox0 ch0–ch3 承担通知。未来可替换为 SGI 或轮询，只要保留上述抽象和四个独立队列，协议 ABI 不需要改变。具体消费者可以自行决定消费顺序和调度策略。

## 实际验证

**已验证**：`EXP-20260822-001` 记录的 R1 RAM-only candidate 在 mailbox0 ch0–ch3 各完成一次 Linux→CPU3→Linux 闭环，并观察到 CPU3 路由及独立 A2B/B2A 回执。

**已验证（主机侧）**：`src/amp-protocol/r1_amp_protocol.h/.c` 的 groundwork 已通过主机 C 单元测试；当前内核/Zephyr mailbox0 四通道版本已主机编译成功。

**待验证（板端）**：上述主机结果尚不代表 Linux↔CPU3 集成、RAM-only FIT 运行或上板 mailbox 通信；共享内存队列的并发、吞吐、超时、异常恢复与长期稳定性也未验证。

**边界说明**：`EXP-20260822-001` 中 priority 0→3 的连续 PING/PONG 是测试应用的扫描顺序，仅证明该次测试的独立映射和消息结果，不能上升为 MailMsg 的消费顺序或调度规则。

## 关联问题

暂无。

## 易错点

- mailbox 通道是通知传输资源，不是共享内存数据队列本身。
- 已验证 mailbox0 闭环不能推出 mailbox1/2 可安全使用；mailbox1 直接 MMIO 访问曾触发 external abort，当前停止探索。
- 非负 `mbox_send_message` 返回值是提交 cookie，不应当解释为错误码；负值才表示失败。

## 总结

- 数据面使用共享内存队列，控制面使用通知后端。
- 通知优先级属于协议语义，不直接等同于硬件通道编号。
- 当前已验证的通知后端是 mailbox0 四通道。
- 后端可替换为 mailbox、SGI 或 poll，而不改变协议 ABI。
- 新增协议代码仍处于未集成、未板端运行验证状态。
- 协议 groundwork 已有主机 C 单元测试证据，但仍待 Linux↔CPU3 集成和上板。
- MailMsg 不定义消费顺序、配额/公平性、丢弃合并或应用任务调度；这些由消费者/运行时决定。

## 参考资料

- [EXP-20260822-001：构建 R1 Linux-Zephyr 共享内存 PING 原型](../experiment/exp-20260822-001-build-r1-amp-shmem-ping.md)，2026-08-26。
- [DEC-20260826-001：选择 mailbox0 四通道作为 AMP 通知层](../decision/dec-20260826-001-select-mailbox0-notification-layer.md)，2026-08-26。
