---
title: "DeepSeek-R1-Distill-Qwen-1.5B W8A8 RK3588 模型候选"
type: resource
status: active
created: 2026-08-15
updated: 2026-08-15
tags: [rk3588, npu, rkllm, model, deepseek]
related:
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
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
- **待确认**：此本地文件是否来自官方模型库、下载是否有发布方校验值、其 Toolkit/Runtime 与 R1 当前 RKNPU 0.8.2 的实际兼容性，以及它能否在 R1 的 4 GB RAM、无 swap 环境完成初始化和生成。
- 不把文件名、扩展名、大小或 SHA-256 当作 NPU 可运行性的证据；首次运行须以 `rkllm_init()` 和实际生成结果验证。

## 完整性与验证状态

- 文件大小：2,040,247,614 字节。
- SHA-256：`85123bc6796760c9e670d6676a7d3e9527d1847406807441976fe1206b04115b`。
- 已验证：主机 `stat` 与 `sha256sum` 已执行；文件已传入 R1 `/userdata/rkllm-api-demo/models/`，板端 `sha256sum` 与主机哈希一致；传输前 R1 `/userdata` 可用空间约 14 GiB。
- 待确认：模型加载、NPU 设备访问、首轮生成及内存占用。

## 关联

- 相关实验：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- 相关 SDK：[airockchip rknn-llm](airockchip-rknn-llm.md)。
