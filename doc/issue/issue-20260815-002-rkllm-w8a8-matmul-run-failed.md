---
title: "ISSUE-20260815-002 RKLLM W8A8 矩阵乘法执行失败"
type: issue
status: resolved
created: 2026-08-15
updated: 2026-08-16
tags: [rk3588, npu, rkllm, w8a8, matmul]
related:
  - "[[status/current]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
  - "[[resource/deepseek-r1-distill-qwen-1-5b-w8a8-rk3588]]"
  - "[[resource/airockchip-rknn-llm]]"
---

# ISSUE-20260815-002 RKLLM W8A8 矩阵乘法执行失败

## 现象与影响

- 首次发现时间：未知。
- 可观察现象：`llm_demo-r1` 已进入 `user:` 交互后，输入 `ok` 时反复输出 `E rkllm: matmul(w8a8) run failed`，未得到生成文本。
- 影响范围：原厂 Linux 5.10.110 / RKNPU 0.8.2 组合无法完成该 W8A8 模型的首次 token 生成。
- 发生频率：旧组合一次复现失败；RKNPU 0.9.8 RAM 启动候选上已完成同模型生成回归。

## 环境与复现

- 环境基线链接：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- 最近变更：向 `/userdata/rkllm-api-demo/models/` 传入 2,040,247,614 字节候选模型；板端 SHA-256 与主机一致。
- 最小复现步骤：以 `LD_LIBRARY_PATH=./lib` 和 `RKLLM_LOG_LEVEL=1` 启动 `llm_demo-r1`，模型参数为 `max_new_tokens=64`、`max_context_len=512`，在 `user:` 输入 `ok`。
- 预期结果：callback 输出生成文本，随后回到交互提示。
- 实际结果：输出 W8A8 matmul 失败，未生成文本。

## 原始证据

```text
user: ok
E rkllm: matmul(w8a8) run failedrobot:
E rkllm: matmul(w8a8) run failed
E rkllm: matmul(w8a8) run failed
E rkllm: matmul(w8a8) run failed
```

最初只保存了上述生成阶段节选；随后已从同一次终端滚动记录取得初始化版本行，并另行采集失败后的内核日志和内存状态。进程退出状态仍未保存。

## 关联知识与实验

- 相关知识点：`rkllm_init()` 成功只说明模型初始化阶段通过；生成阶段的 W8A8 matmul 才触及推理计算路径。
- 验证实验：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：RKLLM Runtime 与 R1 的 RKNPU 0.8.2 驱动不配对 | Runtime 初始化头明确要求 driver >=0.9.7 | 已由同次实际启动日志直接验证 | 当前 Runtime 1.3.0 + Driver 0.8.2 触发低版本警告并在 W8A8 执行失败 | 确认（直接阻塞） |
| H2：模型转换版本/来源与 Runtime 不匹配 | 模型精确下载来源与转换版本未确认 | 在升级驱动候选上使用同一文件完成生成 | 同一 `.rkllm` 文件已生成文本 | 针对本模型排除 |
| H3：运行资源不足 | R1 仅 3.8 GiB RAM、无 swap；但参数已保守 | 在相同 4 GiB 设备上记录成功运行峰值 | 成功运行报告峰值 1673.56 MB | 针对本参数排除 |
| H4：prompt 内容或 CLI 参数导致错误 | 错误发生在 W8A8 matmul，而非参数检查 | 在升级驱动候选上重试同一短 prompt | `ok` 生成 `Alright,` | 排除 |

### 验证 1：失败后的内存与内核日志

学习者在退出 demo 后执行只读 `free -h` 与过滤后的 `dmesg`。实际内存输出为：

```text
Mem:           3.8Gi       631Mi       1.6Gi       6.0Mi       1.6Gi       3.2Gi
Swap:             0B          0B          0B
```

过滤日志中未出现 `oom`、`out of memory`、`killed process`、IOMMU fault 或本次生成后的新 RKNPU 报错；所见 RKNPU 行均为启动早期（约 3 秒）初始化信息，包括已知的 `power_model` 配置缺失。

结论：**已验证**此次采集没有留下常见 OOM killer 或内核侧 NPU fault 证据，且进程退出后可用内存为 3.2 GiB。**不能据此排除 H3**：用户态或 NPU 专用内存分配仍可能在运行瞬间失败而未产生这些内核日志。当时仍缺 Runtime 初始化头；该缺口已由后续验证 2 补齐。

### 验证 2：保存 Runtime 初始化头

学习者从同一次交互终端取得以下完整初始化头：

```text
rkllm init start
W rkllm: Warning: Your rknpu driver version is too low, please upgrade to 0.9.7
I rkllm: rkllm-runtime version: 1.3.0, rknpu driver version: 0.8.2, platform: RK3588
I rkllm: loading rkllm model from ./models/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm
I rkllm: rkllm-toolkit version: 1.2.1b1, max_context_limit: 4096, npu_core_num: 3, target_platform: RK3588, model_dtype: W8A8
I rkllm: Enabled cpus: [4, 5, 6, 7]
I rkllm: Enabled cpus num: 4
rkllm init success
```

结论：**已确认 H1 为当前直接阻塞条件**：正在使用的 RKLLM Runtime 1.3.0 自身明确警告 R1 的 RKNPU 0.8.2 低于它要求的 `0.9.7`。它与随后 W8A8 matmul 失败在时间和功能路径上吻合。该结论不等同于“任意 0.9.7+ 驱动都可安全安装”：R1 的厂商 5.10.110 内核、设备树和现有用户态仍必须匹配。H2 保留为次级兼容性风险，但不再是首要解释。

### 验证 3：RKNPU 0.9.8 候选上的同模型生成回归

学习者以 userdata→RAM 的非持久 FIT 启动 RKNPU 0.9.8 候选内核后，确认 `uname -r` 为 `5.10.252`，并确认唯一的 `/dev/dri/renderD128` 直接绑定 `/sys/bus/platform/drivers/RKNPU`，其 OF compatible 为 `rockchip,rk3588-rknpu`。随后在该候选 Linux 中，用原有 AArch64 demo、同一 `.rkllm` 模型和 `max_new_tokens=64 max_context_len=512` 输入 `ok`，实际输出为：

```text
user: ok
robot: Alright,
I rkllm:  Prefill       196.36           4         49.09                    20.37
I rkllm:  Generate      125.08           1         125.08                   7.99
I rkllm:  Peak Memory Usage (MB)
I rkllm:  1673.56
```

结论：**已验证**同一模型不再出现 `matmul(w8a8) run failed`，并完成 callback 文本输出。render 节点序号从原系统的 `renderD129` 变为 `renderD128`，原因是候选内核未编入 Rockchip display DRM；sysfs 驱动绑定而非节点号证明此轮运行实际面向 RKNPU。


## 根因

当前直接根因已确认：部署的 RKLLM Runtime 1.3.0 明确要求 RKNPU driver 至少为 0.9.7，而原 R1 运行的是 0.8.2。这个不满足的版本前置阻塞 W8A8 生成路径。RKNPU 0.9.8 候选上的同模型回归已成功，支持该版本缺口是本问题的直接原因。

## 解决或绕过方法

已采用的验证性绕过是完整内核候选，而非单独替换驱动：基于 Rockchip `develop-5.10` 构建 RKNPU 0.9.8 内核，并移植 R1 DTS。为避开该 DTS 与新显示/GPU 驱动的启动兼容问题，候选关闭 `CONFIG_DRM_ROCKCHIP` 和 Mali 驱动，但保留通用 DRM、`CONFIG_ROCKCHIP_RKNPU=y` 与 `CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y`。它通过 userdata→RAM 的 FIT 启动；未写入 p1/p3、rootfs 或 U-Boot 环境。该方案满足当前 NPU LLM 验收，但不提供显示或 Mali GPU 功能，不能替代完整板级内核适配。

## 回归验证

已完成：在 RKNPU 0.9.8 RAM 启动候选上，以同一模型、同一短 prompt `ok` 完成生成 `Alright,`；W8A8 matmul 错误未复现。性能/内存见“验证 3”。

## 经验与后续行动

- 可复用的排障方法：把“文件完整性、动态加载、模型初始化、首 token 生成”分为独立证据层，任一层失败都不越级归因。
- [ ] 唯一优先的下一步：在当前候选系统上执行一条多 token 的项目相关 prompt，记录完整生成文本与性能，作为后续 Linux→Zephyr 受限命令接口的 LLM 基线。
