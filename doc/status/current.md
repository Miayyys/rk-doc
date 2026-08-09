---
title: "当前学习状态"
type: status
status: active
created: 2026-08-05
updated: 2026-08-09
tags: [rk3588, progress]
related:
  - "[[status/history]]"
  - "[[roadmap/learning-roadmap]]"
  - "[[environment/environment-index]]"
  - "[[issue/issue-20260805-001-empty-loader]]"
  - "[[issue/issue-20260809-002-read-only-git-mount]]"
  - "[[tool/dtc]]"
  - "[[note/r1-emmc-partition-layout]]"
---

# 当前学习状态

## 当前阶段

**阶段 0：设备识别、环境建档与调试条件准备。** 当前重点不是烧录系统，而是确认板卡身份、已有启动状态、可用接口和安全恢复条件。当前已确认 R1 从 eMMC 启动默认图形目标的 Ubuntu 22.04 LTS，且 Debug UART 可交互；尚未确认完整启动链、图形实际出画和先前进入 MaskROM 的条件。

## 已知事实

- **用户提供**：开发板使用 RK3588，配置为 4 GB RAM、32 GB 存储；当前没有存储卡，已有配件仅有电源。
- **用户提供**：主机运行 Arch Linux；有桌面 Linux 经验，刚接触嵌入式 Linux。
- **用户提供**：长期方向为边缘 AI，同时希望深入学习启动流程、内核和底层驱动。
- **用户提供**：开发板准确型号为风火轮（youyeetoo）R1。
- **用户提供**：开发板为 R1 V2；PCB 丝印照片或文字尚未保存。
- **已验证**：R1 Debug UART 的 USB 转串口模块为 `/dev/ttyUSB0`；其所属组为 `uucp`，当前用户 `loser` 属于该组，可使用普通用户权限访问。见[实验 EXP-20260807-001](../experiment/exp-20260807-001-connect-debug-uart.md)。
- **已验证**：R1 重新上电后通过 Debug UART 启动 Linux，并进入可交互的 `root@R1:~#` Shell；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：运行中的内核为 Linux `5.10.110 #4 SMP`，目标架构为 `aarch64`；`/etc/os-release` 确认用户空间为 Ubuntu 22.04 LTS（Jammy Jellyfish，`ID=ubuntu`、`ID_LIKE=debian`）。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：根文件系统 `/` 挂载自 `/dev/mmcblk0p6`。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`mmcblk0` 类型为 `MMC`，根文件系统位于 eMMC；登录欢迎信息表明用户空间为 Ubuntu 22.04 LTS。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：systemd 默认目标为 `graphical.target`；图形会话的实际显示输出尚未验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`/proc/cmdline` 包含 eMMC 根分区、Debug UART 控制台和 Android 兼容字段；详见[启动参数笔记](../note/linux-kernel-command-line.md)。
- **已验证**：当前运行时设备树 `model` 为 `Rockchip RK3588S EVB4 LP4X V10 Board`，其中包含 SoC 型号 RK3588S，但未出现 R1 商品名；这不能单独推翻实物为 R1 的用户提供信息。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：运行时设备树 `compatible` 依次为 `rockchip,rk3588s-evb4-lp4x-v10` 与 `rockchip,rk3588`，即由具体评估板描述回退到通用 SoC 描述；详见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：内核导出 root 只读的运行时 FDT 二进制 `/sys/firmware/fdt`，大小为 151552 字节；它可用于研究本次启动使用的设备树，但不自动表明 eMMC 中的 DTB 文件来源。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`file` 将运行时 FDT 识别为 Device Tree Blob version 17；该版本是二进制格式版本，大小为 151552 字节，不能当作板型或构建版本。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：当前板端 Root Shell 的 `PATH` 中没有可直接调用的 `dtc`；后续反编译工具应优先在 Arch 主机侧确认，避免为阅读 DTB 改动板端环境。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机已安装 `dtc` 包 `1:1.8.1-1`，DTC v1.8.1 可执行；于 2026-08-07T20:53:39+08:00 单独指定安装，包元数据报告数字签名验证。见[dtc 工具记录](../tool/dtc.md)。
- **已验证**：板端 `eth0` 已启用但为 `NO-CARRIER`，当前无可用以太网物理链路；`lo` 仅本机回环，`can0` 是当前 DOWN 的 CAN 接口，均不能用于传输运行时 FDT。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：当前已挂载根文件系统的 `/boot` 为空，未发现内核镜像、DTB、`extlinux/` 或其他启动配置；这不排除 eMMC 独立分区中存在启动组件。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：当前 eMMC 有 8 个分区；`p1` 标签为 `uboot`，`p3` 标签为 `boot`，`p6` 为挂载到 `/` 的 ext4 `rootfs`。`p3` 的开头是一个声明大小为 1536 字节的 DTB v17，且不等同于大小为 151552 字节的运行时 FDT；其用途和其余内容待验证。详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。
- **已验证**：Arch Linux 主机的 `/usr/bin/rkdeveloptool` 将设备报告为 `DevNo=1 Vid=0x2207,Pid=0x350b,LocationID=106 Maskrom`；见[实验 EXP-20260805-001](../experiment/exp-20260805-001-identify-rockusb-device.md)。
- **已验证**：当前项目已经是有效 Git 工作树；当前分支为 `master`，尚无提交，`.gitignore`、`AGENTS.md` 和 `doc/` 均未跟踪。Agent 环境以只读方式访问 `.git`。见[ISSUE-20260809-002](../issue/issue-20260809-002-read-only-git-mount.md)。
- **资料记载**：R1 使用 RK3588S，32 GB 配置为板载 eMMC 可选规格；见[硬件环境基线](../environment/hardware.md)。
- **推测**：先前进入 MaskROM 可能由按键、上电/连接顺序或当时的启动介质状态触发；该推测尚未解释为何已可启动的 eMMC 当时未被加载。
- **已验证**：`rk3588_spl_loader.bin` 曾被观察到存在且大小为 0 字节，随后已不存在。
- **用户提供**：该文件由学习者主动删除；具体删除时间未记录。相关问题已归档，详见 [ISSUE-20260805-001](../issue/issue-20260805-001-empty-loader.md)。

## 进度

| 项目 | 状态 | 完成证据 |
| --- | --- | --- |
| 明确总体学习方向 | 已完成 | 本文“已知事实” |
| 建立文档结构和记录规范 | 已完成 | [知识库首页](../home.md)与[记录规范](../recording-standard.md) |
| 建立 Obsidian 知识关联与查阅入口 | 已完成 | [DEC-20260807-001](../decision/dec-20260807-001-adopt-obsidian-vault.md)与[知识库首页](../home.md) |
| 确认准确板卡型号和硬件版本 | 部分完成 | 型号为 youyeetoo R1 V2；PCB 丝印仍未记录 |
| 建立主机与板卡通信链路 | 已完成 | Rockchip USB 和 Debug UART 通信均已验证；UART 可进入 Root Shell |
| 准备早期启动日志通道 | 已完成 | `picocom` 以 1500000 baud 接收可读内核日志并支持交互 |
| 验证 Linux 可启动 | 已完成 | 重新上电后获得内核日志和 `root@R1:~#`；见 EXP-20260807-002 |
| 备份或确认可恢复方案 | 待进行 | 尚无证据 |

## 当前阻塞与未知信息

- **待确认**：R1 V2 的 PCB 丝印、Type-C 实物接口标签与其具体针脚定义。
- **待确认**：完整启动链所在介质、图形会话是否实际出画、`p3` 起始小 DTB 的用途与其余内容、eMMC 中实际运行时 DTB 的来源、U-Boot 版本，以及可用的板端—主机文件传输链路。
- **待确认**：电源规格、上电时 LED/显示表现和 Type-C 所在的实物接口标签。
- **待确认**：USB 转串口模块的芯片与逻辑电平；先前 MaskROM 与本次 Linux 启动的触发条件。

## 唯一下一步

在 R1 Root Shell 执行 `dd if=/dev/mmcblk0p3 bs=1536 count=1 status=none | strings -n 3`，只读取 `p3` 起始 DTB 的声明范围并查看可打印字符串。先比较它是否透露 `model`、`compatible` 或其他用途线索；不挂载、不复制、不修改该分区。
