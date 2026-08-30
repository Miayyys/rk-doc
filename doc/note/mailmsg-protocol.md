---
title: "MailMsg 协议设计说明"
type: note
status: draft
created: 2026-08-27
updated: 2026-08-30
tags: [rk3588, amp, ipc, mailmsg, shared-memory]
aliases: ["MailMsg protocol"]
related:
  - "[[decision/dec-20260827-001-name-mailmsg-amp-protocol]]"
  - "[[decision/dec-20260828-001-define-mailmsg-priority-reliability]]"
  - "[[decision/dec-20260826-001-select-mailbox0-notification-layer]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
---

# MailMsg 协议设计说明

## 目标与边界

MailMsg 是面向 Linux+Zephyr AMP 的共享内存消息协议。它把消息数据放在预留共享内存中，把“有消息或状态变化”的提示交给可替换的通知后端，从而同时服务于低延迟控制消息和较大量数据交换。

MailMsg 负责消息帧、队列、发布/消费可见性、完整性检查和传输结果反馈；它不负责应用任务调度、队列公平性、业务完成确认或自动重传。重试、丢弃、降级、报警和复位由上层调用者根据协议结果决定。RPMsg 不是当前主数据平面。

## 架构

数据平面是共享内存，通知平面是抽象接口：

```text
生产者写入 frame → 共享内存 ring → 消费者读取 frame
                         ↑
              可替换 notify/receive_irq
```

当前通知后端为 Rockchip `mailbox0`。mailbox 的 `cmd`/`data` 只承载通知所需的 `u32` 元数据，不承载消息正文；因此 doorbell 是提示，不是消息本身。协议接口可抽象为 `notify(priority)` 和 `receive_irq(priority)`，后端可替换而不改变队列数据语义。

### Endpoint 抽象

通用 endpoint 层 `src/mailmsg/mailmsg_endpoint.{h,c}` 只负责一次生命周期内绑定 Linux 或 CPU3 角色、维护本端序号、发送和接收。发送的语义是先尝试入队，再调用通知；入队结果与通知结果分开返回。endpoint 不负责调度、线程、重传或业务策略。Zephyr 和 Linux 的正常 `mailmsg_ping`/`mailmsg_response` 路径使用该接口；CRC 注入和不发 doorbell 的队列测试仍是测试专用的直接构帧路径。

Linux 用户态提供四个 priority-scoped 字符设备 `/dev/mailmsg-p0` 到 `/dev/mailmsg-p3`，以 `write` 提交本 priority 消息，并以 `poll`/`read` 获取响应。该入口与旧 sysfs 测试入口不能并行消费同一响应队列；字符设备板端证据覆盖 p0 的 ACK/PONG、p2 的 PONG、p3 TX ring 满时 `write` 返回 `-ENOSPC`，以及每 priority 的独占读代表路径，其他行为见实验记录中的未验证边界。

接收端按 priority 维护独占所有权：第一个 `poll/read` fd 成为该 priority ring 的接收者，第二个接收者返回 `EBUSY`；拥有者关闭后，新 fd 可接管，并可在空 ring 上得到 `EAGAIN`。该语义由 2026-08-30 RAM-only 板端代表测试验证；独占读测试后 p0 `901` 仍返回 ACK `sequence=1 peer_sequence=1 status=0` 和 PONG `sequence=2 value=902`，退出码为 `0`，仅说明该改动未回归 p0 代表路径。发送端仍允许多写者；源码审查显示现有 `data->lock` 对发送路径串行化，但这不是多进程发送压力或业务交付正确性证据。

## 优先级队列

Linux→Zephyr 与 Zephyr→Linux 两个方向各有四个独立队列：

| priority | 名称 | 可靠性 |
| ---: | --- | --- |
| 0 | `critical` | 启用 ACK/NACK |
| 1 | `control` | 启用 ACK/NACK |
| 2 | `normal` | 不发送 ACK/NACK |
| 3 | `best-effort` | 不发送 ACK/NACK，类似 UDP |

每个方向/priority 是单生产者、单消费者（SPSC）ring。每个 ring 有 8 个物理 slot，但保留一个位置用于区分满和空，因此最多同时使用 7 个 slot。四个 priority 彼此独立；一个队列的满、坏帧或通知状态不应被解释为其他队列的状态。

## 帧与可见性

共享内存区头包含 `version`，用于标识当前 ABI；当前候选为 `version=3`，且 `version` 不是每帧字段。V3 的具体字节布局以实现 ABI 为准，每帧包含：

| 字段 | 职责 |
| --- | --- |
| `type` | 区分数据、ACK、NACK 等消息类型 |
| `sequence` | 标识本端消息，用于反馈关联 |
| `length` | 表示 payload 有效长度 |
| `payload[32]` | 承载消息内容的 payload 区域 |
| `crc32` | 检查帧内容完整性 |
| `commit` | 表示帧已按顺序发布，可被消费 |

V1 的 48 B frame 和 V2 的 `version=2`、`crc32`、52 B frame 是历史验证 ABI；当前协议为 V3 draft，版本由共享区头表达。当前模型只有 `commit` 发布标记，不存在 `commit`/`ready` 二选一；它不能替代 CRC32。CRC32 只检查完整性，不能表示应用已经完成处理。

生产者发布顺序为：

```text
写入 frame（含 `commit` 未发布状态）→ cache flush/clean → barrier → 发布 `commit` → notify(priority)
```

消费者收到通知或主动检查后，应执行：

```text
receive_irq(priority)/检查 → barrier → cache invalidate → 读取 `commit` 与 frame → 校验并消费
```

缺少 cache 维护、barrier 或正确发布顺序时，消费者可能看到旧数据或未完整数据。

## 通知结果

通知层区分三态：

- `SENT`：已提交通知。
- `COALESCED`：消息已经成功入队，但对应 priority 的硬件 A2B pending 位已置；后端不调用 `mbox_send_message`，也不占 Linux core TX 队列，直接合并到已有 doorbell，并在状态中以 `tx_ret=-EBUSY` 记录观察原因。该状态不是入队失败。
- `FAILED`：通知后端失败；具体后端的负 errno 保留，供上层诊断。

sysfs 入口的入队结果与通知结果分离：入队成功但通知失败或合并时，入口仍报告入队成功，状态文件记录规范化通知状态和计数。通知层不自动重传。

## 可靠性与错误处理

priority 0/1 收到完整且 CRC 正确的帧后发送 ACK；CRC 或其他有效性错误的帧均立即从所属 ring 消费并释放 slot，priority 0/1 另发送关联原消息 `sequence` 和错误码的 NACK。ACK/NACK 是传输层校验/接收结果，不是应用完成确认。priority 2/3 不发送 ACK/NACK；错误帧直接丢弃，避免阻塞队头。

协议不自动重传。发送端应用根据入队结果、ACK/NACK、`sequence` 和错误码自行选择重传、丢弃、降级、报警或复位。

队列满时，协议/底层应立即、非阻塞地报告 `FULL` 或对应错误，不等待，也不覆盖已有消息；后续处置完全由上层调用者决定。该语义已由 p3 代表路径验证，但并发或压力下的泛化仍待验证。

对用户态字符设备而言，p3 代表路径的满队列表现为 `write` 返回 `-ENOSPC`，由调用者决定重试、丢弃、降级或报警；同一验证中 p3 保持饱和后，p0 的 `write` 仍返回 ACK `sequence=1 peer_sequence=9 status=0` 和业务 PONG `sequence=2 value=802`，退出码为 `0`。这只证明本次 p3→p0 代表路径未互相阻塞，不证明 p1/p2 或全优先级隔离，也不证明自动恢复。

实现还可以提供发送方向的 TX-full 诊断状态（例如计数、priority、消息类型和错误结果）供观察；这只是报告，不改变入队、通知、重试、重排或丢弃策略。发送端应用仍自行决定后续处置。

## 当前实现与证据范围

V3 当前 endpoint 集成的 mailbox0 四通道后端代表路径已完成 RAM-only 板端验证：四个 priority 的正常消息均按其固定策略返回，p0 的 CRC 坏帧返回 NACK 且无 PONG；p0/p1 ACK/NACK、p2 无反馈、p3 正常路径及 queue-full 代表路径，以及 mailbox0 ch0 的 `COALESCED` 也已有相应实验记录。主机 endpoint/protocol/notify/mailbox0 测试和完整 ARM64 Image 构建已通过。上述结果证明代表路径的协议语义，不等于完整产品验证。

尚未由这些实验覆盖的范围包括：并发、高负载、长期稳定性、吞吐性能、更广泛的通道组合、eMMC 持久化、通知异常的压力边界和自动重传。p3 满队列与 p0 跨 priority 隔离已有代表路径证据，但不替代上述范围的验证；详细镜像、校验值、命令输出及逐次排障过程保留在[共享内存 PING 实验记录](../experiment/exp-20260822-001-build-r1-amp-shmem-ping.md)中。

## 参考决策

- [DEC-20260827-001：将 AMP 主协议正式命名为 MailMsg](../decision/dec-20260827-001-name-mailmsg-amp-protocol.md)
- [DEC-20260828-001：定义 MailMsg V3 按优先级固定可靠策略](../decision/dec-20260828-001-define-mailmsg-priority-reliability.md)
- [DEC-20260826-001：选择 mailbox0 四通道作为 AMP 通知层](../decision/dec-20260826-001-select-mailbox0-notification-layer.md)
