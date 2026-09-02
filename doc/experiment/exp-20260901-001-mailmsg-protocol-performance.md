---
title: "MailMsg 协议性能观察：四优先级与 p2 stepped window"
type: experiment
status: active
created: 2026-09-01
updated: 2026-09-02
tags: [rk3588, mailmsg, performance]
related:
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
  - "[[note/mailmsg-protocol]]"
---

# MailMsg 协议性能观察：四优先级与 p2 stepped window

## 目标

记录目前 MailMsg R7 RAM-only 回归中得到的请求速率、用户态端到端延迟、窗口化回收和满环观察，并明确这些数据的测量路径与适用边界。该记录不定义性能指标，也不把单次工作负载结果当作硬件容量或产品保证。

## 环境与前置条件

- 执行端：Arch 主机仓库根目录 `/home/loser/Study/rk3588`；测试脚本通过 SSH 在板端运行 profile。
- 硬件及版本：R1，Linux `5.10.252`，`nproc=7`；Zephyr CPU3 已启动。
- 软件、镜像或 Git commit：R7 fresh normal RAM-only 会话，session `1/1`、state `active`；未写 eMMC。
- 测试日期：2026-09-01；具体事件时刻未记录。板端报告文件名中的 RTC 时间不作为事件时间。
- 关联主记录：[MailMsg 共享内存 PING 原型](exp-20260822-001-build-r1-amp-shmem-ping.md)的步骤 61、62 保存完整回归上下文和原始附件。

## 风险与恢复

- 影响范围：仅使用 RAM-only 候选和用户态测试进程；不写 eMMC，不改变持久化启动内容。
- 备份：此前已通过的两组结果附件按原字节保存在 `doc/_assets/mailmsg-r7/`；两轮 sweep 的原始报告、RKLLM 日志和 8 路日志分别保存在 `build/local/mailmsg-four-priority-sweep-20260901/` 与 `build/local/mailmsg-four-priority-sweep-20260902/`。
- 恢复方法：测试结束后按既有脚本使用 fresh RAM 会话；本记录不包含板端持久化变更。

## 步骤与证据

### 步骤 1：四优先级同时 `window=1` + RKLLM 回归（已验证）

目的与预期结果：在四个 priority 各使用一个独占持久 fd，同时运行 8 秒的 `window=1` benchmark，并在同一会话完成 24 次 RKLLM Generate。p0/p1 每条 4-byte PING 需要 PONG+ACK，p2/p3 需要 PONG；预期各路成功回收且不出现脚本记录的输入/响应错误。

```bash
# Arch 主机，仓库根目录 /home/loser/Study/rk3588
scripts/mailmsg-v1-test.sh --profile llm-four-priority
```

脚本通过 SSH 在板端执行 profile，命令退出结果为 profile 通过。fresh R7 normal 会话为 Linux `5.10.252`、7 核、CPU3 on、session `1/1` active；四路各自使用一个持久 fd，同时运行 8 秒，benchmark 参数为 priority/window/duration/base，其中 `window=1`，每条请求为 4-byte PING。

实际结果：

| priority | write | PONG | ACK | 请求速率（req/s） | latency min/p50/p95/p99/max（μs） |
| --- | ---: | ---: | ---: | ---: | --- |
| p0 | 97339 | 97339 | 97339 | 12167.375 | 4.375 / 46.373 / 115.785 / 380.604 / 10399.691 |
| p1 | 98177 | 98177 | 98177 | 12272.125 | 4.666 / 46.373 / 118.702 / 381.772 / 10182.412 |
| p2 | 110227 | 110227 | — | 13778.375 | 3.208 / 33.539 / 101.494 / 333.648 / 10300.530 |
| p3 | 106192 | 106192 | — | 13274.000 | 3.208 / 35.581 / 107.911 / 347.940 / 10429.440 |

各路 `ENOSPC/EAGAIN/write_other/nack/duplicate/unknown/timeout/lost` 均为 `0`，`max_inflight=1`；总 write 为 `411935`，合计约 `51491.875 req/s`。24 次 RKLLM Generate 全完成，post p0–p3 回归通过；最终 `worker msg=411939`（基线 0、窗口写入 411935、post 4），pending、A2B 和各队列 depth 均为 `0`，full/incomplete/crc/invalid/stale 均为 `0`，通知均为 `SENT`。`empty=43` 是轮询中的空读，不是错误。

这里的 latency 是用户态端到端路径观察，包含用户程序启动、sysfs/字符设备路径和协议往返，不等于纯 ISR 或纯协议延迟。结果只属于本次 fresh RAM-only、四路同时、`window=1`、8 秒工作负载；不构成并发上限、公平性/优先级抢占、吞吐承诺、实时上界或长期稳定性结论。

证据来自本机目录 `build/local/mailmsg-four-priority-20260901/`；原始板端报告路径为 `/userdata/mailmsg-v1-r7/reports/llm-four-priority-20231122-050008.log`。附件均为原始字节的相对链接：

- [四优先级报告](../_assets/mailmsg-r7/llm-four-priority.log)，5798 B，SHA-256 `b0c80c4f28586e669345409ab636418923824da48b5a87981fea6f4f77235f0a`。
- [RKLLM 日志](../_assets/mailmsg-r7/llm-four-priority-rkllm.log)，28056 B，SHA-256 `1518c82376b036e7f235b1a166047c7cf3fcff959cf4a5fbbf3a9bee30125296`。
- [p0 日志](../_assets/mailmsg-r7/llm-four-priority-p0.log)，284 B，SHA-256 `a7c93a193388bcb7a6c85abaf5029d04ddad6305db7265f398b7af73e8b6f15a`。
- [p1 日志](../_assets/mailmsg-r7/llm-four-priority-p1.log)，284 B，SHA-256 `6d074b2000d8a600140103f6ab330058771043297ce5fb43c2a3a86b3b48c574`。
- [p2 日志](../_assets/mailmsg-r7/llm-four-priority-p2.log)，284 B，SHA-256 `024de0ad7719f70d01eaad8726352edcf7c7b999103b1dc7d19a8cc4c5873a6b`。
- [p3 日志](../_assets/mailmsg-r7/llm-four-priority-p3.log)，284 B，SHA-256 `0ab55e61d40060b2d24919ee081918453e45c856ac5222fe37bdbec74d632a8f`。

### 步骤 2：p2 stepped `window` 与 RKLLM 回归（已验证）

目的与预期结果：在 p2 单 priority 工作负载下逐级运行 `window=1/2/4/7/8/16/32`，观察 PONG 回收、输入 `ENOSPC`、超时/丢失和 reverse full 统计；同一 profile 完成 24 次 RKLLM Generate。`window` 表示每进程未完成请求数，不是硬件队列容量。

```bash
# Arch 主机，仓库根目录 /home/loser/Study/rk3588
scripts/mailmsg-v1-test.sh --profile llm-p2-window
```

脚本通过 SSH 在板端运行，使用 fresh R7 RAM-only normal 会话。窗口 `1/2/4/7` 的 PONG lost 均为 `0`，窗口 `8/16/32` 的 lost 分别为 `1/9/25`；reverse full delta 同为 `1/9/25`。p2 输入 `ENOSPC` 的原报告分档只给出序列 `0/0/0/33/224`，本记录不为其猜测窗口映射。24 次 RKLLM Generate 全完成，`accepted=2616267`、PONG `2616232`、lost/timeout `35`、ENOSPC `257`。

本步骤的 `lost/timeout` 定义为 5 秒 drain 窗口内未回收 PONG。direct-doorbell 下未见 generic mailbox TX queue `ENOBUFS`；reverse full 与高窗口 lost 同步观察，但不能单独据此归因 response ring full，也不能推出通用硬上限。完整状态、worker 计数、最终四 priority 回归和工具校验见主记录步骤 61。

证据来自本机目录 `build/local/mailmsg-p2-window-20260901/`；原始板端报告路径为 `/userdata/mailmsg-v1-r7/reports/llm-p2-window-20231122-045851.log`。附件均为原始字节的相对链接：

- [p2 窗口报告](../_assets/mailmsg-r7/llm-p2-window.log)，7005 B，SHA-256 `b7c4492cd1dfadab2cea3e88de865b8e70826ce5144c67cc90d0682a08828dbe`。
- [p2 RKLLM 日志](../_assets/mailmsg-r7/llm-p2-window-rkllm.log)，28056 B，SHA-256 `9be1e10cc11ee2adae1130c372b673045a11757adaefe42d8c4082959da2f53a`。
- [p2 benchmark 日志](../_assets/mailmsg-r7/llm-p2-window-bench.log)，1994 B，SHA-256 `b18280d329ea66a911bf94ff3a2cac293e2917c140dce519b9fd22eae84d0200`。

### 步骤 3：四优先级同时负载 `window=2/4/7` sweep（未完整通过）

目的与预期结果：在 R7 final RAM image 的 fresh RAM 会话中，按严格规则依次运行四 priority 短时 RKLLM 并行负载，比较 `window=2/4/7` 的 write、PONG、ACK、timeout/lost 和 in-flight 结果。脚本在 p0 失败后停止，因此本轮只执行到 `window=4`。

本次事件时间为 2026-09-01，板端 RTC 输出目录名 `20231122-050240` 不作为事件时间。R7 final RAM image 已启动；运行端为 Arch 主机仓库根目录 `/home/loser/Study/rk3588`，脚本通过 SSH 在板端运行对应 profile。window=2 四 priority 短时 RKLLM 并行通过：p0/p1 的 ACK=PONG=write_ok，p2/p3 的 PONG=write_ok，timeout/lost/协议错误均为 `0`，`max_inflight=2`。

window=4 的实际结果如下：

| priority | write_ok | PONG | ACK | timeout | lost | max_inflight |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| p0 | 131961 | 131960 | 131961 | 1 | 1 | 4 |
| p1 | 138183 | 138182 | 138183 | 1 | 1 | 4 |
| p2 | 188479 | 188479 | 0 | 0 | 0 | 4 |
| p3 | 182659 | 182659 | 0 | 0 | 0 | 4 |

脚本按严格规则在 p0 失败后停止；因此 window=7 和 post-regression 未执行。最终状态快照显示 `mailmsg_tx_full ... count=2 priority=0 type=2 result=-2`，Linux 输入环 `full=0`、通知失败 `0`。response ring 满与 p0/p1 的 PONG lost 存在关联候选，但本次观察不能证明因果，也不能单独归因于 response ring full。

证据来自本机目录 `build/local/mailmsg-four-priority-sweep-20260901/`；原始板端报告路径为 `/userdata/mailmsg-v1-r7/reports/llm-four-priority-sweep-20231122-050240.log`。文件均保持原字节，未将板端 RTC 文件名当作事件时间：

- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240.log`（主报告，5137 B），SHA-256 `b3d1ab30b68e9ea74f030d2afd02ddf06d1d1fff31b6bd2c51d1696acafa489c`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-rkllm.log`（RKLLM，4760 B），SHA-256 `873970c06853a7a034cfd166df96764e6f715280160574ee603611829cf17e7d`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w2-p0.log`（window=2 p0，289 B），SHA-256 `aa392e0e1bc8ed4f2d2d8c60315b394fcf1bdc37627cd0ba16276c4df6954514`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w2-p1.log`（window=2 p1，288 B），SHA-256 `7efe3b33444820cdad18f2301f03b243d851f425fdc6ff00806db71fa560bb51`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w2-p2.log`（window=2 p2，284 B），SHA-256 `03325d3434a17c3084c47737d9aa0e8fb0e435d7ee24f0d015d9569bd9e37794`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w2-p3.log`（window=2 p3，284 B），SHA-256 `6c08da3d820002c36772d23035204f584d71316743401e6d650ccbf52d1663cc`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w4-p0.log`（window=4 p0，289 B），SHA-256 `0a281068f86a17feffd2772fce18dd65abefb43528c682861898b471379b449d`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w4-p1.log`（window=4 p1，288 B），SHA-256 `6cac6a05f2f2fe5865d5735ce0777ad3a89c9b84545f8f28ca2124c6c154ea2c`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w4-p2.log`（window=4 p2，283 B），SHA-256 `21d00f4348a6e0e260c07f37b7cac121f6ae9425c90e2203bdd9af560addddba`。
- `build/local/mailmsg-four-priority-sweep-20260901/llm-four-priority-sweep-20231122-050240-four-p-w4-p3.log`（window=4 p3，283 B），SHA-256 `9cff46043898b4441334f68ec4b492513dfb35282ef72be4b70050ec7f0cea4`。

### 步骤 4：四优先级同时负载第二次 fresh RAM sweep（未完整通过）

目的与预期结果：在另一 fresh RAM 会话中重复四优先级 `window=2/4/7` 短时 RKLLM 并行负载，确认前一轮 window=4 边界是否可复现。脚本在 p0 严格失败后停止，故本轮不执行 window=7 和 post-regression。

本次事件时间为 2026-09-02，板端 RTC 文件名 `20231122-050608` 不作为事件时间。使用 R7 final RAM image，Linux `5.10.252`、7 核，初始 fresh `unarmed/session=0/0`；执行端为 Arch 主机仓库根目录 `/home/loser/Study/rk3588`，命令为 `./scripts/mailmsg-v1-test.sh --profile llm-four-priority-sweep`，脚本通过 SSH 上传、校验并运行，未写 eMMC。RKLLM 日志显示 init success、Enabled CPUs `[3,4,5,6]`/4，至少 3 次生成可见；终态 CPU3 on、session active、pending/a2b/depth 均为 `0`，Linux input ring `full=0`、notify failed `0`。

window=2 的四路结果为 p0 `124816/124816/124816`、p1 `125509/125509/125509`、p2 `142745/142745/0`、p3 `140334/140334/0`（各项依次为 write/PONG/ACK）；错误和丢失均为 `0`，`max_inflight=2`。

window=4 的实际结果如下：

| priority | write_ok | PONG | ACK | timeout | lost | max_inflight |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| p0 | 132602 | 132601 | 132602 | 1 | 1 | 4 |
| p1 | 137547 | 137546 | 137547 | 1 | 1 | 4 |
| p2 | 191004 | 191004 | 0 | 0 | 0 | 4 |
| p3 | 186047 | 186047 | 0 | 0 | 0 | 4 |

脚本在 p0 严格失败后停止；因此 window=7 与 post-regression 未执行。最终 snapshot 为 `mailmsg_tx_full valid=1/commit=3/count=2/priority=1/type=2/result=-2`，Linux input ring `full=0`、notify failed `0`，pending/a2b/depth 为 `0`，CPU3 仍 on、session active。response ring saturation 与 p0/p1 各 1 次 PONG lost 的关联仍只是待定位候选，不能写成已证实根因。

证据来自本机目录 `build/local/mailmsg-four-priority-sweep-20260902/`；原始板端报告路径为 `/userdata/mailmsg-v1-r7/reports/llm-four-priority-sweep-20231122-050608.log`。文件保持原字节，板端 RTC 文件名不作为事件时间：

- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608.log`（主报告，5107 B），SHA-256 `d745002a5c398a5a65f1ef34eef88dc0504a87d6f9d8b932b3e85d17e57dc746`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-rkllm.log`（RKLLM，4748 B），SHA-256 `2604140a7af04bd2c6bce12788a43cf3bd7d273bdfa45986cf90cab810c40a77`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w2-p0.log`（window=2 p0，288 B），SHA-256 `7ffd5a806a2320f904a1c87c7a46db5b008b7f43f533b5743de48ba259e966dd`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w2-p1.log`（window=2 p1，288 B），SHA-256 `2602e7579a63c1062c8f3ca1084f01517824c518fb13dacdcef99416e946e3cb`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w2-p2.log`（window=2 p2，284 B），SHA-256 `4067a6e346c01e9e3d55f6cc365b362f1d67ab51c50a39c20e469e1a3e5c1763`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w2-p3.log`（window=2 p3，284 B），SHA-256 `99fc1382aa84245fd8927c4042e1e1da9350e96cd95e61e114678714b2f0b817`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w4-p0.log`（window=4 p0，288 B），SHA-256 `31349684a18b0810c9f9f21b83c87669afe5b52dfd2ca9d9c030f0a29b9284ea`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w4-p1.log`（window=4 p1，288 B），SHA-256 `2a1ebfe7c7a92a36330dfb02537d2f2765205d31dbc8d21adc01d41044929db6`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w4-p2.log`（window=4 p2，283 B），SHA-256 `84e7f80ef6f968b64fa07ec3d364ec2cde8baf6d759d2b5fa44222d1cef655fa`。
- `build/local/mailmsg-four-priority-sweep-20260902/llm-four-priority-sweep-20231122-050608-four-p-w4-p3.log`（window=4 p3，283 B），SHA-256 `845b4b7c0c9a1c174934df6d265b6441cefd996ad85be707188258be9ea03842`。

### 步骤 5：现有 active 会话的 `window=4` focused 诊断（已记录，边界仍待定位）

目的与预期结果：在不重启的现有 active R7 会话中，用现有 benchmark 的 5 秒 drain 分别运行 p0 单路、p1 单路和 p0+p1 双路，观察 PONG 丢失是否只在跨 priority 竞争时出现。该诊断不证明晚到响应，也不把 response ring full 作为已证实根因。

实际结果：p0 单路 `write=602854/PONG=602853/ACK=602854/lost=1/timeout=1`，`mailmsg_tx_full` 计数 `2→3`，最后记录 p0/type2/result `-2`；p1 单路 `write=311815/PONG=311814/ACK=311815/lost=1/timeout=1`，full `3→4`，最后记录 p1/type2/result `-2`。p0+p1 双路分别为 p0 `write=379037/PONG=379036/ACK=379037/lost=1`、p1 `write=378831/PONG=378830/ACK=378831/lost=1`，full `4→6`。

单路与双路均复现 PONG lost，因此在本次诊断中跨 priority 竞争不是必要条件；response ring full 仍是强关联候选，不能写成已证实因果。所有 lost/timeout 仅按现有 benchmark 的 5 秒 drain 定义，未证明响应在 drain 后到达。

### 步骤 6：四优先级同时负载第三次 fresh RAM sweep（未完整通过）

目的与预期结果：进行第 3 次独立 fresh RAM + RKLLM sweep，复核 window=2 的可用观察和 window=4 的 p0/p1 边界。板端 RTC 文件名 `20231122-045819` 仅作为报告文件名，不作为事件时间。

本次事件时间为 2026-09-02。使用 R7 final RAM image，脚本通过 SSH 运行；window=2 的四路 write/PONG/ACK 分别为 p0 `121456/121456/121456`、p1 `121099/121099/121099`、p2 `140553/140553/0`、p3 `139522/139522/0`，均 `timeout/lost=0`、`max_inflight=2`。window=4 的四路结果如下：

| priority | write_ok | PONG | ACK | timeout | lost | max_inflight |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| p0 | 133948 | 133947 | 133948 | 1 | 1 | 4 |
| p1 | 136023 | 136022 | 136023 | 1 | 1 | 4 |
| p2 | 186422 | 186422 | 0 | 0 | 0 | 4 |
| p3 | 179184 | 179184 | 0 | 0 | 0 | 4 |

脚本严格在 p0 失败后停止，window=7 和 post-regression 未执行。报告终态为 CPU3 on、session active、pending/a2b/depth 均为 `0`，Linux input ring `full=0`、notify failed `0`；本轮 session 起始清零的 `mailmsg_tx_full` 为 `count=2`，最后记录 p1/type2/result `-2`。window=4 的 p0/p1 各 1 次 PONG lost 已在 3 次独立 fresh sweep 中复现，不能称偶发；response ring full 仍是关联候选，不是已证实因果。

证据来自本机目录 `build/local/mailmsg-four-priority-sweep-20260902-repeat3/`；原始报告的板端文件名为 `llm-four-priority-sweep-20231122-045819.log`。文件保持原字节：

- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819.log`（主报告，5142 B），SHA-256 `fd8d30f511ef5e70c64c4564ec77ed92bf63699b027686d541527adc3c685c06`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-rkllm.log`（RKLLM，4778 B），SHA-256 `497d2331a1065bb0f7385e795c0988c7043697603d99dfc7dc17126fb9ee2b13`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w2-p0.log`（window=2 p0，288 B），SHA-256 `3d97b3ad7119cdc470a198db139934c462f161ec5728e889dcdc1d35ae4d42ba`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w2-p1.log`（window=2 p1，288 B），SHA-256 `ffe91ae7832bad04cc952abe6b2bb54e934de1ca42d0d7f0d23d1c24533e61b9`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w2-p2.log`（window=2 p2，284 B），SHA-256 `b6aad9c96ff6ceb4881d357d3f0c236b61ff82470cea05159229967d965f17a6`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w2-p3.log`（window=2 p3，283 B），SHA-256 `cc0e91020677f215dff76f5021aa72212d5a53608095cf2724957298cd5ec023`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w4-p0.log`（window=4 p0，288 B），SHA-256 `70ef5b152f9198b4b2b6f69dc2cad91bc258c235f76cc5c1c195c296afd20a60`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w4-p1.log`（window=4 p1，288 B），SHA-256 `af8c01ea7a6055a18eb6e13d0961b888e60c161920930bbb30fe1e18fc0da808`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w4-p2.log`（window=4 p2，283 B），SHA-256 `0c6e692b2f0fb7b441284f6d6607bafb8b616e69803fc1756a221206596683ec`。
- `build/local/mailmsg-four-priority-sweep-20260902-repeat3/llm-four-priority-sweep-20231122-045819-four-p-w4-p3.log`（window=4 p3，283 B），SHA-256 `5911afeda60cbf40cb22ab83ec68d51e570e681cddc59e55ba1b4e5132ce386c`。

### 步骤 7：四优先级同时负载 `window=3` 边界诊断（已验证，范围受限）

目的与预期结果：在同一 active RAM-only 会话中，以四 priority、`window=3`、8 秒的短时负载观察可靠通道是否仍能在每个请求的 ACK/PONG 成对响应下保持无丢失；本步骤不使用 RKLLM，也不是 fresh 会话或生产安全上限测试。

事件时间为 2026-09-02；板端为 R7 active session，具体启动时间未记录。Arch 主机通过四个并行 SSH 分别在板端执行既有 benchmark；四条实际命令分别为：

```text
/userdata/mailmsg-v1-r7/mailmsg-window-bench-aarch64 0 3 8 7000000
/userdata/mailmsg-v1-r7/mailmsg-window-bench-aarch64 1 3 8 8000000
/userdata/mailmsg-v1-r7/mailmsg-window-bench-aarch64 2 3 8 9000000
/userdata/mailmsg-v1-r7/mailmsg-window-bench-aarch64 3 3 8 10000000
```

四路分别对应 priority/base，目的为在 `window=3` 下观察四 priority 的并行收发。测试前后读取 status，测试后读取 `mailmsg_stats`，再将板端 `reports/window3-boundary-20260902/p0..p3.log` 原字节 scp 至本机 `build/local/mailmsg-window3-boundary-20260902/`。主会话确认连续两轮诊断均为零丢失、零 timeout、零 ENOSPC，`max_inflight=3`。保存证据的一轮结果为：p0 write/PONG/ACK=`202205/202205/202205`，p1=`209929/209929/209929`，p2 write/PONG=`277185/277185`，p3=`252403/252403`；post 各方向 depth 为 `0`。前后状态中的 `mailmsg_tx_full` 均为 `valid=1/commit=3/count=2/priority=1/type=2/result=-2`，本轮未新增。

该轮仅使用当前 active 会话且不含 RKLLM；因此不能写成 fresh/RKLLM 回归、生产安全上限、吞吐/延迟保证或并发消费者保证。源码审查（`src/mailmsg/mailmsg.h:24,129-130`、`src/mailmsg/mailmsg.c:9-32`、`src/zephyr-amp-shmem-ping/src/main.c:321-337,403-420,486-555`）确认 `MAILMSG_RING_SLOTS=8`、实际可用 7；p0/p1 对每个合法 PING 严格先写 ACK、再写 PONG，二者进入同一 priority 的反向 ring，FULL 返回 `-2` 且当前调用方只记录 `tx_full`、不重试。于是无并发消费的可靠通道最坏突发 `window=4` 需要 8 个反向帧，第 8 个 PONG 超出 7 槽；p2/p3 每请求一个 PONG，已有 window=7 无损观察。结合本步骤两轮 window=3 均无损，确认既有 window=4 p0/p1 各少一个 PONG 的结构性响应放大边界；并发消费可能暂时提高可观察窗口，但不提供最坏情况保证。

证据文件保持原字节，大小和 SHA-256 如下：

- `build/local/mailmsg-window3-boundary-20260902/p0.log`（287 B），SHA-256 `32962048c16f59ef935c7a1dae0cf9be8841e748e53f349f854f8da0a544706c`。
- `build/local/mailmsg-window3-boundary-20260902/p1.log`（287 B），SHA-256 `3323a02cac0cece20c99047c71c553807ddc72eacf3a39863791c218fa51bcff`。
- `build/local/mailmsg-window3-boundary-20260902/p2.log`（281 B），SHA-256 `c25d0ab2e5eb654fff18c13a498256678d296a5fe3f775a83840e1143d75a88f`。
- `build/local/mailmsg-window3-boundary-20260902/p3.log`（281 B），SHA-256 `eb3f80754eac564030bd32a80b62c86756d7c1304de9d676b31544ac51e6f44f`。
- `build/local/mailmsg-window3-boundary-20260902/post.stats`（437 B），SHA-256 `b4c80a9b66f791cc0c65a5f0ee77a00713d1d9f54fe33f6a5e2ee5ff5450bfda`。
- `build/local/mailmsg-window3-boundary-20260902/post.status`（794 B），SHA-256 `3810c1815d44df1ef3f187492880ff5e489d87b9c4a6612dbfb9f5a7ca02aeb7`。
- `build/local/mailmsg-window3-boundary-20260902/pre.status`（796 B），SHA-256 `52000602eee31da9c2732a05af8b008d4aeca09d8b7cb09e5bdcf6fafef89919`。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 四 priority `window=1` 8 秒功能闭环 | p0/p1 PONG+ACK、p2/p3 PONG，且无脚本错误 | 四路 write/PONG/ACK 或 PONG 全部对应，错误计数均为 0 | 通过（本工作负载） |
| 四 priority 用户态 latency 观察 | 记录各路 min/p50/p95/p99/max | 四路数据如步骤 1；延迟包含用户态启动/设备路径/协议往返 | 已记录，不作纯协议性能结论 |
| p2 stepped window | 记录不同窗口的回收和错误 | `1/2/4/7` lost=0；`8/16/32` lost=`1/9/25`；输入 ENOSPC 原报告序列 `0/0/0/33/224` | 通过（本工作负载观察） |
| 四 priority sweep | window=2 完成；window=4 后按规则停止 | window=2 通过；window=4 p0/p1 各 timeout/lost=1，脚本停止；window=7/post 未执行 | 未完整通过 |
| 四 priority sweep 第二次复现 | fresh RAM 重复 window=2/4，确认前一轮边界 | window=2 通过；window=4 p0/p1 再次各 timeout/lost=1，脚本停止；window=7/post 未执行；response ring 满为待定位关联候选 | 未完整通过 |
| `window=4` focused 诊断 | 比较 p0 单路、p1 单路及 p0+p1 双路是否均可复现 | p0 单路、p1 单路和双路均各出现 1 次 PONG lost；full 计数分别推进 `2→3`、`3→4`、`4→6` | 已记录，因果待定位 |
| 四 priority sweep 第三次复现 | fresh RAM 重复 window=2/4 | window=2 四路均无错误或丢失；window=4 p0/p1 再次各 timeout/lost=1，window=7/post 未执行；session 起始清零后 `mailmsg_tx_full count=2` | 未完整通过 |
| 四 priority `window=3` 两轮边界诊断 | 当前 active 会话下观察四 priority 无损 window=3 | 两轮均 zero loss/timeout/ENOSPC、`max_inflight=3`；保存一轮 p0/p1 ACK+PONG 完整，p2/p3 PONG 完整；`tx_full` 未新增 | 通过（受限工作负载） |
| 证据可追溯性 | 保留报告、日志和完整 SHA-256 | 四优先级 6 份、p2 3 份及 window=3 诊断 7 份本机证据均记录大小/完整 SHA-256 | 通过 |

## 结论

两组既有结果以及本轮 sweep 均由主会话确认并有本机证据支持。四优先级 `window=1` 仍是已完成的功能/用户态路径观察；p2 stepped profile 的窗口结果保持原记录边界。本轮 sweep 中 window=2 通过，window=4 在 p0/p1 各观察到 1 次 timeout/lost 后按严格规则停止，window=7 与 post-regression 未执行。步骤 7 的两轮 `window=3` 在当前 active 会话均无丢失，且源码明确给出可靠通道的 ACK/PONG 响应放大边界：7 个可用反向 slot 对每请求两个响应时，`window=4` 的第 8 个响应为结构性 FULL；p2/p3 每请求一个响应，已有 window=7 无损观察。旧步骤中的 `tx_full` 只作为当时的运行时观察；结合源码和本次边界实验后，p0/p1 各少一个 PONG 的结构性原因已确认，不再只是候选。

以上数据只描述各自测试工作负载，不能作为 MailMsg 的通用并发上限、公平性/抢占、吞吐或实时性保证，也不能替代长期稳定性、并发 writers、崩溃恢复、大载荷或 eMMC 验证。window=4 已在 3 次独立 fresh sweep 中复现 p0/p1 各 1 次 PONG lost，focused 诊断又在 p0 单路、p1 单路和双路复现各 1 次 lost，说明本次观察中跨 priority 竞争不是必要条件；步骤 7 的两轮当前 active 会话 window=3 均无损。window=2 已 3/3 fresh sweep 通过，但不是数学硬上限。源码与步骤 7 已确认 response ring saturation 是可靠通道 ACK/PONG 响应放大的结构性边界；这不等于已经完成缓解，也不把它扩展为并发消费者、吞吐、实时性或生产安全上限结论。5 秒 drain 外是否有晚到响应未验证。

## 关联知识与问题

- 支持或修正的知识点：[MailMsg 协议设计说明](../note/mailmsg-protocol.md)中的窗口、队列和用户态路径边界。
- 关联实验：[MailMsg 共享内存 PING 原型](exp-20260822-001-build-r1-amp-shmem-ping.md)步骤 61、62。

## 后续行动

- [ ] 在 fresh RAM 边界下先将安全运行档位暂定为 `window<=2`；定位并缓解已由源码与边界实验确认的 window=4 response ring saturation 结构性边界后，再考虑 `window=7`，并继续区分并发消费对观察窗口的暂时影响与最坏情况保证。
