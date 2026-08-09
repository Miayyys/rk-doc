---
title: "状态变更历史"
type: status
status: active
created: 2026-08-05
updated: 2026-08-09
tags: [rk3588, progress, history]
related:
  - "[[status/current]]"
---

# 状态变更历史

本文件只追加状态变化，不替代 [`current.md`](current.md)。记录时间表示写入文档的时间，不等同于事件实际发生时间。

| 记录时间 | 事件时间 | 对象 | 原状态 | 新状态 | 原因与证据 | 信息来源 |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-08-05（具体时间未记录） | 2026-08-05（具体时间未记录） | 总体学习 | 未建档 | 阶段 0 进行中 | 建立环境与设备识别阶段；见 [`current.md`](current.md) | 用户目标与仓库记录 |
| 2026-08-05T19:46:50+08:00 | 未知 | ISSUE-20260805-001 | active | archived | 学习者确认同名空文件由本人删除，当前不再跟踪该文件；见[问题记录](../issue/issue-20260805-001-empty-loader.md) | 用户提供 |
| 2026-08-05T19:55:37+08:00 | 未知 | 板型识别 | 待确认 | 部分完成 | 学习者确认型号为 youyeetoo R1；PCB 版本仍待确认 | 用户提供 |
| 2026-08-05T19:55:37+08:00 | 未知 | 主机—板卡通信 | 未建立 | USB 枚举已报告 | 学习者报告 `lsusb` 出现 `2207:350b`；见[实验记录](../experiment/exp-20260805-001-identify-rockusb-device.md) | 用户提供 |
| 2026-08-05T19:59:39+08:00 | 未知 | Rockchip 启动模式 | 推测为 MaskROM | MaskROM 已确认 | `rkdeveloptool ld` 报告 `Maskrom`；见[实验 EXP-20260805-001](../experiment/exp-20260805-001-identify-rockusb-device.md) | 学习者终端输出 |
| 2026-08-05T20:03:10+08:00 | 未知 | 早期启动日志通道 | 待确认 | 阻塞 | 学习者确认当前仅有 Type-C 数据线，没有 USB 转串口模块；见[硬件环境](../environment/hardware.md) | 用户提供 |
| 2026-08-07T19:39:56+08:00 | 未知 | 早期启动日志通道 | 阻塞 | 进行中 | 学习者已接入 Debug UART，且主机 `/dev` 中出现 `ttyUSB*` 节点；见[实验 EXP-20260807-001](../experiment/exp-20260807-001-connect-debug-uart.md) | 用户提供 |
| 2026-08-07T19:45:39+08:00 | 未知 | 串口访问权限 | 待确认 | 已确认 | `id -nG` 显示用户 `loser` 属于设备所属组 `uucp`；见[实验 EXP-20260807-001](../experiment/exp-20260807-001-connect-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T19:55:38+08:00 | 未知 | 早期启动日志通道 | 进行中 | 已完成 | 以 1500000 baud 收到可读内核日志并进入交互 Shell；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T19:55:38+08:00 | 未知 | Linux 可启动性 | 未验证 | 已验证 | R1 重新上电后启动 Linux 并进入 `root@R1:~#`；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T19:59:06+08:00 | 未知 | 运行中内核身份 | 未识别 | 已记录 | `uname -a` 报告 Linux 5.10.110、aarch64；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:04:02+08:00 | 未知 | 根文件系统来源 | 未识别 | 已记录 | `findmnt -no SOURCE /` 报告 `/dev/mmcblk0p6`；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:07:53+08:00 | 未知 | 根文件系统物理介质 | 待确认 | eMMC 已确认 | `mmcblk0` 的 `type` 为 `MMC`，根文件系统在 eMMC 分区 `mmcblk0p6`；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:07:53+08:00 | 未知 | 用户空间发行版 | 未识别 | Ubuntu 22.04 LTS 已记录 | 登录欢迎信息报告 Ubuntu 22.04 LTS；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:11:19+08:00 | 未知 | 默认启动目标 | 未识别 | 图形目标已确认 | `systemctl get-default` 报告 `graphical.target`；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:14:59+08:00 | 未知 | 内核启动参数 | 未识别 | 已记录 | `/proc/cmdline` 显示 eMMC、PARTUUID 根分区、UART 控制台及 Android 兼容字段；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:22:47+08:00 | 2026-08-07T20:22:47+08:00 | 文档组织方式 | 普通 Markdown 文档目录 | Obsidian Vault 核心结构已建立 | 保留原有记录分类，建立唯一命名的内容索引、关联属性和 Vault 配置；见[DEC-20260807-001](../decision/dec-20260807-001-adopt-obsidian-vault.md) | 用户决定与仓库变更 |
| 2026-08-07T20:32:00+08:00 | 未知 | 用户空间发行版 | Ubuntu 22.04 LTS 已记录 | `/etc/os-release` 标准身份已确认 | 标准文件报告 Ubuntu 22.04 LTS、Jammy Jellyfish、`ID=ubuntu` 和 `ID_LIKE=debian`；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:37:12+08:00 | 未知 | 运行时设备树模型 | 未识别 | 已记录 | `/proc/device-tree/model` 报告 `Rockchip RK3588S EVB4 LP4X V10 Board`；该名称包含 RK3588S，但未出现 R1 商品名；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:39:55+08:00 | 未知 | 运行时设备树兼容关系 | 未识别 | 已记录 | `/proc/device-tree/compatible` 报告具体字符串 `rockchip,rk3588s-evb4-lp4x-v10` 和通用回退 `rockchip,rk3588`；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:43:37+08:00 | 未知 | 运行时 FDT 可访问性 | 未确认 | 已确认 | `/sys/firmware/fdt` 存在、仅 root 可读，大小为 151552 字节；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:47:24+08:00 | 未知 | 运行时 FDT 格式 | 未验证 | Device Tree Blob v17 已确认 | `file /sys/firmware/fdt` 解析出 version 17、总大小 151552、boot CPU 0、字符串区 7470 和结构区 141980；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:50:27+08:00 | 未知 | 板端 `dtc` 可用性 | 未确认 | 当前 `PATH` 中不可用 | `command -v dtc` 无输出；退出码未记录；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:52:10+08:00 | 未知 | 主机 `dtc` 可用性 | 未确认 | 当前 `PATH` 中不可用 | Arch 主机 `command -v dtc` 无输出；退出码未记录；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:54:26+08:00 | 未知 | 主机 `dtc` 可用性 | 当前 `PATH` 中不可用 | DTC v1.8.1 可执行 | 学习者运行 `dtc -v` 报告 `Version: DTC v1.8.1`；安装来源与时间待查询；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-07T20:56:14+08:00 | 2026-08-07T20:53:39+08:00 | 主机 `dtc` 安装元数据 | 版本可执行、来源待确认 | 已安装包元数据已确认 | `pacman -Qi dtc` 报告 `dtc` 1:1.8.1-1、单独指定安装、数字签名验证及安装日期；见[工具记录](../tool/dtc.md) | 学习者终端输出 |
| 2026-08-07T21:04:10+08:00 | 未知 | 板端—主机网络传输链路 | 未确认 | 当前不可用 | `ip -br link` 显示 `eth0` 为 `NO-CARRIER`；`lo` 仅本机回环，`can0` 为 DOWN 的 CAN 接口；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-09T13:23:31+08:00 | 未知 | 根文件系统启动目录 | 未检查 | 空目录已确认 | `ls -la /boot` 只显示 `.` 和 `..`；未发现内核、DTB 或启动配置；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-09T13:29:19+08:00 | 未知 | eMMC 分区布局 | 未识别 | 已记录 | `lsblk` 显示 8 个分区，其中 `p1=uboot`、`p3=boot`、`p6=rootfs`（ext4，挂载 `/`）；`p3` 内容待验证；见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md) | 学习者终端输出 |
| 2026-08-09T13:35:03+08:00 | 未知 | `p3` 起始镜像格式 | 未验证 | DTB v17 起始 blob 已确认 | `file -s /dev/mmcblk0p3` 识别出声明大小为 1536 字节的 DTB v17；该 blob 与大小为 151552 字节的运行时 FDT 不是同一个完整对象；见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) | 学习者终端输出 |
| 2026-08-09T13:38:37+08:00 | 未知 | Git 初始化 | 未初始化 | 阻塞 | `.git` 是只读 `tmpfs` 挂载点，`git rev-parse` 不能识别仓库且 `rmdir .git` 报“Device or resource busy”；见[ISSUE-20260809-002](../issue/issue-20260809-002-read-only-git-mount.md) | Agent 本机检查 |
| 2026-08-09T13:42:45+08:00 | 未知 | Git 初始化 | 阻塞 | 已完成 | `git rev-parse --is-inside-work-tree` 返回 `true`，`git status` 报告当前为尚无提交的 `master` 分支；挂载变化的执行者和事件时间未知；见[ISSUE-20260809-002](../issue/issue-20260809-002-read-only-git-mount.md) | Agent 本机检查 |
