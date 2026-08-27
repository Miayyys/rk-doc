---
title: "DeepSeek-R1-Distill-Qwen-1.5B W8A8 RK3588 模型候选"
type: resource
status: active
created: 2026-08-15
updated: 2026-08-22
tags: [rk3588, npu, rkllm, model, deepseek]
related:
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
  - "[[experiment/exp-20260821-003-build-r1-psci-cpu-on-heartbeat]]"
  - "[[resource/airockchip-rknn-llm]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
---

# DeepSeek-R1-Distill-Qwen-1.5B W8A8 RK3588 模型候选

## 身份与来源

- 厂商/作者：待确认。
- 来源 URL：学习者未保存精确下载页面；文件名与 airockchip RKLLM API demo 所示示例名一致。官方 SDK README 的模型库入口为 <https://console.box.lenovo.com/l/l0tXb8>（提取码 `rkllm`）。
- 获取日期：2026-08-15（学习者主机文件时间显示 22:40；时区及精确事件时间未独立核对）。
- 版本、发布日期或 commit：待确认。
- 本地位置：`/home/loser/Downloads/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm`。
- 许可证：待确认；不得仅凭模型名推断原模型或转换产物的许可证。

## 适用范围与边界

- **已验证**：文件名为 `DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm`，扩展名为 `.rkllm`，大小为 2,040,247,614 字节。
- **资料记载**：airockchip 的纯文本 API demo 以同名模型作为 RK3588 示例。
- **来源待确认**：此本地文件是否来自官方模型库、下载是否有发布方校验值、转换版本与许可证仍未知；不得把 Lenovo 模型库入口或文件名当作来源证明。
- **兼容性事实**：原厂 RKNPU 0.8.2 组合未通过；在 RKNPU 0.9.8 + RKLLM Runtime 1.3.0 的 RAM 候选中，同一文件已完成初始化和短文本生成，但这不等于来源、转换链或持久化镜像已确认。
- 不把文件名、扩展名、大小或 SHA-256 当作 NPU 可运行性的证据；首次运行须以 `rkllm_init()` 和实际生成结果验证。

## 完整性与验证状态

- 文件大小：2,040,247,614 字节。
- SHA-256：`85123bc6796760c9e670d6676a7d3e9527d1847406807441976fe1206b04115b`。
- 已验证：主机 `stat` 与 `sha256sum` 已执行；文件已传入 R1 `/userdata/rkllm-api-demo/models/`，板端 `sha256sum` 与主机哈希一致；传输前 R1 `/userdata` 可用空间约 14 GiB。
- **已验证（RAM 候选）**：模型加载、NPU 访问和短文本生成成功，输出 `Alright,`；运行时记录峰值内存约 1673.56 MB，候选未写入 eMMC。
- **待确认**：精确下载来源、转换/发布版本、许可证，以及该候选在持久化镜像上的重复回归。

## 关联

- 相关实验：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- 相关 SDK：[airockchip rknn-llm](airockchip-rknn-llm.md)。
