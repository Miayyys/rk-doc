---
title: "MailMsg 通信协议设计"
type: note
status: draft
created: 2026-08-27
updated: 2026-08-27
tags: [rk3588, amp, ipc, mailmsg, shared-memory]
aliases: ["MailMsg protocol"]
related:
  - "[[decision/dec-20260827-001-name-mailmsg-amp-protocol]]"
  - "[[decision/dec-20260826-001-select-mailbox0-notification-layer]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
---

# MailMsg 通信协议设计

## 学习目标

能够根据本设计说明 MailMsg V2 的固定消息帧、队列拓扑、发布/消费可见性和通知契约，并区分 V1 历史基线、当前 V2 设计与板端待验证项。

## 前置知识

- 共享内存与 cache maintenance。
- SPSC ring 与 doorbell 通知。

## 协议定位与非目标

MailMsg 是基于共享内存的 AMP 通信协议：共享内存承载消息数据，通知后端仅提示对应队列有可处理内容。通知后端可替换，当前选择 mailbox0。

协议不规定消费者的队列消费顺序、配额、公平性、丢弃/合并策略或应用任务调度；这些不属于 MailMsg ABI。

## 队列与术语

通信拓扑为双向 × 四 priority：Linux→Zephyr 和 Zephyr→Linux 两个方向各有 priority 0–3 队列。每个方向/priority 是一个 SPSC（single-producer, single-consumer）ring；每个 ring 有 8 个物理 slot，可用队列上限为 7，保留一个 slot 区分满与空。

```text
Linux → Zephyr: priority 0 | priority 1 | priority 2 | priority 3
Zephyr → Linux: priority 0 | priority 1 | priority 2 | priority 3
                 每个 ring：8 个物理 slot，最多使用 7 个
```

`slot` 是 ring 中的物理队列位置；`frame` 是 slot 中承载的消息帧。当前设计不把二者混称。

## 固定消息帧与 ABI

MailMsg ABI 当前为 V2，固定 frame 为 52 B，`version=2`，字段包括：

| 字段 | 设计作用 |
| --- | --- |
| `version` | ABI 版本；当前为 `2` |
| `type` | 消息类型 |
| `sequence` | 消息序号 |
| `length` | payload 有效长度 |
| `payload32` | 32 B payload 区域 |
| `commit` / `ready` | 发布/就绪标记 |
| `crc32` | 帧完整性校验字段 |

字段的具体字节偏移和各字段宽度以 V2 实现为准；本设计不在未确认处补写布局细节。此前 V1 固定 frame 为 48 B，是不含 V2 `version=2` 与 `crc32` 的历史基线；V2 当前固定 frame 为 52 B。

| ABI | 固定 frame 大小 | 版本/完整性字段 | 状态 |
| --- | ---: | --- | --- |
| V1（历史基线） | 48 B | 不含 V2 `version=2` 与 `crc32` | 曾有 RAM-only PING/PONG evidence |
| V2（当前） | 52 B | `version=2`、`crc32` | 主机单元测试与 Linux/Zephyr V2 构建通过；板端仅验证一次 ch0 CRC 正常路径 |

`commit`/`ready` 是必要的发布标记，用来表示 frame 已按协议顺序发布；它不是 CRC，也不能替代 CRC32。CRC32 是独立的帧完整性字段。

## 发布、消费与内存可见性

生产者按以下顺序发布消息：

```text
写 frame 内容 → cache flush/clean → barrier → 发布 commit/ready → notify(priority)
```

消费者收到通知或主动检查后，按以下顺序读取：

```text
receive_irq(priority)/检查 → barrier → cache invalidate → 读取 commit/ready 与 frame → 校验
```

共享内存不是网络发包，因此也不会自动消除并发或缓存观察问题。生产者与消费者若缺少正确的发布顺序、barrier 或 cache flush/invalidate，可能观察到未完整数据或旧数据。`commit`/`ready` 只表示发布状态；它不能证明 payload 未被破坏。V2 的 CRC32 校验已在主机单元测试覆盖 payload 损坏拒绝，但板端行为仍待验证。

## 通知契约

协议层通知接口抽象为：

- `notify(priority)`：提示指定 priority 队列发生可处理状态变化。
- `receive_irq(priority)`：接收指定 priority 的通知并触发队列检查。

mailbox 是当前 doorbell 后端，不是 MailMsg 数据面。当前 mailbox `cmd` 和 `data` 各为一个 `u32`，只承载通知所需元数据；消息内容位于共享内存 frame。通知后端可替换为其他机制而不改变队列 ABI，但具体后端映射仍需单独定义和验证。

## 完整性与错误模型

当前可识别的发布状态由 `commit`/`ready` 提供；sequence 和 length 用于消息关联与有效长度表达。V2 使用 `crc32` 检查帧完整性；主机测试已验证 payload corruption → reject，不能据此宣称板端具备故障恢复。

以下策略不属于已冻结 ABI，均待实现或验证：sequence 异常、length 越界、空/满 ring、通知丢失、重复通知、CRC32 失败后的超时、重试和恢复。

## 兼容性与状态

- 当前协议设计状态：draft；ABI V2 使用 `version=2` 与 `crc32`，固定 frame 为 52 B。V1 的 48 B frame 保留为历史基线。
- V2 验证状态：通用 host C unit test 已覆盖 payload corruption → reject 并通过；Linux kernel 与 Zephyr V2 构建通过；板端仅完成一次 ch0 CRC 正常路径闭环。板端 payload 篡改拒收与故障恢复仍无证据。
- 当前 ring 模型：双向四 priority、SPSC、每 ring 8 个物理 slot/可用上限 7。
- 当前通知后端：mailbox0；其他后端保持可替换设计。
- 已有实验只作为最简 evidence：`EXP-20260822-001` 记录过 V1 MailMsg priority 0 PING/PONG 和一次连续 priority 0–3 测试应用映射；该证据不证明 V2 CRC32 的板端行为、故障恢复或压力边界。
- 待实现/待验证：板端 CRC 篡改拒收与错误处理、大数据 buffer pool、endpoint、多生产者/消费者、背压、超时/恢复、并发/压力、长期稳定和大数据传输。

## 关联问题

暂无。

## 总结

- MailMsg 的数据面是双向四 priority 的共享内存 SPSC ring。
- 每个 ring 有 8 个物理 slot，可用上限为 7；slot 与 frame 是不同术语。
- 发布必须遵守 frame 写入、cache flush/clean、barrier、commit/ready、通知的顺序。
- `commit`/`ready` 是必要发布标记，不是 CRC，也不是 CRC32 的替代；V2 `crc32` 已纳入 ABI，但板端仍待验证。
- mailbox 当前只负责 doorbell，`cmd/data` 各为 `u32`，不承载消息正文。

## 参考资料

- [EXP-20260822-001：构建 R1 Linux-Zephyr 共享内存 PING 原型](../experiment/exp-20260822-001-build-r1-amp-shmem-ping.md)，2026-08-27。
- [DEC-20260827-001：将 AMP 主协议正式命名为 MailMsg](../decision/dec-20260827-001-name-mailmsg-amp-protocol.md)，2026-08-27。
