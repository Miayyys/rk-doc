---
title: "R1 Ubuntu Camera Image V2/V3 候选镜像"
type: resource
status: active
created: 2026-08-15
updated: 2026-08-15
tags: [rk3588, r1, ubuntu, firmware, npu, camera]
related:
  - "[[status/current]]"
  - "[[experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
---

# R1 Ubuntu Camera Image V2/V3 候选镜像

## 身份与来源

- 厂商/作者：学习者说明为 youyeetoo 官方镜像；youyeetoo 的 [R1 eMMC 烧录页](https://wiki.youyeetoo.com/en/r1/burnemmc) 将 Ubuntu eMMC 烧录指向 GUI `RKDevTool` 的 “Upgrade Firmware → Firmware → Upgrade” 流程（2026-08-15 查阅）。该页未给出此本地文件的直接下载 URL、版本或厂商校验值。
- 获取日期：2026-08-15（学习者提供；精确下载完成时间未知）。
- 版本、发布日期或 commit：文件名为 `R1_UbuntuCamera_ImageV2V3.img`；文件名声称 V2/V3，尚未从发布说明或镜像内容独立验证。
- 本地位置：`/home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img`，位于仓库外，不提交到 Git。
- 许可证：待确认。

## 适用范围与边界

该文件是用于 R1 eMMC 的 Ubuntu camera 变体候选镜像；当前 R1 为用户提供的 V2、4 GB RAM、32 GB eMMC。学习者于 2026-08-15 报告已用该文件完成更新，但烧录路径与板端启动结果尚未保存和验证。镜像的分区格式、板型配置、发布完整性、NPU 运行时或 LLM 支持仍未知；`camera` 的硬件适配不自动等同于 NPU LLM 就绪。

## 完整性与验证状态

- 文件大小：9,639,807,562 字节。
- 修改时间：`2026-08-15 10:09:01 +08:00`（文件元数据，不等同于厂商发布日期或下载完成时间）。
- SHA-256：`55cd40508c70f48d05f411f9103f9cbb004456a05574e6f473007df78e2c758f`。
- 已验证：主机 `stat` 将其识别为常规文件；通用 `file -s` 输出为 `data`；文件头起始 ASCII 为 `RKFW`，是 Rockchip 固件容器而非 raw eMMC 镜像。
- 已验证：当前安装的 `rkdeveloptool` 帮助中没有完整 `RKFW` 统一固件升级子命令；其 Loader/LBA/分区/GPT/擦除命令均不能直接作用于该容器。厂商 R1 eMMC 页面仅明确指定 GUI `RKDevTool` 的统一固件升级流程；其 macOS 附录中的 `rkdeveloptool wl 0` 示例针对另一个 raw Armbian 镜像，不能套用到本候选 `RKFW`。
- 待确认：官方直接来源与厂商校验值、容器内载荷、R1 V2/V3 适用性、镜像内的 RKNN/RKNPU/RKLLM 组件，以及是否存在可验证来源、可在 Arch 上运行且支持该容器的厂商烧录客户端。

## 关联

- 格式检查：[EXP-20260815-001](../experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image.md)。
- 项目约束：[DEC-20260813-003](../decision/dec-20260813-003-npu-llm-required-project-core.md)。
