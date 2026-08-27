---
title: "ISSUE-20260821-001 RKLLM 在 7 CPU AMP 候选中的 CPU mask 不匹配"
type: issue
status: resolved
created: 2026-08-21
updated: 2026-08-21
tags: [rk3588, rkllm, npu, amp, cpu]
related:
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed]]"
  - "[[note/rkllm-cpu-mask-configuration]]"
  - "[[status/current]]"
---

# ISSUE-20260821-001 RKLLM 在 7 CPU AMP 候选中的 CPU mask 不匹配

## 现象与影响

- 首次发现时间：2026-08-21（具体时间未记录）。
- 可观察现象：AMP resource-DTB 候选启动后，`llm_demo-r1` 在 `rkllm_init()` 失败，尚未进入推理。
- 影响范围：Linux 已成功让出一个 A55 CPU 给未来 Zephyr，但当前 RKLLM 1.3.0 用户态不能按此前方式初始化；NPU 硬件、驱动和模型尚未在本候选上重新验证。
- 发生频率：目前一次复现。

## 环境与复现

- 环境基线：R1、RAM 启动 Linux `5.10.252`、RKNPU driver `0.9.8`、AMP DTS 使 Linux online CPU 为 `0-6`。
- 最近变更：resource 内 `rk-kernel.dtb` 已替换为 AMP DTB；该 DTS 删除 `cpu@300`，使 Linux 由 8 CPU 变为 7 CPU。
- 最小复现步骤：在 `/userdata/rkllm-api-demo` 执行 `./llm_demo-r1 ./models/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm 2048 4096`。
- 预期结果：`rkllm init success`，进入 `user:`。
- 实际结果：Runtime 列出 `[0, 1, 2, 3, 4, 5, 6]` 后报告 CPU mask/count 不匹配并初始化失败。

## 原始证据

```text
I rkllm: rknpu driver version: 0.9.8, platform: RK3588
I rkllm: Enabled cpus: [0, 1, 2, 3, 4, 5, 6]
I rkllm: Enabled cpus num: 4
E rkllm: Mismatch between enabled CPUs mask and expected count. Please check the configuration.
rkllm init failed
```

此前同一 Runtime/模型在 8 CPU 候选中列出 `[4, 5, 6, 7]`，`Enabled cpus num: 4` 并能实际生成文本；两种 CPU 可见集合不同是已知变量。

## 关联知识与实验

- Linux CPU 与 Zephyr carveout 的 RAM 回归：[EXP-20260820-001](../experiment/exp-20260820-001-static-amp-dts-resource-partition.md)。
- 旧 RKNPU 驱动版本导致 W8A8 matmul 失败的问题：[ISSUE-20260815-002](issue-20260815-002-rkllm-w8a8-matmul-run-failed.md)。本问题发生在 `rkllm_init()`，不能与旧问题混同。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：RKNPU 0.9.8 或模型不兼容 | RKLLM 初始化失败 | 对比 Runtime 日志阶段与此前成功链路 | 失败发生在 NPU 提交前，日志仍识别 0.9.8 | 暂不支持，待保留 |
| H2：RKLLM 的 CPU 亲和性/拓扑假设要求 4 个可见 CPU | 运行时把 7 个可见 CPU 列入 mask，却声明期望数为 4 | 用 `taskset -c 3-6` 将 demo 进程限制为 4 CPU | Runtime 仍列出 `[0..6]` 并同样失败 | 排除：Runtime 不按该进程 affinity 选 CPU |
| H3：删除 `cpu@300` 改变 CPU 逻辑编号，Runtime 不能识别 RK3588 的大小核布局 | 原来日志为 `[4,5,6,7]`，现在为 `[0..6]` | 读取每个 online CPU 的最大频率，确认小核/大核的新逻辑编号 | CPU0–2 为 1.8 GHz，CPU3–6 为 2.256 GHz | 已确认编号改变；Runtime 的具体算法仍未知 |
| H4：demo 未设置官方 `enabled_cpus_*` 参数，导致 Runtime 自动检测不适配该拓扑 | `rkllm.h` 定义该字段，demo 只设置 `base_domain_id`、`embed_flash` | 指定 CPU3–6、count 4 后原生编译独立 demo | Runtime 选中 `[3, 4, 5, 6]`，初始化及短文本生成均成功 | 已确认 |

## 根因

已确认的行为根因是：原 demo 未配置 RKLLM Runtime 头文件公开的 `enabled_cpus_num` 与 `enabled_cpus_mask`，因而依赖 Runtime 的自动 CPU 拓扑检测。静态删除 `cpu@300` 后，A76 从逻辑 CPU4–7 重编号为 CPU3–6；在这个 7 CPU 布局中，自动检测打印全体 `[0..6]`，却保留期望 count 4，导致初始化失败。

Runtime 的内部检测实现没有源码证据，故不把其具体算法当作已知事实。`taskset -c 3-6` 不改变 Runtime 选择，也说明这不是单纯的 demo 进程 affinity 问题。

## 解决或绕过方法

在独立的 `llm_demo-amp` 中保留同一模型、Runtime 和 RKNPU driver，只补充官方参数：

```cpp
param.extend_param.enabled_cpus_num = 4;
param.extend_param.enabled_cpus_mask = CPU3 | CPU4 | CPU5 | CPU6;
```

原 `llm_demo-r1` 未被覆盖。该 mask 仅适用于当前“删除 A55 CPU3 后，Linux 的四个 A76 为逻辑 CPU3–6”的 DTS；若以后变更 DTS/CPU 在线集合，必须重新读取 topology，不能照抄。

## 回归验证

完成。在保持 Linux 7 CPU、`cpu@300` 缺失和 `zephyr@50000000` carveout 生效的 RAM 候选中，独立 demo 输出：

```text
I rkllm: Enabled cpus: [3, 4, 5, 6]
I rkllm: Enabled cpus num: 4
rkllm init success
user: ok
robot: Alright,
```

因此该回归同时证明：RKNPU driver 仍为 `0.9.8`，并未为解决本问题改动驱动；显式 CPU 配置可让 RKLLM 1.3.0 在当前静态 AMP 资源布局中完成一次真实 NPU 文本生成。

## 经验与后续行动

- CPU 从 Linux 移交给 AMP 固件会影响用户态对拓扑/亲和性的假设，不能只验证内核和设备树。
- Runtime 的 CPU 配置与 Linux 的 logical CPU 编号绑定；这是应用/Runtime 层配置，不是 NPU 内核驱动问题。
- 后续应只读梳理 BL31、Rockchip AMP driver 与 `amp-cpus` DTS 的启动责任链；在明确固件支持前，不调用 SMC 或启动次级 CPU。
