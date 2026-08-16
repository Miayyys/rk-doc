---
title: "GPIO 控制器、pinctrl 与运行时 gpiochip"
type: note
status: verified
created: 2026-08-12
updated: 2026-08-12
tags: [rk3588, r1, linux, devicetree, gpio, pinctrl]
aliases: [GPIO bank, gpiochip]
related:
  - "[[experiment/exp-20260812-002-select-first-simple-device]]"
  - "[[experiment/exp-20260812-001-locate-running-kernel-config]]"
  - "[[note/device-tree-model-and-compatible]]"
---

# GPIO 控制器、pinctrl 与运行时 gpiochip

## 学习目标

能区分设备树中的 GPIO 控制器节点、引脚复用控制器（pinctrl）与 Linux 运行时的 `/dev/gpiochipN`，不将它们的编号直接当成扩展排针编号。

## 核心概念

`gpio-controller` 把一个硬件 GPIO bank 声明为可供其他设备树节点引用的 GPIO 提供者；`#gpio-cells = <2>` 表示引用它时需要两个 GPIO 专用参数，具体含义由对应 binding 定义。pinctrl 负责同一物理引脚的复用和电气配置。`gpio-ranges` 把 GPIO 控制器中的线号范围连接到 pinctrl 的引脚编号范围。

Linux GPIO 子系统将已注册控制器导出为 `/dev/gpiochipN`；这里的 `N` 对应 character device 的次设备编号。旧 GPIO sysfs 同时以 `/sys/class/gpio/gpiochipB` 导出控制器，其中 `B` 是该控制器的 legacy GPIO 基址。两个数字来自不同接口：都不是设备树 alias、SoC GPIO bank 号或排针丝印号，不能混用。此内核的 legacy GPIO sysfs 目录不提供 `dev` 属性；应通过通用 sysfs 索引 `/sys/dev/char/<major>:<minor>` 反查 character device 的设备路径，再结合 label 建立映射。

## R1 的实际验证

R1 运行内核的 `/proc/config.gz` 显示 `CONFIG_GPIOLIB=y` 与 `CONFIG_GPIO_ROCKCHIP=y`。前者是 Linux GPIO 核心框架，后者是 Rockchip GPIO 控制器驱动；`=y` 表示均内建于当前内核，而非依赖 `/lib/modules/5.10.110` 下的可加载模块。本板确实没有该 modules 目录，但不能因此概括为“所有驱动都内建”。这两项也不替代引脚复用、占用状态或电气条件检查。

R1 保存的 FIT 内 Linux DTS 在 `aliases` 中声明 `gpio0` 至 `gpio4`，分别指向 `fd8a0000`、`fec20000`、`fec30000`、`fec40000`、`fec50000` 五个 `rockchip,gpio-bank` 节点。每个节点声明 `gpio-controller`、`#gpio-cells = <2>` 和 32 条线的 `gpio-ranges`；其 pinctrl 范围起点依次为 0、32、64、96、128。

运行内核实际导出 `/dev/gpiochip0` 至 `/dev/gpiochip5` 共六个 GPIO character devices。旧 GPIO sysfs 显示五个 SoC bank 的 base 分别为 0、32、64、96、128，label 为 `gpio0` 至 `gpio4`，设备路径逐一匹配上述 DTS 地址；剩余控制器的 base 为 509、label 为 `rk806-gpio`、有 3 条线，设备路径位于 `spi2.0/rk806-pinctrl.0.auto`。因此第六个控制器来自 RK806 PMIC 的 GPIO 功能，而不是遗漏的 RK3588 GPIO bank。

通过 `/sys/dev/char/254:<minor>` 已验证精确对应：次设备号 0–4 依次为 `gpio0`–`gpio4`，次设备号 5 为 `rk806-gpio`。这条顺序是本次启动、此内核版本的运行时事实；不应把它当作所有内核版本或所有 RK3588 板卡的固定规则。

## 易错点

- `/dev/gpiochip0` 不等于 `/sys/class/gpio/gpiochip0`：前者末尾是 character device 次设备号，后者末尾是 legacy GPIO base。二者也不等于 DTS `gpio0` alias 或扩展排针的第 0 脚。
- `gpio-ranges` 说明 GPIO 与 pinctrl 的内部映射，不提供 R1 扩展排针位置或外设接线。
- 看到 `gpio-controller` 只说明控制器存在；在不了解该线的复用、占用、电气电平和外接电路前，不应请求或驱动它。

## 总结

- DTS 已确认 R1 有 5 个 `rockchip,gpio-bank` 主节点。
- 运行内核已确认内建 GPIO 核心和 Rockchip GPIO 控制器驱动。
- 每个已见 bank 声明 32 条 GPIO 线，并连接到 pinctrl 的连续引脚范围。
- 五个 SoC bank 与 DTS 地址已经由 label/设备路径逐一映射；第六个是 RK806 PMIC 的 3 条 GPIO。
- character device 次设备号与 legacy GPIO base 是两套接口编号；本板当前映射已核对，但换内核后应重新验证。
- 选择外接 LED 或按键前，必须先完成控制器映射并核对 R1 V2 排针资料。

## 参考资料

- 本机验证：[EXP-20260812-002](../experiment/exp-20260812-002-select-first-simple-device.md)、[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。
