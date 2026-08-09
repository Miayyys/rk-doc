---
title: "软件环境基线"
type: environment
status: active
created: 2026-08-05
updated: 2026-08-09
tags: [arch-linux, rk3588]
related:
  - "[[status/current]]"
---

# 软件环境基线

| 项目 | 当前记录 | 证据等级 |
| --- | --- | --- |
| 主机发行版 | Arch Linux | 用户提供 |
| 主机内核 | 未记录 | 待确认 |
| 主机架构 | 未记录 | 待确认 |
| 当前项目 Git | Git 2.55.0；有效工作树，分支 `master`，尚无提交 | 已验证 |
| 板端操作系统 | Ubuntu 22.04 LTS，Jammy Jellyfish，`ID=ubuntu`、`ID_LIKE=debian`（`/etc/os-release`） | 已验证 |
| 默认图形桌面状态 | systemd 默认目标为 `graphical.target`；显示实际出画未验证 | 已验证 / 待确认 |
| Bootloader 版本 | 未知 | 待确认 |
| 板端内核版本 | `5.10.110 #4 SMP Sun Sep 29 10:38:13 CST 2024` | 已验证 |
| 板端主机名 | `R1` | 已验证 |
| 板端架构 | `aarch64`（64 位 ARM） | 已验证 |
| 根文件系统挂载源 | `/dev/mmcblk0p6` | 已验证 |
| 根文件系统物理介质 | eMMC（`/sys/block/mmcblk0/device/type` 为 `MMC`） | 已验证 |
| 根文件系统 `/boot` | 空目录；未发现内核、DTB 或启动配置 | 已验证 |
| eMMC 分区布局 | 8 分区；`p1=uboot`、`p3=boot`（开头为 1536 字节 DTB v17）、`p6=rootfs`（ext4，挂载 `/`）、`p7=oem`、`p8=userdata` | 已验证 |
| 运行时设备树模型 | `Rockchip RK3588S EVB4 LP4X V10 Board`；未出现 R1 商品名 | 已验证 |
| 运行时设备树兼容列表 | `rockchip,rk3588s-evb4-lp4x-v10`；回退到 `rockchip,rk3588` | 已验证 |
| 运行时 FDT 导出 | `/sys/firmware/fdt`，root 只读，151552 字节 | 已验证 |
| 板端设备树编译器 | `dtc` 在当前 Root Shell 的 `PATH` 中不可用 | 已验证 |
| 主机设备树编译器 | `dtc` 1:1.8.1-1，DTC v1.8.1；2026-08-07T20:53:39+08:00 单独指定安装，数字签名验证 | 已验证 |
| 设备树文件、来源与版本 | 未知 | 待确认 |
| 交叉编译工具链 | 未安装或未记录 | 待确认 |
| RK3588 工具 | `rkdeveloptool`，路径 `/usr/bin/rkdeveloptool` | 用户提供 |
| Rockchip USB 设备 | `DevNo=1 Vid=0x2207,Pid=0x350b,LocationID=106 Maskrom` | 已验证 |

首次采集信息时保留命令、完整输出和日期；不要仅写“最新版”。工具升级后更新本页，并在受影响的实验中注明版本变化。

内核构建日期是镜像或内核的构建元数据，不等同于开发板当前系统时间或首次启动时间。内核、发行版标准身份和运行时设备树属性信息来源：[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
