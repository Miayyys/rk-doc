---
title: "DEC-20260828-001 定义 MailMsg V3 按优先级固定可靠策略"
type: decision
status: active
created: 2026-08-28
updated: 2026-08-29
tags: [rk3588, amp, ipc, mailmsg, protocol]
related:
  - "[[decision/dec-20260827-001-name-mailmsg-amp-protocol]]"
  - "[[note/mailmsg-protocol]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
---

# DEC-20260828-001 定义 MailMsg V3 按优先级固定可靠策略

## 背景与约束

MailMsg V2 已完成固定 52 B frame、`crc32` 及一次正常路径和损坏帧检测/拒收的 RAM-only 验证，但 V2 没有冻结 ACK/NACK、错误帧释放 slot 或重传处置语义。需要在实现前明确四个 priority 的服务契约，同时保留协议层与应用层之间的责任边界。

本决定定义 **V3 当前 draft ABI 的可靠性策略**。p0/p1 ACK/NACK、p2/p3 无反馈及队列满代表路径已有 RAM-only 板端验证；并发压力、长期稳定性、自动重传和持久化仍待验证。V2 的已验证事实仍只适用于 V2，不得用本决定回溯解释或改写 V2 运行结果。

## 候选方案

| 方案 | 优点 | 缺点 | 风险 | 验证情况 |
| --- | --- | --- | --- | --- |
| 每个 priority 固定可靠策略（V3） | 让关键/控制消息获得明确反馈；普通消息避免 ACK 开销；错误责任清晰 | 发送端应用需要自行处理 ACK/NACK 和失败策略 | 并发压力、长期稳定性及持久化仍需测试 | **p0/p1/p2/p3 及队列满代表路径已验证；其余待验证** |
| 所有 priority 统一 ACK/NACK | 语义简单、反馈一致 | 增加普通/尽力而为流量的开销与耦合 | 反馈拥塞可能影响数据面 | 未采用 |
| 所有 priority 均无 ACK/NACK | 实现简单、类似 UDP | 无法为关键/控制消息提供协议反馈 | 可靠性完全外移，故障难以区分 | 未采用 |

## 决定

MailMsg V3 采用“每个 priority 固定可靠策略”:

| priority | 名称 | ACK/NACK | V3 目标语义 |
| ---: | --- | --- | --- |
| 0 | `critical` | 启用 | 可靠队列；收到完整且 CRC 正确的帧后发送 ACK |
| 1 | `control` | 启用 | 可靠队列；收到完整且 CRC 正确的帧后发送 ACK |
| 2 | `normal` | 禁用 | 无 ACK/NACK，按无反馈数据面处理 |
| 3 | `best-effort` | 禁用 | 无 ACK/NACK，语义类似 UDP |

对所有 priority，CRC 错误或其他帧有效性错误都应立即消费该 slot 并丢弃帧，避免错误帧阻塞队头。对于启用可靠反馈的 priority 0/1，错误处理同时发送 NACK；NACK 必须关联原消息的 `sequence` 和错误码。priority 2/3 不发送 ACK/NACK，即使帧有效或无效。

ACK/NACK 只报告接收与校验结果，不触发协议层自动重传。发送端应用根据 ACK/NACK、关联的 `sequence` 和错误码自行决定重传、丢弃、降级或复位；这些动作不属于 MailMsg 的自动行为。

上述语义是 V3 当前 draft 契约；p0/p1 ACK/NACK、p2/p3 无反馈及队列满代表路径已落实并完成板端验证，并发压力、长期稳定性、自动重传和持久化仍未验证。ACK/NACK 的具体消息类型、承载方向、错误码枚举及与 ring 状态字段的布局以 V3 实现为准，并保持与 V2 历史验证记录区分。

队列满时 MailMsg/底层应立即、非阻塞地返回 `FULL` 或对应错误，不等待，也不覆盖已有消息；重试、丢弃、降级或报警完全由上层调用者决定。RAM-only p3 代表路径已验证运行期 8 个物理 slot、7 个可用 slot，第 8 次写入立即返回空间不足且已入队帧随后可消费；该结果不覆盖并发压力或长期稳定性。

## 影响与复查条件

- 影响：后续 MailMsg 实现和测试按 priority 0/1 可靠、priority 2/3 无反馈的固定契约设计；错误帧必须释放 slot，可靠 priority 还需回送关联 sequence/错误码的 NACK。
- 当前边界：V3 为当前 draft ABI，p0/p1 ACK/NACK、p2/p3 无反馈及队列满代表路径已有板端验证，但不代表并发压力、长期稳定性、自动重传或持久化已验证；V2 正常路径、CRC 检测/拒收及 priority ring 隔离证据保持原有范围。
- 何时需要重新评估：若 ACK/NACK 的共享内存承载、队列空间、错误码关联或压力测试显示该契约无法满足实时性/吞吐要求，再重新评估 priority 划分或反馈机制；在此之前不得自行把协议扩展为自动重传。
