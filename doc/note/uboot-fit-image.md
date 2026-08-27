---
title: "U-Boot FIT 镜像：FDT 容器与启动载荷"
type: note
status: verified
created: 2026-08-09
updated: 2026-08-21
tags: [uboot, fit, fdt, boot]
aliases: ["FIT image", "Flattened Image Tree", "FIT 镜像"]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[note/r1-emmc-partition-layout]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[issue/issue-20260820-001-resource-dtb-overrides-fit-dtb]]"
---

# U-Boot FIT 镜像：FDT 容器与启动载荷

## 学习目标

能够解释为什么 FIT 会被 `file` 识别成 DTB，并区分 FIT 元数据树、其中引用的板级 FDT 与内核运行时 FDT。

## 前置知识

- FDT/DTB 是一种树形二进制表示，不只用于描述硬件。
- U-Boot 位于 Linux 内核之前，负责选择并加载启动组件。

## 核心概念

FIT（Flattened Image Tree）是 U-Boot 用于组织启动镜像的格式。它使用 FDT 结构描述两类核心节点：`/images` 列出内核、设备树、ramdisk 或其他载荷；`/configurations` 指定一次启动选择哪些载荷。哈希或签名节点可用于完整性和真实性校验。

因此，FIT 的开头本身是一个合法 FDT。`file` 只能依据二进制头识别为 Device Tree Blob，不能判断它是“硬件设备树”还是“FIT 描述树”。要结合 `description`、`images` 和 `configurations` 等节点判断语义。

```text
FIT（FDT 格式的元数据树）
  ├─ images
  │   ├─ kernel   → Linux 内核载荷
  │   ├─ fdt      → 板级设备树载荷
  │   └─ resource → 资源载荷
  └─ configurations
      └─ conf      → 将上述载荷组合成启动方案
```

当 FIT 使用 `data-position`、`data-offset` 与 `data-size` 时，元数据节点记录载荷的位置和长度，实际二进制可放在 FDT 元数据之外。于是“FIT 头只有 1536 字节”和“分区中还有较大的内核、FDT、资源数据”并不矛盾。

### R1 FIT 中三个载荷的含义

| 节点 | 配置的对象 | R1 已读取的关键属性 | U-Boot 需要据此做什么 |
| --- | --- | --- | --- |
| `fdt` | 供 Linux 使用的板级设备树载荷 | `type = "flat_dt"`、`arch = "arm64"`、`data-position = <0x800>`、`data-size = <0x24172>`、`compression = "none"`、SHA-256 | 从 FIT 中取出 DTB，并作为内核启动时的硬件描述 |
| `kernel` | Linux 内核载荷 | `type = "kernel"`、`os = "linux"`、`arch = "arm64"`、`data-position = <0x24a00>`、`data-size = <0x220da00>`、`load`、`entry`、SHA-256 | 取出未压缩内核，按镜像的加载/入口语义准备跳转 |
| `resource` | 附属资源载荷 | `type = "multi"`、`arch = "arm64"`、`data-position = <0x2232400>`、`data-size = <0x9c000>`、SHA-256 | 交给厂商启动流程或相关组件处理；R1 已验证其内含最终 DTB |

三个节点共有的 `data-position` 和 `data-size` 定义“从 FIT 起始位置取哪一段字节”；`compression = "none"` 表示这些载荷在该镜像中没有压缩；`hash` 节点保存对应数据的 SHA-256 期望值。`load`、`entry` 的具体数值由 U-Boot 和镜像类型解释，不能仅凭本 DTS 中的 `0xffffff00`、`0xffffff01` 当作普通物理内存地址。

`configurations/conf` 是默认启动方案：它用 `fdt = "fdt"`、`kernel = "kernel"` 和 `multi = "resource"` 把三个节点引用在一起。其 `signature` 子节点描述应覆盖的对象、算法、填充与键提示；它本身不证明本次启动已经完成签名验证。

### R1 厂商 resource 对最终 DTB 的影响

R1 的 `resource` 不是纯 logo 附件。它是带索引的厂商格式，原始内容含 `rk-kernel.dtb`、`logo.bmp` 与 `logo_kernel.bmp`。原 resource 的 `rk-kernel.dtb` 与 FIT 的 `fdt` 子镜像字节相同；但在 RAM 测试中，即使 FIT `fdt` 已替换成 AMP DTB，Linux 仍收到旧树。只替换并重建 resource 的 `rk-kernel.dtb` 后，Linux 才得到 AMP 树（7 CPU、`cpu@300` 缺失、`zephyr@50000000` 存在）。

因此，对这版 R1 厂商 U-Boot 路径，`resource/rk-kernel.dtb` 是实际 Linux DTB 的输入之一，不能仅替换 FIT 的 `fdt` 子镜像。这个结论限于当前厂商 U-Boot 和 RAM 启动实测路径；它不推广到通用 U-Boot/FIT，也不说明具体板级函数如何实现该替换。

## 工作流程

先识别 FDT 头，再读取树结构及属性值，最后根据位置和长度提取单个载荷并单独验证格式、哈希与用途。`strings` 适合发现关键词，但会丢失节点层次、属性边界和数值，也会显示哈希字节中偶然形成的可打印文本。

## 实际验证

[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md) 已将 R1 的 1536 字节 FIT 元数据反编译为 DTS。`conf` 是默认配置，引用 `fdt`、`kernel` 和 `resource`；其中 `fdt` 位于偏移 `0x800`、大小 `0x24172`，`kernel` 位于 `0x24a00`、大小 `0x220da00`，`resource` 位于 `0x2232400`、大小 `0x9c000`。板端对 `fdt` 范围计算的 SHA-256 已匹配 FIT 中该节点的声明值。具体布局见[eMMC 分区笔记](r1-emmc-partition-layout.md)。

`p1=uboot` 起始的 2560 字节 FDT 已反编译为 RK3588S EVB4 平台设备树，含根 `compatible`、`model`、设备别名和时钟等硬件节点；在已读取范围内没有 FIT 的 `images` 或 `configurations`。因此它**不是** `p1` 的 FIT 元数据。对整个 `p1` 做 `strings` 仍可发现 `FIT Image with ATF/OP-TEE/U-Boot/MCU` 和 `U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1`，但这些文本所属对象与偏移尚未定位。不能据此断言 `p1` 中 FIT 的具体结构、各组件偏移、大小或实际验证策略。

后续 Debug UART 启动日志已直接显示：SPL 尝试其命名为 `MMC1` 的设备，在 `0x4000 sector` 读取一个早期 FIT，并对 `atf-*`、`uboot`、`fdt`、`optee` 分别报告 SHA-256 `OK`，随后经 ARM Trusted Firmware 跳转到 U-Boot。这个**早期固件 FIT**包含的组件与 `p3` 内用于启动 Linux 的 FIT 不应混为一谈。当前尚未把 SPL 的 MMC 编号与扇区位置精确映射到 Linux 块设备或分区；也未从 U-Boot 后续日志直接确认它怎样选择 `p3` 的 Linux FIT。

现在已补充第二阶段 U-Boot 的后续日志：它在 Android 路径失败后，以 `conf` 配置加载 Linux FIT 的 `kernel` 和 `fdt`，并分别报告 SHA-256 `OK`。这两个载荷的大小和哈希分别与 `p3` FIT 中已读取的 `kernel`、`fdt` 完全一致；以 FIT RAM 基址计算，FDT 的相对偏移为 `0x800`、kernel 的相对偏移为 `0x24a00`，也完全一致。由此可将当前 Linux 的 kernel/FDT 载荷与 `p3` FIT 直接关联；日志未直印物理分区名，故不把“物理读取必为 `mmcblk0p3`”写成已验证事实。U-Boot 显示的 FIT RAM 长度止于 `resource` 的起始偏移，当前日志也未显示该资源的 Linux 启动加载过程。

学习者已在 `Hit key to stop autoboot('CTRL+C')` 阶段进入 `=>` 提示符，并以只读 `version` 确认第二阶段 U-Boot 为 `2017.09-g33a7c066a8-dirty #youyeetoo1`（2024-09-29 构建标识），构建工具链为 AArch64 GNU Toolchain 10.3.1 / GNU ld 2.36.1。此交互入口可用于后续只读观察环境变量和启动脚本；在未理解影响前，不执行 `setenv`、`saveenv`、存储写入或擦除命令。

本板 `bootcmd` 为 `boot_android ${devtype} ${devnum};boot_fit;bootrkp;run distro_bootcmd;`。它解释了日志中先出现 Android 失败、再成功启动 FIT 的顺序。这里的分号是顺序执行；成功启动 Linux 后不会返回 U-Boot，所以本次未到达 `bootrkp` 与 `distro_bootcmd`。尚需读取 `boot_android`、`boot_fit` 及变量值，才能判断它们具体从哪个设备和偏移读取数据。

`printenv` 显示 `devtype=mmc`、`devnum=0`，但将 `boot_android`、`boot_fit` 报为未定义。这里“未定义”只针对环境变量表：日志已证明它们能作为 U-Boot 命令运行。应先展开变量、再由命令解析器查找内建命令；要了解命令接口用 `help boot_android`、`help boot_fit`，而不是继续用 `printenv`。

本板 `help boot_android` 说明该命令读取 BCB 以选择 Android 的普通、恢复或 bootloader 模式；其帮助中普通/恢复模式会从相应 `boot` 分区加载内核。`help boot_fit` 说明它可从内存或 `boot`/`recovery` 分区启动 FIT。这是解释当前 Android 尝试失败后转向 FIT 的命令级证据。帮助展示的 `boot_android` 形参数量与当前 `bootcmd` 中的调用不完全相同；实际默认值待确认，不能从帮助文本倒推。

## 关联问题

本知识点解释了 `file -s /dev/mmcblk0p3` 为什么报告 1536 字节 DTB，而运行时 `/sys/firmware/fdt` 却是 151552 字节：前者是 FIT 元数据树，后者是内核收到的硬件描述树。R1 FIT 中的 `fdt` 载荷大小为 147826 字节，也不同于运行时 FDT；两者的转换或来源待继续排查。

## 易错点

- 把所有 FDT 二进制都当成板级设备树。
- 把 FIT 元数据大小当作整个启动镜像大小。
- 仅凭 `strings` 猜测节点关系、载荷偏移或签名是否验证成功。
- 看到 `sha256,rsa2048` 就断言系统已启用并强制执行安全启动；当前只确认签名元数据存在。

## 总结

1. FIT 是基于 FDT 的 U-Boot 启动镜像容器描述。
2. `images` 描述载荷，`configurations` 选择启动组合。
3. FIT 中名为 `fdt` 的载荷通常是板级设备树；R1 厂商路径还会从 resource 的 `rk-kernel.dtb` 取得最终树。
4. 位置、大小、哈希和签名必须读取结构化属性后再判断。

## 参考资料

- U-Boot Project, [Flat Image Tree (FIT)](https://docs.u-boot.org/en/v2025.01/usage/fit/index.html)，访问日期 2026-08-09。
- U-Boot Project, [Flattened Image Tree (FIT) Format](https://docs.u-boot.org/en/v2023.10/usage/fit/source_file_format.html)，访问日期 2026-08-09。
- U-Boot Project, [How to use images in the new image format](https://docs.u-boot.org/en/latest/usage/fit/howto.html)，访问日期 2026-08-09。
