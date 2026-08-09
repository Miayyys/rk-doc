---
title: "设备树的 model 与 compatible"
type: note
status: verified
created: 2026-08-07
updated: 2026-08-07
tags: [rk3588, devicetree, boot, kernel]
aliases: ["DT", "DTB", "FDT", "设备树模型", "设备树兼容列表"]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[environment/software]]"
---

# 设备树的 `model` 与 `compatible`

## 学习目标

能够区分设备树根节点中供人阅读的 `model` 与供内核匹配的 `compatible`，并知道它们不能单独证明实物 PCB 型号。

## 前置知识

- 设备树（DT）是启动时交给内核的硬件描述数据；其二进制形式通常称为 DTB 或 FDT。
- 当前板端系统经 Debug UART 可交互，见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 核心概念

- `model` 是面向人类的设备/板级描述字符串，适合显示和人工核对。
- `compatible` 是一个按“最具体 → 最通用”排序的字符串列表。内核用它进行平台识别，并让驱动或通用代码匹配适用的硬件描述。
- 设备树描述的是**当前加载的 DTB**。若厂商镜像复用参考板 DTB，`model` 和 `compatible` 可能使用参考板名称，而不是销售商品名或 PCB 丝印。

## 工作流程

```text
Bootloader 传递 DTB → 内核读取根节点属性
                         ├─ model：供人阅读
                         └─ compatible：具体板级标识 → SoC 通用回退
```

在本板的当前系统中，根节点属性为：

```text
model: Rockchip RK3588S EVB4 LP4X V10 Board
compatible:
  rockchip,rk3588s-evb4-lp4x-v10
  rockchip,rk3588
```

因此可以确认当前 DTB 以 RK3588S EVB4 LP4X V10 为最具体描述，并声明兼容通用 RK3588。不能仅据此断定物理 R1 是或不是 Rockchip EVB4；需要额外核对 PCB 丝印、厂商资料和实际 DTB 来源。

## 实际验证

学习者在 R1 的 Root Shell 执行：

```sh
tr -d '\0' < /proc/device-tree/model; printf '\n'
tr '\0' '\n' < /proc/device-tree/compatible
```

原始输出与执行上下文见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。`\0` 是设备树字符串的分隔/结尾字符；将它转换为换行可显示 `compatible` 中的多个字符串。

## 运行时 FDT 与存储中的 DTB

当前内核还通过 `/sys/firmware/fdt` 导出一份 root 只读、大小为 151552 字节的 FDT 二进制。它是研究**本次启动实际使用的设备树**的合适起点。

它不自动等同于 eMMC 某个路径下的 `.dtb` 文件：启动加载器可能选择、拼接或修改 DTB 后再传给内核。`ls -l` 显示的文件时间字段也不能在未验证前当作 DTB 构建时间。要追溯 DTB 来源，仍需要后续检查启动配置和 eMMC 分区内容。

`file /sys/firmware/fdt` 读取到：

```text
Device Tree Blob version 17
size=151552
boot CPU=0
string block size=7470
DT structure block size=141980
```

版本 17 是 FDT 的二进制格式版本，不描述设备树内容或板卡版本。DTB 由头部、内存保留区、结构区和字符串区等组成；此处的结构区保存节点和属性编码，字符串区保存属性名称。总大小与文件大小一致，说明 `file` 解析的是这份运行时 FDT 本身。

当前板端 Shell 的 `command -v dtc` 没有输出，表明 `dtc` 不在当前 `PATH` 中。为保持板端环境基线稳定，应先在 Arch 主机确认或准备该工具，再决定是否需要复制运行时 FDT 进行反编译。

主机侧最初也没有可调用的 `dtc`，但随后 `dtc -v` 返回 `DTC v1.8.1`。`pacman -Qi dtc` 进一步确认已安装包为 `1:1.8.1-1`，于 2026-08-07T20:53:39+08:00 单独指定安装，且有数字签名验证。已验证范围与未验证操作见[dtc 工具记录](../tool/dtc.md)。

## 关联问题

这解释了“实物自称 R1，但运行时设备树没有 R1 名称”的现象：目前证据支持“当前 DTB 使用参考板命名”，但其来源仍待确认。

## 易错点

- 将 `model` 当作 PCB 丝印或电路层面的硬件探测结果。
- 认为 `compatible` 中的通用 `rockchip,rk3588` 足以说明全部外设都与所有 RK3588 板相同。
- 只看第一行而忽略 `compatible` 是按具体到通用排列的列表。

## 总结

1. `model` 便于人读，`compatible` 服务于内核匹配。
2. `compatible` 的首项通常最具体，后续项提供更通用的兼容回退。
3. 当前 DTB 声明 RK3588S EVB4 LP4X V10，并回退兼容 RK3588。
4. `/sys/firmware/fdt` 允许从内核侧研究本次启动使用的 FDT，但不自动定位其存储来源。
5. FDT version 17 表示二进制布局版本，不能当作内核、板卡或 DTB 内容版本。
6. 当前板端没有可直接调用的 `dtc`，反编译实验应优先在 Arch 主机准备。
7. 主机已可运行 DTC v1.8.1，适合作为后续 DTB 反编译实验环境。
8. 设备树命名与实物商品名不一致时，应继续追查 DTB 来源，而非立即否定板卡身份。

## 参考资料

- Linux Kernel Documentation, “Linux and the Devicetree”, https://docs.kernel.org/devicetree/usage-model.html，访问于 2026-08-07。
- Devicetree Specification, “Flattened Devicetree (DTB) Format”, https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html，访问于 2026-08-07。
- Arch Linux, “dtc package”, https://archlinux.org/packages/extra/x86_64/dtc/，访问于 2026-08-07。
