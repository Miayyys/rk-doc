---
title: "ISSUE-20260816-001 候选内核 HDMI probe 空指针 Oops"
type: issue
status: resolved
created: 2026-08-16
updated: 2026-08-16
tags: [rk3588, kernel, drm, hdmi, device-tree]
related:
  - "[[status/current]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
  - "[[issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed]]"
---

# ISSUE-20260816-001 候选内核 HDMI probe 空指针 Oops

## 现象与影响

- 首次发现时间：未知；首次 RAM 启动候选内核时观察到。
- 可观察现象：候选内核启动约 71 秒后先发生 `__dw_hdmi_probe` NULL pointer dereference，随后报告 `Attempted to kill init! exitcode=0x0000000b`。
- 影响范围：RKNPU 0.9.8 候选内核不能稳定启动到现有 Ubuntu rootfs，故无法继续 NPU 回归。
- 发生频率：同一候选的两次 RAM 启动均以同一 init panic 结束；第二次保存的完整日志已定位 HDMI Oops。

## 环境与复现

- 环境基线链接：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- 最近变更：基于 Rockchip `develop-5.10` 的 RKNPU 0.9.8 内核，移植 R1 vendor DTS，并以 userdata→RAM 的 FIT 启动；未改 eMMC boot 分区。
- 最小复现步骤：U-Boot 从 `mmc 0:8` 加载候选 FIT 到 `0x0a200000`，执行 `bootm 0x0a200000#conf`。
- 预期结果：候选内核能启动现有 Ubuntu rootfs，以便验证 RKNPU 0.9.8。
- 实际结果：Rockchip DRM HDMI 初始化时 Oops，随后内核因 init 退出 panic。

## 原始证据

```text
dwhdmi-rockchip fde80000.hdmi: invalid resource
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000040
CPU: 4 PID: 1 Comm: swapper/0 Not tainted 5.10.252 #1
pc : __dw_hdmi_probe+0x774/0xad8
Call trace:
 __dw_hdmi_probe
 dw_hdmi_qp_bind
 dw_hdmi_rockchip_bind
 rockchip_drm_bind
 rockchip_drm_platform_probe
 rockchip_drm_init
 kernel_init
```

完整串口日志位于 `build/local/r1-20260816/r1-candidate-boot-attempt2.log`，该路径为可再生成的本机分析产物，尚未单独固定哈希。

## 关联知识与实验

- 相关知识点：设备树的 `status` 决定节点是否参与 probe；`reg` 属性描述驱动可索引的 MMIO 资源。
- 验证实验：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：PID 1 的用户态 systemd 自身崩溃 | panic 文本为 `Attempted to kill init` | 提取 panic 前的首次 Oops 与 CPU/调用栈 | Oops 发生在 `swapper/0` 的 HDMI DRM bind，早于 init panic | 排除为首因 |
| H2：候选 DTS 的 HDMI 资源不满足新驱动 | DTB HDMI 节点仅有一个 `reg`，新驱动会取索引 1 | 对照 DTB `reg` 与 `dw-hdmi-qp.c` | driver 在 HDCP14 分支取得 resource 1 后触发 Oops | 确认 |
| H3：禁用无关 HDMI 可让 headless NPU 候选越过该 Oops | 首个 Oops 在 HDMI component bind，项目当前不要求显示 | 隔离 DTS 仅禁用 HDMI 相关节点，重建并 RAM 启动 | 已加载并校验 headless FDT，仍出现同一 HDMI Oops；该绕过不足 | 排除 |
| H4：移除 display-subsystem 的 DRM compatible 可阻止 HDMI component bind，且不影响独立 RKNPU DRM | crash 经 `rockchip_drm_platform_probe` / `rockchip_drm_bind` 进入 HDMI；RKNPU 为独立 `RKNPU` DRM driver | 仅在隔离 DTS 删除 display-subsystem `compatible` 后 RAM 启动 | U-Boot 两次均明确加载该 display-less FDT；内核仍在 `__dw_hdmi_probe` Oops | 排除 |
| H5：关闭 `CONFIG_DRM_ROCKCHIP` 可阻止显示/HDMI 驱动进入候选内核，同时保留 RKNPU 的通用 DRM GEM 接口 | `DRM_ROCKCHIP` 与 `ROCKCHIP_RKNPU_DRM_GEM` 都直接依赖通用 `DRM`，后者不依赖前者 | 独立输出目录关闭 `DRM_ROCKCHIP`；后续还关闭 Mali 驱动，再 RAM 启动 | 最终候选进入 Linux 5.10.252，RKNPU 0.9.8 完成 RKLLM 生成，未再出现 HDMI Oops | 确认 |

## 根因

已确认的直接根因：移植后的 R1 DTS 使 `/hdmi@fde80000` 处于 `okay`，但只提供一个 MMIO `reg` 资源；Rockchip `develop-5.10` 的 HDMI 驱动在硬件检测到 HDCP14 后无空值检查地访问第二个资源，导致内核 NULL pointer dereference。根因属于该 DTS 与新 HDMI 驱动的兼容性问题，不是 NPU 或 eMMC boot 分区问题。

## 解决或绕过方法

两个设备树级绕过均已排除：headless 与移除 `display-subsystem` 的 `compatible` 都被 U-Boot 明确加载，但 HDMI 仍进入 component bind。最终临时方案是在独立候选内核配置关闭 `CONFIG_DRM_ROCKCHIP`，即根本不编入 Rockchip 显示/HDMI 驱动；通用 `CONFIG_DRM=y`、`CONFIG_ROCKCHIP_RKNPU=y` 和 `CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y` 保持启用。该候选还关闭 Mali 驱动以避免后续的 GPU probe RCU stall，随后以 userdata→RAM FIT 启动并进入 Linux，RKNPU 0.9.8 成功执行 RKLLM。它会禁用整套 Rockchip 显示 DRM 和 Mali GPU，不能作为完整板级支持方案；原 p3 系统未改。完整显示修复仍需要使 R1 HDMI 设备树与新驱动资源要求匹配，或修正驱动的空值处理。

## 回归验证

已完成：最终无 Rockchip display DRM、无 Mali GPU 的候选进入 Linux `5.10.252`；`/dev/dri/renderD128` 的 sysfs driver 为 `RKNPU`，并完成 RKLLM W8A8 文本生成。该启动过程中未再出现 `__dw_hdmi_probe` Oops。

## 经验与后续行动

- 可复用的排障方法：`Attempted to kill init` 是末端症状；先找 panic 前第一条 Oops/Call trace，再决定是用户态、内核还是设备树问题。
- [ ] 唯一优先的下一步：若未来需要显示功能，在独立实验中对齐 R1 HDMI DTS 的资源描述与 `develop-5.10` HDMI 驱动；不得把当前 headless NPU 候选写入 eMMC boot 分区。
