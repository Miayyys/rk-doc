---
title: "youyeetoo R1 Linux 5.10 内核源码候选"
type: resource
status: active
created: 2026-08-15
updated: 2026-08-16
tags: [rk3588, r1, linux, kernel, rknpu]
related:
  - "[[resource/youyeetoo-r1-documentation-repository]]"
  - "[[issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
---

# youyeetoo R1 Linux 5.10 内核源码候选

## 身份与来源

- 厂商/作者：youyeetoo GitHub 组织。
- 来源 URL：<https://github.com/youyeetoo/r1-linux-kernel.5.10>。
- 获取日期：2026-08-15；主机以 `--depth 1` 浅克隆。
- 版本、发布日期或 commit：默认分支 `master`，HEAD `82c69382f596e98f0458ae929966bcde28483af1`，作者时间 `2024-05-12T11:10:47+08:00`，提交主题 `r1-kernel v0.0.0`；未验证 tag 或签名。
- 本地位置：`src/youyeetoo-r1-linux-kernel-5-10`。
- 许可证：仓库页面列出 `LICENSES/`，具体适用于 R1 改动的许可证待本地核对。

## 适用范围与边界

- **资料记载**：R1 官方 README 的 Ubuntu source code compilation 链接指向 youyeetoo Wiki；该 Wiki 入口对应到此 youyeetoo 组织的 Linux 5.10 完整内核树。
- **已验证**：本地工作树当前报告 `master...origin/master` 且无短状态输出；该完整内核树含 `drivers/rknpu/`。其 RKNPU Makefile/Kconfig 使用 `CONFIG_ROCKCHIP_RKNPU`，并提供 `CONFIG_ROCKCHIP_RKNPU_DRM_GEM` 路径；这与 R1 已验证的内建配置相符。
- **已验证**：`drivers/rknpu/rknpu_drv.c` 定义 `DRIVER_MAJOR=0`、`DRIVER_MINOR=8`、`DRIVER_PATCHLEVEL=2`，即该树的 RKNPU 软件版本为 `0.8.2`；与当前板端 Runtime 报告的版本相同，低于 RKLLM Runtime 1.3.0 明示的最低 `0.9.7`。
- **已验证**：此提交含厂商 Ubuntu 页面列出的 `arch/arm64/configs/rockchip_linux_defconfig`（16,067 B）和 `rk3588_linux.config`（90 B）配置输入；后者禁用两个 BCMDHD 选项并启用 `CONFIG_MALI_CSF_SUPPORT=y`。还含 R1 DTS `arch/arm64/boot/dts/rockchip/rk3588s-yyt.dts`，但尚未找到 `rk3588s-yyt.img` 的构建规则，页面命令能否在此独立内核树直接产生该文件待验证。
- **待确认**：该树是否精确对应 R1 当前 Ubuntu 的 `Linux 5.10.110 #4`、R1 EVB4 LP4X DTS/配置、完整固件打包步骤和回退方式。
- 文件归属为厂商组织不等于任何 `master` 提交或构建产物可安全替换当前 eMMC 启动镜像。
- **资料对照 + 已验证版本宏**：官方 `rockchip-linux/kernel` 的 `develop-5.10`、`develop-6.1`、`develop-6.6` 分支均在同一路径声明 RKNPU `0.9.8`（日期 `20240828`）。这使 `develop-5.10` 成为优先研究的升级候选：RKNPU 满足当前 Runtime 的最低要求，且与 R1 现有 5.10 基线同代；精确提交、R1 DTS 合并和可启动性尚未验证。
- **已验证**：固定的官方 `develop-5.10` 候选含 `rk3588s-evb4-lp4x-v10.dts`，与 R1 运行时设备树的首个 compatible 对应；不含本地厂商树的 `rk3588s-yyt.dts`。两份顶层 DTS 的完整 diff 仅有一处：官方包含 `rk3588s-evb4-lp4x.dtsi` 与 `rk3588-android.dtsi`，R1 包含 `rk3588s-evb4-lp4x-yyt.dtsi` 与 `rk3588-linux.dtsi`；`model` 后的顶层正文相同。升级工作应以这四份基础 include 的实际内容为准，不能仅按同一 compatible 推断板级配置完全相同。
- **已验证**：上述四份 include 中，只有 `rk3588s-evb4-lp4x-yyt.dtsi` 不存在于固定官方候选；标准 EVB4、Linux、Android 三份同名基础文件均在两棵树存在。Yyt 文件因此是定位 R1 厂商板级差异的首要来源；同名文件是否字节或语义相同仍待确认。
- **已验证**：三个同名基础 include 经 `cmp -s` 均判定为不同，故 R1 DTS 不能按“仅复制 Yyt 文件”移植。后续应把 R1 的完整 DTS include 依赖包放入独立工作树，以新 DTB 名称先编译验证；不能覆盖官方 EVB4 文件。

## 完整性与验证状态

- 文件大小：未获取。
- SHA-256：不适用；已固定 Git commit。
- 已验证：本地 Git 身份、完整内核树与 `drivers/rknpu/`、RKNPU 配置符号/DRM GEM 路径。
- **已验证**：一次只读核对时，工作树曾只剩 Git 元数据和少量点文件，`git status` 报告 84,052 个受跟踪文件缺失；未启用 sparse checkout，`git fsck` 无错误且 `HEAD` 保留这些文件。学习者执行 `git restore --source=HEAD --worktree -- .` 后恢复 84,058 个索引项并回到干净状态。该现象不是仓库的正常“官方裁剪”状态。
- 待确认：R1/Ubuntu 内核精确匹配、构建和恢复路径；以及是否存在厂商维护的、适配 R1 且 RKNPU >=0.9.7 的另一份源码或完整镜像。

## 关联

- 相关问题：[ISSUE-20260815-002](../issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed.md)。
- 相关实验：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
