---
title: "DEC-20260821-004 AMP 原型保留 RKLLM CPU 逻辑编号"
type: decision
status: active
created: 2026-08-21
updated: 2026-08-21
tags: [rk3588, amp, rkllm, cpu, device-tree]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[issue/issue-20260821-001-rkllm-cpu-mask-after-amp-carveout]]"
  - "[[note/rkllm-cpu-mask-configuration]]"
  - "[[status/current]]"
---

# DEC-20260821-004 AMP 原型保留 RKLLM CPU 逻辑编号

## 背景与约束

静态 AMP DTS 删除 A55 `cpu@300` 后，Linux 仅有 7 CPU，且 A76 的逻辑编号由原来的 `4–7` 变为 `3–6`。当前 RKLLM 1.3.0 Runtime 在这一布局中直接读取全系统 `[0..6]` 并在初始化前报 CPU mask/count mismatch；进程级 `taskset` 不改变这一行为。RKNPU LLM 是项目不可删的核心，同时 AMP 原型仍需为 Zephyr 预留内存和最终可移交 CPU。

## 候选方案

| 方案 | 优点 | 缺点 | 风险 | 验证情况 |
| --- | --- | --- | --- | --- |
| 启动 DTS 删除 `cpu@300` + demo 显式 CPU3–6 mask | Linux 从启动即不管理该核，资源边界更强；LLM 使用四个 A76 | logical ID 变化后需维护 Runtime 配置 | DTS 改动后必须重核对 mask | 已验证：初始化并生成文本 |
| 保留 8 CPU 节点，运行时 offline CPU3 | 保持 A76 为 `4–7`，CPU 交接可逆，能先验证 LLM 与离线 CPU 共存 | Linux 仍在启动时认识 CPU3，不是最终的启动时隔离 | 若未来固件启动需要完全不枚举 CPU，需重新评估 | 保留为备选，未验证 |
| 在 demo 中显式配置 Runtime 的 CPU mask | 保持启动时 CPU/内存静态排除，使用 API 已声明的配置字段 | 与当前 Runtime/模型组合仍需实测 | 配置的 CPU logical ID 随 DTS 改变 | 优先验证 |
| 修改或替换 RKLLM Runtime | 理论上可适配任意编号 | 无 Runtime 源码/兼容性保证，风险大 | 可能破坏 NPU 运行链 | 不采用 |

## 决定

此前选择“保留 8 CPU 逻辑编号 + runtime offline CPU3”仅基于 Runtime 自动检测失败的现象。随后确认 `rkllm.h` 已提供 `enabled_cpus_num` 与 `enabled_cpus_mask`，而当前 demo 没有设置它们，因此该决定暂缓。

已验证官方 API 的显式 mask：在 7 CPU AMP 候选中将 Runtime 固定到 A76 CPU3–6 后，`rkllm_init()` 成功并生成 `Alright,`。因此当前选择保留启动时 CPU 删除的静态隔离方向，并在 demo 中明确设置：

```cpp
enabled_cpus_num = 4
enabled_cpus_mask = CPU3 | CPU4 | CPU5 | CPU6
```

“保留 8 CPU 节点 + runtime offline CPU3”仍是未构建备选，不进入当前 RAM 回归路径。

## 影响与复查条件

- 影响：`rk3588s-yyt-amp-hotplug.dts` 仅作为未构建的备选 DTS；不进入当前 RAM 回归路径。
- 何时需要重新评估：若 DTS/online CPU 集合改变；若新 Runtime/模型对 CPU mask 有不同约束；或 BL31/AMP 启动路径要求不同的 CPU 交接方式时。
