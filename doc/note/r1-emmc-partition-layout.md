---
title: "当前 R1 eMMC 分区布局与启动候选"
type: note
status: verified
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, r1, emmc, partition, boot]
aliases: ["R1 分区表", "eMMC 分区布局", "boot 分区"]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[note/linux-kernel-command-line]]"
  - "[[environment/software]]"
---

# 当前 R1 eMMC 分区布局与启动候选

## 学习目标

能够根据分区标签、文件系统和挂载点区分当前镜像的根文件系统与启动组件候选，并知道分区名不是内容证明。

## 前置知识

- 当前根文件系统是 eMMC 的 `/dev/mmcblk0p6`。
- 当前根文件系统的 `/boot` 为空；启动资产不在该目录。

## 当前镜像的分区地图

学习者在 R1 上读取到如下布局：

| 分区 | 大小 | 文件系统 | PARTLABEL | 当前状态与解释 |
| --- | ---: | --- | --- | --- |
| `mmcblk0p1` | 4 MiB | 未识别 | `uboot` | U-Boot 相关组件候选；内容未验证 |
| `mmcblk0p2` | 4 MiB | 未识别 | `misc` | 厂商布局中的辅助分区；内容未验证 |
| `mmcblk0p3` | 64 MiB | 未识别 | `boot` | 起始位置是一个声明 1536 字节的 DTB v17；其余内容和用途待验证 |
| `mmcblk0p4` | 128 MiB | 未识别 | `recovery` | 恢复镜像候选；内容未验证 |
| `mmcblk0p5` | 32 MiB | 未识别 | `backup` | 备份用途候选；内容未验证 |
| `mmcblk0p6` | 14 GiB | ext4 | `rootfs` | 挂载到 `/`，当前用户空间根文件系统 |
| `mmcblk0p7` | 128 MiB | ext2 | `oem` | 挂载到 `/oem` |
| `mmcblk0p8` | 14.4 GiB | ext2 | `userdata` | 挂载到 `/userdata` |

## 核心概念

`PARTLABEL` 是分区表记录的名称，表达镜像制作方的布局意图；`FSTYPE` 为空只表示 `lsblk` 没有识别出普通文件系统。两者都不能单独证明一个分区的实际镜像内容。

`file -s /dev/mmcblk0p3` 读取并识别的是分区偏移 0 处的文件头。FDT 头内的 `size=1536` 表示从该偏移开始的**一个** DTB 长度，而不是整个 `p3` 分区的长度。当前运行时 FDT 为 151552 字节，因此它不可能与 `p3` 起始处这个 1536 字节 DTB 是同一个完整 blob；两者的来源和关系仍待验证。

本板的内核命令行 `root=PARTUUID=614e0000-0000` 与 `mmcblk0p6` 的 PARTUUID 开头相符，且 `p6` 实际挂载为 `/`。这形成了“启动加载器选择 p6 → 内核挂载 p6 根文件系统”的直接证据链。

## 工作流程

```text
eMMC 分区表
  ├─ p1: uboot 候选
  ├─ p3: boot 分区 ── 开头为 1536 B DTB，下一步读取其中字符串
  └─ p6: rootfs ── 已挂载为 /
```

先读分区表，再用 `file -s` 识别候选分区的文件头；若头部表示某个嵌入 blob，还要将其声明长度与整个分区和运行时对象分别比较。只有这些边界被确认后，才讨论其中是否包含内核、ramdisk 或 DTB。不要因 `boot` 标签就直接挂载、格式化或覆盖分区。

## 实际验证

完整 `lsblk` 命令和原始输出见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 关联问题

它解释了为什么 `/boot` 为空但 Linux 仍可启动：本镜像采用 eMMC 独立分区布局，而不是把全部启动文件作为根文件系统 `/boot` 下的普通文件。

## 易错点

- 将 `boot` 分区标签直接当作“这里必定有 DTB”的证据。
- 忽略没有普通文件系统的启动分区。
- 将 `p6` 的根文件系统与 `/boot` 目录混为一谈。

## 总结

1. 当前 R1 镜像将根文件系统放在 `mmcblk0p6`。
2. `mmcblk0p3` 是 64 MiB、标签为 `boot` 的启动镜像候选。
3. `p3` 开头存在一个 1536 字节的 DTB v17，但它不是当前 151552 字节运行时 FDT 的完整副本。
4. 启动组件可能位于原始分区，不会出现在 `/boot` 目录。
5. 分区标签说明意图；镜像头只能说明相应偏移处的格式，不能代替整分区分析。
