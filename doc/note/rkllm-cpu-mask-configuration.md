---
title: "RKLLM Runtime 的显式 CPU mask 配置"
type: note
status: verified
created: 2026-08-21
updated: 2026-08-22
tags: [rk3588, rkllm, npu, amp, cpu]
aliases: []
related:
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[experiment/exp-20260821-003-build-r1-psci-cpu-on-heartbeat]]"
  - "[[issue/issue-20260821-001-rkllm-cpu-mask-after-amp-carveout]]"
  - "[[decision/dec-20260821-004-preserve-rkllm-cpu-numbering-during-amp-prototype]]"
---

# RKLLM Runtime 的显式 CPU mask 配置

## 学习目标

能够区分 RKLLM Runtime 的 CPU 配置与 Linux/NPU 内核驱动，并按当前 Linux logical CPU 编号为 Runtime 指定可用 CPU。

## 前置知识

- Linux logical CPU 编号由当前设备树和 online CPU 集合决定，不等同于物理核的固定编号。
- RK3588 R1 当前 AMP RAM 候选从 Linux 删除一个 A55 后，CPU0–2 是 A55，CPU3–6 是 A76。

## 核心概念

`rkllm.h` 的 `RKLLMExtendParam` 提供两个公开字段：

```cpp
int8_t enabled_cpus_num;
uint32_t enabled_cpus_mask;
```

它们配置的是 RKLLM Runtime 进行 CPU 侧工作的核集合；不改变 RKNPU 内核驱动、NPU 核数、Linux scheduler 或设备树。当前 Runtime 的自动检测在标准 8 CPU RK3588 布局中能选到 A76 CPU4–7；静态移除一个 A55 后，A76 被重编号为 CPU3–6，自动检测在实测版本中不再适配。

## 工作流程

1. 先读取 online CPU 的最大频率或 topology，确认哪些 logical CPU 是目标核。
2. 将 count 设为 mask 中实际置位的 CPU 数。
3. 在 `rkllm_init()` 前写入 `param.extend_param`。
4. 检查 Runtime 日志的 `Enabled cpus` 与预期一致，再验证实际生成。

当前 R1 静态 AMP 候选的有效配置：

```cpp
param.extend_param.enabled_cpus_num = 4;
param.extend_param.enabled_cpus_mask = CPU3 | CPU4 | CPU5 | CPU6;
```

## 实际验证

在保持 Linux 7 CPU、`cpu@300` 缺失、`zephyr@50000000` carveout 存在的 RAM 启动候选中，Runtime 输出 `[3, 4, 5, 6]`、`rkllm init success`，并对 `ok` 返回 `Alright,`。RKNPU driver 仍是 `0.9.8`；资源划分回归见 [EXP-20260820-001](../experiment/exp-20260820-001-static-amp-dts-resource-partition.md)，与 Zephyr 心跳并行的最终运行时证据见 [EXP-20260821-003](../experiment/exp-20260821-003-build-r1-psci-cpu-on-heartbeat.md)。

## 关联问题

[ISSUE-20260821-001](../issue/issue-20260821-001-rkllm-cpu-mask-after-amp-carveout.md) 表明：`taskset -c 3-6` 只限制 demo 进程 affinity，实测没有改变 Runtime 自动打印的 CPU 集合。因此不能用 `taskset` 代替 Runtime 参数配置；[EXP-20260821-003](../experiment/exp-20260821-003-build-r1-psci-cpu-on-heartbeat.md) 在同一心跳实例再次保留了这一反证，并记录显式 mask 二进制成功生成 `Alright,` 且心跳从 `HB|0x323` 推进到 `HB|0x32b`。

## 易错点

- `CPU3 | CPU4 | CPU5 | CPU6` 不是所有 RK3588 都可直接复用；它依赖本机当前 logical CPU 编号。
- `enabled_cpus_num` 与 mask 置位数不一致会导致 Runtime 初始化失败。
- 该配置修复的是用户态 Runtime 的拓扑选择，不是升级或替换 NPU driver。

## 总结

- AMP 的 CPU 划分可能改变 Linux logical CPU 编号。
- RKLLM 可通过公开字段显式选择 CPU。
- 本 R1 7 CPU 候选应使用四个 A76：CPU3–6。
- 显式 mask 已与 RKNPU 0.9.8 的真实文本生成共同验证。

## 参考资料

- Rockchip RKLLM Runtime 1.3.0：`rkllm.h`，本仓库 `src/rknn-llm/rkllm-runtime/Linux/librkllm_api/include/rkllm.h`，2026-08-21 查阅。
