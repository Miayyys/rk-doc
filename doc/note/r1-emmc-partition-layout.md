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
  - "[[note/uboot-fit-image]]"
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
| `mmcblk0p1` | 4 MiB | 未识别 | `uboot` | 起始为 2560 字节 RK3588S EVB4 平台 FDT（不是 FIT 元数据）；全分区文本另含 ATF、OP-TEE、U-Boot、MCU FIT 与 Youyeetoo U-Boot 构建标识，所属对象和布局待定位 |
| `mmcblk0p2` | 4 MiB | 未识别 | `misc` | 厂商布局中的辅助分区；内容未验证 |
| `mmcblk0p3` | 64 MiB | 未识别 | `boot` | 起始为 U-Boot FIT；已解析 `fdt`、`kernel`、`resource` 的偏移和大小 |
| `mmcblk0p4` | 128 MiB | 未识别 | `recovery` | 恢复镜像候选；内容未验证 |
| `mmcblk0p5` | 32 MiB | 未识别 | `backup` | 备份用途候选；内容未验证 |
| `mmcblk0p6` | 14 GiB | ext4 | `rootfs` | 挂载到 `/`，当前用户空间根文件系统 |
| `mmcblk0p7` | 128 MiB | ext2 | `oem` | 挂载到 `/oem` |
| `mmcblk0p8` | 14.4 GiB | ext2 | `userdata` | 挂载到 `/userdata` |

## 核心概念

`PARTLABEL` 是分区表记录的名称，表达镜像制作方的布局意图；`FSTYPE` 为空只表示 `lsblk` 没有识别出普通文件系统。两者都不能单独证明一个分区的实际镜像内容。

`file -s /dev/mmcblk0p3` 读取并识别的是分区偏移 0 处的文件头。FDT 头内的 `size=1536` 表示 FIT 元数据树的长度，而不是整个 `p3` 分区或板级设备树载荷的长度。反编译后的 FIT 明确给出本镜像的 `data-position` 与 `data-size`。详见[FIT 笔记](uboot-fit-image.md)。

### 当前 `p3` FIT 的已解析布局

以下偏移均从 `p3` 起始处计算；范围右端不包含在内。

| 组成 | 起始偏移 | 大小 | 结束偏移 | 说明 |
| --- | ---: | ---: | ---: | --- |
| FIT 元数据树 | `0x000000` | `0x600`（1536 B） | `0x000600` | `file` 识别到的 FDT 头和配置树 |
| `fdt` | `0x000800`（2048） | `0x24172`（147826 B） | `0x024972` | 类型为 `flat_dt`；FIT 声明的 SHA-256 已与实际载荷匹配 |
| `kernel` | `0x024a00`（150016） | `0x220da00`（35707392 B） | `0x2232400` | `arm64`、`linux`、未压缩内核载荷 |
| `resource` | `0x2232400`（35857408） | `0x9c000`（638976 B） | `0x22ce400` | `multi` 类型资源载荷 |
| FIT 末尾填充 | `0x22ce400` | `0x400`（1024 B） | `0x22ce800` | 根节点 `totalsize` 结束位置 |

三个载荷按 `fdt → kernel → resource` 排列；`fdt` 与 `kernel` 之间有 `0x8e` 字节填充。根节点的 `totalsize=0x22ce800` 是 FIT 属性，不能与 FDT 二进制头的 1536 字节大小混淆。

当前运行时 FDT 为 151552 字节，而 FIT 中 `fdt` 载荷为 147826 字节，因此两者不可能是同一个未修改的原始 blob。运行时 FDT 可能被启动加载器修改、来自其他位置，或存在其他差异；原因待验证。

本板的内核命令行 `root=PARTUUID=614e0000-0000` 与 `mmcblk0p6` 的 PARTUUID 开头相符，且 `p6` 实际挂载为 `/`。这形成了“启动加载器选择 p6 → 内核挂载 p6 根文件系统”的直接证据链。

后续在 U-Boot 执行 `part list mmc 0`，显示的 8 个标签、LBA 范围和 Partition GUID 与 Linux `/dev/mmcblk0` 完全一致。因此本板 U-Boot 的 `mmc 0` 就是 Linux 的 eMMC `mmcblk0`；`boot_fit` 帮助中的 `boot`/`recovery` 分区可对应到 `p3`/`p4`。早期 SPL 输出的 `Trying fit image at 0x4000 sector` 与该表 `uboot` 分区的起始 LBA 相同，支持早期 FIT 从 `p1` 起点读取；但 `p1` 起始 FDT 的非标准结构与 FIT 日志间的具体封装关系仍待解释。

## 工作流程

```text
eMMC 分区表
  ├─ p1: uboot 候选
  ├─ p3: boot 分区 ── FIT 元数据 → fdt(0x800) → kernel(0x24a00) → resource(0x2232400)
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
3. `p3` 开头的 1536 字节 FDT 是 U-Boot FIT 元数据树，不是板级设备树本体。
4. 启动组件可能位于原始分区，不会出现在 `/boot` 目录。
5. FIT 已给出三类载荷的位置和大小；`fdt` 实际数据已匹配 FIT 声明的 SHA-256，`kernel` 与 `resource` 尚未校验。
6. 后续 U-Boot 启动日志已实际加载并校验与 `p3` FIT 声明一致的 `kernel`、`fdt`；`resource` 的本次启动用途仍未观察到。
