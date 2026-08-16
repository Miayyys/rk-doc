---
title: "ISSUE-20260811-002 上游 RK3588 U-Boot 缺少外部启动载荷"
type: issue
status: archived
created: 2026-08-11
updated: 2026-08-11
tags: [rk3588, uboot, binman, firmware, boot-chain]
related:
  - "[[experiment/exp-20260809-004-acquire-upstream-uboot-source]]"
  - "[[issue/issue-20260811-001-uboot-build-missing-swig]]"
  - "[[note/uboot-fit-image]]"
  - "[[note/rockchip-external-boot-blobs]]"
  - "[[status/current]]"
---

# ISSUE-20260811-002 上游 RK3588 U-Boot 缺少外部启动载荷

## 现象与影响

- 首次发现时间：未知。
- 可观察现象：Binman 打包 `simple-bin` 时报告缺少 `rockchip-tpl` 与 `atf-bl31`，因此镜像为 non-functional，make 退出。
- 影响范围：上游 EVB RK3588 学习构建不能生成可用的完整启动镜像；此前 pylibfdt 编译已越过。未写入 R1。
- 发生频率：仅复现一次。

## 环境与复现

- 环境基线链接：[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- 最近变更：在本地实验分支应用两行 pylibfdt Python 3 API 补丁后，复用原隔离输出目录重新构建。
- 最小复现步骤：

```fish
make O=/home/loser/Study/rk3588/build/uboot-evb-rk3588 CROSS_COMPILE=aarch64-linux-gnu- -j(nproc)
```

- 预期结果：Binman 取得构造完整 Rockchip 启动镜像需要的输入并完成打包。
- 实际结果：TPL 与 BL31 缺失；OP-TEE 被报告为 optional。

## 原始证据

```text
BINMAN  .binman_stamp
Image 'simple-bin' is missing external blobs and is non-functional: rockchip-tpl atf-bl31

/binman/simple-bin/mkimage/rockchip-tpl (rockchip-tpl):
   An external TPL is required to initialize DRAM. Get the external TPL
   binary and build with ROCKCHIP_TPL=/path/to/ddr.bin.

/binman/simple-bin/fit/images/@atf-SEQ/atf-bl31 (atf-bl31):
   You may need to build ARM Trusted Firmware and build with BL31=/path/to/bl31.bin

Image 'simple-bin' is missing optional external blobs but is still functional: tee-os
Some images are invalid
make: *** [Makefile:189：__sub-make] 错误 2
```

## 关联知识与实验

- 相关知识点：TPL 负责 DDR 初始化；BL31 是 ARM Trusted Firmware 的 EL3 阶段；OP-TEE 为安全世界载荷。R1 的厂商启动日志已经观察到 BL31 与 OP-TEE，但厂商二进制不能直接当作上游 EVB 的可用输入。
- 验证实验：[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：EVB 配置选择外部 TPL | Binman 缺失项为 `rockchip-tpl` | 阅读 Rockchip Binman DTS 与最终 `.config` | DTS 显示 `CONFIG_ROCKCHIP_EXTERNAL_TPL` 条件下加入 `rockchip-tpl`；最终 config 为 `y` | 确认 |
| H2：EVB 镜像需要 BL31 作为 FIT 外部输入 | Binman 报 `atf-bl31` 且给出 `BL31=` | 阅读 Rockchip Binman DTS 中的 FIT 定义 | AArch64 条件下的 `@atf-SEQ` 含 `atf-bl31`，并使用 `fit,operation = "split-elf"` | 确认 |
| H4：`BL31` 可由 TF-A 或 Rockchip 提供的二进制得到 | Binman 提示 `BL31=` | 阅读上游 Rockchip 板级文档的 TF-A 小节 | 文档说明按平台构建 TF-A；特定 SoC 未公开时使用 Rockchip BL31，并给出 RK3588 EVB 的 BL31 ELF 示例 | 确认 |
| H5：EVB 示例的外部载荷可直接用于 R1 | 同为 RK3588 系列 | 比较 EVB 默认 DTS 与 R1 运行时根节点 | EVB1 `rk3588-evb1-v10` 与 R1 `rk3588s-evb4-lp4x-v10` 不同，仅通用 SoC 项相同 | 排除 |
| H6：当前 rkbin HEAD 保留文档所列精确文件名 | 文档列出两个文件路径 | 对当前 HEAD 执行 `test -f` | 两个精确路径均缺失 | 排除 |
| H7：可按名称近似直接选择当前 DDR TPL | 当前 HEAD 有多个 RK3588 DDR 文件 | 先列出全部候选并比较其命名维度 | 频率、`eyescan` 与版本不同，缺少选择依据 | 保留（禁止选择） |
| H8：本地 `rkbin` 的旧文件缺失是浅克隆未取得较新 master 所致 | 仓库以 `--depth 1` 取得 | 读取提交者时间并核验远端 `master` | 远端 `master` 与本地 HEAD 相同，均为 `ecb4fcbe...` | 排除 |
| H9：文档列出的 v1.33 BL31 与 v1.09 DDR TPL 曾在 rkbin 历史中存在 | 当前 master 缺失但文档点名 | 按两条精确路径查询完整历史 | 两个引入提交均已定位，随后各有版本更新提交 | 确认（是否同一提交点共存待核验） |
| H3：任意 RK3588 rkbin DDR 文件均可用于 R1 | Binman 推荐 rkbin 来源 | 对照 R1 的 SoC、DRAM、PMIC、板级资料与文件元数据 | 无证据 | 禁止假设 |

## 根因

当前直接根因是上游 EVB RK3588 构建树未提供 Binman 所需的外部 TPL 与 BL31 输入路径。`CONFIG_ARM64=y`、`CONFIG_ROCKCHIP_EXTERNAL_TPL=y` 和 `CONFIG_SPL_ATF=y` 已验证，因此外部 TPL 与 AArch64/BL31 相关打包分支确实被该 EVB 配置选中。当前本地 `rkbin` HEAD 为 `ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4`，作者时间为 `2025-12-29T11:11:15+08:00`、提交者时间为 `2025-12-30T19:41:07+08:00`，且浅克隆与远端 `master` 当前指向完全相同的提交。它缺少 U-Boot 文档列出的两份精确文件；因此“本地克隆未取得较新 master”已排除，文档与当前资源集合存在版本或命名漂移已确认。历史已定位 v1.09 DDR 的引入与 v1.10 更新、v1.33 BL31 的引入与 v1.34 更新。学习者决定不再验证旧路径是否在同一提交树中共存，也不为完成 EVB 示例选择任何旧/新二进制；该旁支到此归档。EVB/R1 兼容性仍未验证。

## 解决或绕过方法

未采取绕过。学习者决定不继续追溯资源版本，也不使用任意同名 DDR/TPL、BL31、OP-TEE 文件制造“构建成功”。若日后重开，必须先明确其为纯 EVB 学习镜像还是 R1 适配工作，并核验每个输入的来源、SoC、内存参数、版本、大小与校验值。

## 回归验证

未执行；本问题按学习者的优先级归档。若日后重开并准备外部载荷，必须重新运行同一构建并检查 Binman 是否报告有效镜像；即使构建成功，也不能作为 R1 烧录许可。

## 经验与后续行动

- 可复用的排障方法：从 Binman 报告的镜像节点倒查设备树打包描述和 Kconfig，再处理外部二进制来源。
- [x] 读取最终 `.config` 中的 `CONFIG_ROCKCHIP_EXTERNAL_TPL`、`CONFIG_SPL_ATF` 与 `CONFIG_ARM64`，确认条件分支为何在本次 EVB 构建中生效。
- [x] 读取上游 Rockchip 板级文档中 RK3588 专用的 TPL/BL31 输入说明；不下载、不烧录。
- [x] 对比 EVB1 默认设备树与 R1 的 EVB4 LP4X 运行时标识，界定该 EVB 示例不可直接用于 R1。
- [x] 核对当前 `rkbin` HEAD 的作者时间与浅克隆状态：作者时间为 `2025-12-29T11:11:15+08:00`，且为浅克隆；本地无法查询旧历史。
- [x] 核对远端 `master`：它与本地 HEAD 均为 `ecb4fcbe...`，提交者时间为 `2025-12-30T19:41:07+08:00`；当前资源与文档的精确文件名已确认漂移。
- [x] 在不选择候选文件的前提下追溯历史提交，定位文档点名的旧资源引入与替换提交。
- 未执行 `git ls-tree` 验证两个旧路径是否在同一提交树中共存：学习者决定该信息对当前学习目标价值不足，不再执行。
- [x] 按学习者决定归档本旁支；不选择、不导出、不传入任何外部二进制。
