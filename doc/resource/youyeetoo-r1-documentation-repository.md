---
title: "youyeetoo R1 官方文档索引仓库"
type: resource
status: verified
created: 2026-08-12
updated: 2026-08-16
tags: [rk3588, r1, vendor, documentation, pinout]
related:
  - "[[environment/hardware]]"
  - "[[experiment/exp-20260812-003-check-running-kernel-source-availability]]"
  - "[[status/current]]"
---

# youyeetoo R1 官方文档索引仓库

## 身份与本地位置

- 厂商/作者：youyeetoo。
- 上游地址：<https://github.com/youyeetoo/R1>。
- 本地目录：`R1/`（仓库根目录下的独立 Git 工作树；未移动、未修改）。
- 已验证 HEAD：`816bd5413ab9a6470f7c3b3add98d9f795ff6378`，提交主题 `Update README.md`，作者时间 `2025-01-17T19:57:43+08:00`。
- 工作树：`main...origin/main`，盘点时无未提交改动；不是浅克隆。
- 许可证：仓库内 `LICENSE` 为 GPL-3.0 文本。

## 内容边界

本仓库只有 `README.md`、`LICENSE` 与 Git 元数据；**不含** R1 内核源码、U-Boot 源码、固件镜像、Loader 或原理图文件本体。README 的 SHA-256 为 `121c5ab30420c2c88133f56bae37328a708ee403c33dc7099b9000f187d6047e`，大小为 24739 字节。

因此它是官方资料的导航入口，不能作为当前 `5.10.110` 内核的匹配源码树，也不能直接提供可烧录文件。

## 对当前学习有用的内容

- 30PIN PH2.0 逐针表：供电、GND、默认功能、复用功能、电压与驱动强度。
- R1 原理图与 2D 图下载入口：用于后续核对排针 1 脚方向、接口和电气连接。
- Ubuntu 源码编译、eMMC/TF 烧录、镜像重打包链接：仅在完成备份、板型和输入文件核对后使用。
- Ubuntu 外设使用与 Debian 编程链接：GPIO、I2C、UART、PWM、ADC、SPI、NPU 等主题的厂商示例入口。

## 2026-08-16：完整 BSP 入口线索

- **已验证**：本地 README 的 R1 固件/源码章节明确链接 Ubuntu 源码编译页：<https://wiki.youyeetoo.com/en/r1/Ucompile>；同一索引标注 R1 支持 Ubuntu 22.04、Debian 11、Android 13 和 Buildroot，均为 kernel 5.10。
- **资料记载**：官方 R1 Ubuntu 页面 <https://wiki.youyeetoo.com/r1/ubuntu> 列出“Get Ubuntu Source Code”“Build the OS Image”和构建选项章节，但本次网页文本抓取没有给出 Ubuntu 源归档文件名或下载 URL。
- **资料记载，不能等同于 Ubuntu SDK**：官方 Debian 编译页 <https://wiki.youyeetoo.com/en/r1/Dcompile> 给出完整 SDK 的组织示例：`r1_linux_release_v2.0_v3.0_20240928_sdk.tar.gz`、本地 repo 同步、`kernel/` 子目录和 `rk3588s-yyt.img` 内核产物。这证明厂商存在 R1 完整 BSP 发布形式，但不能证明该 Debian SDK、其 RKNPU 版本或其 kernel 与当前 Ubuntu R1 一致。
- **用户提供截图 + 已验证**：学习者在 `Ucompile`（标题“Ubuntu编译步骤”）的“解压源码”区看到的也只有同一组 “R1-Debian Source” 官方/Baidu 链接；其中 Google Drive 文件夹 <https://drive.google.com/drive/folders/1AYpMrvScgpaZQYcPmMsULS1woYz5tKPk?usp=drive_link> 与 `Dcompile` 页面所列链接一致。因此它不能再被排除为“与 Ubuntu 无关”；它是同时被 Ubuntu 页面引用、但仍以 Debian 命名的 R1 Linux SDK 候选。**待确认**它是供多种 rootfs 共用的 BSP，还是 Ubuntu 页面文案/链接复用错误；未下载、未使用。
- **待确认**：该 Debian 标注 SDK 对 Ubuntu 的实际构建配置、归档文件名/发布日期、R1 V2 适用性、内核提交与 RKNPU 版本。未下载、未构建、未烧录。

## 已发现的资料冲突

README 摘要称 7 路 GPIO 为 `GPIO1_A7`、`GPIO1_A4`、`GPIO1_D5`、`GPIO2_A6`、`GPIO1_B1`、`GPIO0_B0`、`GPIO0_A0`；但同一 README 的逐针表显示 pin 9 为 `GPIO1_A6`，并明确列出 pin 11、13、15、17、25。摘要与逐针表不能视为完全一致。涉及接线时以逐针表、R1 V2 实物方向、运行时 pinctrl 状态和原理图交叉验证，不以摘要列表单独决定。

## 参考

- 本机 `R1/README.md` 与 Git 元数据，于 2026-08-12 盘点。
