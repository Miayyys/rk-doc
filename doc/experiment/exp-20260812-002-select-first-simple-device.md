---
title: "EXP-20260812-002 筛选第一个简单设备驱动实验候选"
type: experiment
status: verified
created: 2026-08-12
updated: 2026-08-12
tags: [rk3588, r1, linux, devicetree, gpio, led]
related:
  - "[[experiment/exp-20260812-001-locate-running-kernel-config]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[note/gpio-controller-and-pinctrl]]"
  - "[[status/current]]"
---

# EXP-20260812-002 筛选第一个简单设备驱动实验候选

## 目标

从 R1 的已保存 FIT 内 Linux DTS 中寻找低风险、可观察的首个简单设备驱动实验候选。第一候选为 GPIO LED；本实验只回答该 DTS 是否包含常见 GPIO LED 绑定，不操作任何 GPIO。

## 环境与前置条件

- 执行端：Arch 主机 fish Shell。
- 输入：`build/local/r1-20260812/linux-fit-fdt.dts`，由 p3 FIT 内 Linux FDT 反编译而来；其来源与 SHA-256 见[EXP-20260812-001](exp-20260812-001-locate-running-kernel-config.md)。
- 操作前状态：尚未确认该 DTS 是否包含可作为实验对象的 GPIO LED。

## 风险与恢复

- 影响范围：仅搜索主机上的 DTS 文本。
- 备份：不需要。
- 恢复方法：不修改文件、开发板或 GPIO，无恢复操作。

## 步骤与证据

### 步骤 1：搜索常见 GPIO LED 绑定

目的：筛选 `gpio-leds` 父节点、`gpio-led` compatible 或名为 `leds` 的节点。预期可能匹配到相关文本，也可能无结果；无结果只否定这些搜索模式，不否定板上存在 LED。

```fish
set artifact_dir /home/loser/Study/rk3588/build/local/r1-20260812
rg -n -i -C 4 'gpio-leds|compatible = "gpio-led"|leds \{' $artifact_dir/linux-fit-fdt.dts
printf $status
```

实际输出（学习者提供）：前一条 `rg` 无文本输出；`printf $status` 输出：

```text
1
```

观察：`rg` 的退出状态 1 表示没有匹配项。因此，此 FIT 内 Linux DTS 中未找到本次搜索覆盖的常见 GPIO LED 绑定。不能由此得出“R1 没有板载 LED”或“运行中内核没有 LED 类设备”：LED 可能由其他设备树绑定、运行时补充、固件或未被本模式命中的节点提供，仍需运行时检查。

若存在标准 `gpio-leds` 绑定，子 LED 节点通常还需 `gpios` 属性来指明 GPIO 控制器、引脚和标志（含有效电平）；`label`、`default-state`、`linux,default-trigger` 等则分别有助于命名和定义初始/触发行为。这里未发现节点，不能据此推断本板的 GPIO 编号或极性。

### 步骤 2：检查运行时 LED 类设备

目的：验证未发现常见 GPIO LED DTS 节点后，运行内核是否仍注册 LED 类设备。预期可能为空、包含符号链接或没有此目录；不同结果决定是否继续追踪该类设备。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs。

```sh
ls -l /sys/class/leds
```

实际输出（学习者提供）：

```text
total 0
lrwxrwxrwx 1 root root 0 Nov 22 04:57 mmc0:: -> ../../devices/platform/fe2e0000.mmc/leds/mmc0::
```

观察：运行内核注册了一个名为 `mmc0::` 的 LED 类对象，它关联到平台 MMC 控制器 `fe2e0000.mmc`。这不是 `gpio-leds` 的证据，也还不能证明它对应板上哪一颗物理 LED；它可能是 MMC 活动指示机制。学习者提供板上有两颗物理 LED，但两颗灯的功能、供电/控制路径和是否由 Linux 注册仍未知，不能把其中任意一颗直接当作此 sysfs 对象。

### 步骤 3：尝试读取通用 LED trigger 属性

目的：确认该 LED 类对象是否暴露通用 LED trigger 接口。预期若支持，该文件列出可选 trigger；若不存在，则先检查对象实际暴露的属性而不假定实现。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs。

```sh
cat /sys/class/leds/mmc0::/trigger
```

实际输出（学习者提供）：

```text
cat: '/sys/class/leds/mmc0::/trigger': No such file or directory
```

观察：此 `mmc0::` 对象不在该路径暴露 `trigger` 属性，因此不能用“方括号中的当前 trigger”解释其行为。`/sys` 是内核实时导出的虚拟文件系统；`ls -l` 首行的 `total 0` 表示目录占用块数统计为 0，不表示其中没有符号链接或设备对象。下一步必须先列出该对象实际导出的属性，不能写入 `brightness`、`trigger` 或其他属性。

### 步骤 4：列出 MMC LED 对象实际导出的属性

目的：确认该对象是否实现 LED class 的亮度接口，以及还提供哪些关联入口。预期可能有 `brightness`、`max_brightness`、`device`、`power`、`uevent` 等；文件名和权限只能说明接口存在，不单独证明会改变哪一颗物理 LED。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs。

```sh
ls -la /sys/class/leds/mmc0::/
```

实际输出（学习者提供）：

```text
total 0
drwxr-xr-x 3 root root    0 Nov 22 04:57 .
drwxr-xr-x 3 root root    0 Nov 22 04:57 ..
-rw-r--r-- 1 root root 4096 Nov 22 06:18 brightness
lrwxrwxrwx 1 root root    0 Nov 22 06:18 device -> ../../../fe2e0000.mmc
-r--r--r-- 1 root root 4096 Nov 22 06:18 max_brightness
drwxr-xr-x 2 root root    0 Nov 22 06:18 power
lrwxrwxrwx 1 root root    0 Nov 22 04:57 subsystem -> ../../../../../class/leds
-rw-r--r-- 1 root root 4096 Nov 22 04:57 uevent
```

观察：对象属于 LED class（`subsystem` 指向 `/sys/class/leds`），并实现标准 `brightness` 与 `max_brightness` 属性；`brightness` 对 root 可写。这表明内核允许向此 LED class 对象请求一个逻辑亮度值，但尚不知道允许的数值范围、当前值、实际驱动回调效果或对应哪颗物理 LED。应先只读 `brightness` 和 `max_brightness`，再决定是否做一个可恢复的单次写入实验。

### 步骤 5：读取亮度值域

目的：确定当前逻辑亮度及该 LED class 对象接受的最大值，为后续可恢复的最小写入实验建立原始值和合法范围。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs。

```sh
cat /sys/class/leds/mmc0::/brightness
cat /sys/class/leds/mmc0::/max_brightness
```

实际输出（学习者提供）：

```text
0
255
```

观察：当前逻辑亮度为 0，最大值为 255。因而在此内核接口中，0 是当前可恢复原值，255 是允许范围内的最大亮度请求。该值域不说明硬件一定支持 256 级模拟调光，也不证明 255 会让某颗实体 LED 可见；它只定义这个 LED class 接口接受的逻辑范围。

### 步骤 6：受控写入最大逻辑亮度

目的：验证内核是否接受对该 MMC LED class 对象的亮度写入，并观察其是否对应可见实体 LED。操作前已记录原值 0；只写入已确认允许的最大值 255。

执行端：R1 目标 Linux 的 root Shell。影响范围仅为该 LED class 对象的逻辑状态；不读写 eMMC 数据、不改变网络或供电。恢复值为 0。

```sh
printf '255\n' > /sys/class/leds/mmc0::/brightness
cat /sys/class/leds/mmc0::/brightness
```

实际输出与观察（学习者提供）：

```text
255
```

学习者观察两颗实体 LED 均无可见变化。

观察：内核接受了 255 的亮度请求并读回同一值，说明该 LED class 逻辑状态确实改变。一次肉眼观察未见两颗实体 LED 变化，否定了“此对象显然直接控制其中一颗可见板载 LED”的假设；但不能仅据此断定没有任何硬件作用，例如可能无实体 LED 接出、观察条件不足，或接口用于非可见逻辑状态。当前逻辑状态仍为 255，必须先恢复到实验前记录的 0。

### 步骤 7：恢复实验前的逻辑亮度

目的：将受控实验改动恢复为开始前记录的逻辑值 0。

执行端：R1 目标 Linux 的 root Shell。

```sh
printf '0\n' > /sys/class/leds/mmc0::/brightness
cat /sys/class/leds/mmc0::/brightness
```

实际结果（学习者提供）：已恢复。原始读回输出未保留。

观察：学习者确认恢复操作完成，但没有提供恢复后的原始读回行。因此“已恢复”保留为用户提供信息，不能替代下一次实际输出的证据；本次未报告 eMMC、网络或启动状态受到影响。

### 步骤 8：确认 GPIO character-device 控制器与查看工具

目的：在不申请 GPIO 线的前提下，确认内核是否已导出 GPIO character-device 接口，并检查是否已有 `gpioinfo` 用于下一步只读查看控制器、线路名称和占用状态。

执行端：R1 目标 Linux 的 root Shell；仅读取 `/dev` 目录及 `PATH`，不请求 GPIO 线、不写 GPIO 或改变电平。

```sh
ls -l /dev/gpiochip*
command -v gpioinfo
```

实际输出（学习者提供）：

```text
crw------- 1 root root 254, 0 Nov 22 04:57 /dev/gpiochip0
crw------- 1 root root 254, 1 Nov 22 04:57 /dev/gpiochip1
crw------- 1 root root 254, 2 Nov 22 04:57 /dev/gpiochip2
crw------- 1 root root 254, 3 Nov 22 04:57 /dev/gpiochip3
crw------- 1 root root 254, 4 Nov 22 04:57 /dev/gpiochip4
crw------- 1 root root 254, 5 Nov 22 04:57 /dev/gpiochip5
```

`command -v gpioinfo` 没有输出。

观察：运行内核已导出 6 个 GPIO character devices，主设备号均为 254、次设备号为 0–5；它们只能由 root 读写，普通用户不能直接打开。`gpioinfo` 当前未在 root Shell 的 `PATH` 中，可能未安装，也可能位于未包含的目录；目前只能确认“不可直接调用”，不推断其安装状态。设备节点编号本身不等同于某个排针的物理编号，下一步应先从设备树读取各 GPIO 控制器的节点、别名和 `gpio-ranges` 信息，再决定是否需要安装工具或查核板级排针资料。

### 步骤 9：首次主机侧 DTS 搜索的路径前置条件

目的：执行 GPIO 控制器 DTS 搜索。该步骤同时验证命令中的相对路径是否与实际主机工作目录一致。

执行端：Arch 主机 fish Shell；学习者当时位于仓库的 `build/` 目录，而非仓库根目录。命令只尝试读取 DTS 文本，没有访问开发板或修改文件。

```fish
rg -n -C 3 'gpio@[0-9a-f]+|gpio-ranges|gpio[0-9] = ' $artifact_dir/linux-fit-fdt.dts
```

实际输出（学习者提供）：

```text
rg: build/local/r1-20260812/linux-fit-fdt.dts: IO error for operation on build/local/r1-20260812/linux-fit-fdt.dts: No such file or directory (os error 2)
```

观察：此前给出的 `$artifact_dir` 是相对路径 `build/local/r1-20260812`，只能在仓库根目录正确解析。学习者从仓库 `build/` 子目录执行时，该路径实际指向不存在的 `build/build/local/...`。这只是主机路径前置条件不满足，不是 DTS 丢失、GPIO 节点不存在或开发板状态异常。后续使用绝对路径避免工作目录影响。

### 步骤 10：定位 DTS 中的 GPIO 控制器与 pinctrl 范围

目的：从已核对哈希的 FIT 内 Linux DTS 找出 GPIO aliases、GPIO 控制器节点和 `gpio-ranges`，建立设备树侧的控制器清单；不把它们直接等同为运行时 gpiochip 或物理排针。

执行端：Arch 主机 fish Shell。命令只读取保存的 DTS 文本，不访问开发板、不申请 GPIO 线。

```fish
set artifact_dir /home/loser/Study/rk3588/build/local/r1-20260812
rg -n -C 3 'gpio@[0-9a-f]+|gpio-ranges|gpio[0-9] = ' $artifact_dir/linux-fit-fdt.dts
```

实际输出（学习者提供；仅省略与 GPIO 节点无关的上下文行）：

```text
gpio0 = "/pinctrl/gpio@fd8a0000";
gpio1 = "/pinctrl/gpio@fec20000";
gpio2 = "/pinctrl/gpio@fec30000";
gpio3 = "/pinctrl/gpio@fec40000";
gpio4 = "/pinctrl/gpio@fec50000";

gpio@fd8a0000 {
    compatible = "rockchip,gpio-bank";
    reg = <0x00 0xfd8a0000 0x00 0x100>;
    gpio-controller;
    #gpio-cells = <0x02>;
    gpio-ranges = <0x17a 0x00 0x00 0x20>;
};

gpio@fec20000 { gpio-ranges = <0x17a 0x00 0x20 0x20>; };
gpio@fec30000 { gpio-ranges = <0x17a 0x00 0x40 0x20>; };
gpio@fec40000 { gpio-ranges = <0x17a 0x00 0x60 0x20>; };
gpio@fec50000 { gpio-ranges = <0x17a 0x00 0x80 0x20>; };
```

观察：设备树 aliases 明确命名 5 个主 GPIO bank，地址按 `gpio0` 至 `gpio4` 对应。每个节点均为 `rockchip,gpio-bank`、有 `gpio-controller` 和 `#gpio-cells = <2>`，且 `gpio-ranges` 的最后一个单元均为 `0x20`（32）。五个范围在同一 pinctrl phandle `0x17a` 下依次从 0、32、64、96、128 起始。这里的 phandle 是 DTB 内部引用标识，不能当成物理地址。

这与运行时 `/dev/gpiochip0` 至 `/dev/gpiochip5` 的 6 个节点并不一一闭合：已从 DTS 定位 5 个主 bank，但第 6 个运行时控制器仍待识别，也尚未知道运行时分配次序。下一步必须从运行时 sysfs 读取每个 gpiochip 的 label、线数和设备路径，而不是猜测编号。

### 步骤 11：用 sysfs label 和设备路径完成控制器来源映射

目的：读取每个 legacy GPIO sysfs 对象的 label、线数和设备路径，把运行时控制器与 DTS 节点或其他硬件提供者对应起来。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs，不申请 GPIO 线或改变电平。

```sh
for chip in /sys/class/gpio/gpiochip*; do
    printf '%s\n' "== $chip =="
    cat "$chip/label"
    cat "$chip/ngpio"
    readlink -f "$chip/device"
done
```

实际输出（学习者提供）：

```text
== /sys/class/gpio/gpiochip0 ==
gpio0
32
/sys/devices/platform/pinctrl/fd8a0000.gpio
== /sys/class/gpio/gpiochip128 ==
gpio4
32
/sys/devices/platform/pinctrl/fec50000.gpio
== /sys/class/gpio/gpiochip32 ==
gpio1
32
/sys/devices/platform/pinctrl/fec20000.gpio
== /sys/class/gpio/gpiochip509 ==
rk806-gpio
3
/sys/devices/platform/feb20000.spi/spi_master/spi2/spi2.0/rk806-pinctrl.0.auto
== /sys/class/gpio/gpiochip64 ==
gpio2
32
/sys/devices/platform/pinctrl/fec30000.gpio
== /sys/class/gpio/gpiochip96 ==
gpio3
32
/sys/devices/platform/pinctrl/fec40000.gpio
```

观察：`gpio0` 至 `gpio4` 的 label、32 条线及 `fd8a0000`/`fec2...`/`fec3...`/`fec4...`/`fec5...` 设备路径，逐一匹配 DTS 的 5 个 `rockchip,gpio-bank`。第六个控制器的 label 是 `rk806-gpio`，仅有 3 条线，路径位于 SPI2 的 `rk806-pinctrl` 子设备；它是 RK806 PMIC 提供的 GPIO，不是漏掉的 SoC GPIO bank。

同时要区分两组编号：本命令遍历的是 `/sys/class/gpio/gpiochipB`，其中 `B` 是 legacy GPIO base（本板为 0、32、64、96、128、509）；先前 `/dev/gpiochipN` 的 `N` 是 character device 次设备号（0–5）。这次尚未读取 sysfs 的 `dev` 属性，故没有逐项证明哪个 legacy base 对应哪个 `/dev/gpiochipN`；不得仅凭两个名称都带 `gpiochip` 就将数字相等或排序对应。

### 步骤 12：验证 legacy GPIO sysfs 是否含 character-device 映射属性

目的：检查 legacy GPIO sysfs 对象是否直接导出 `dev` 属性，以主/次设备号关联 `/dev/gpiochipN` 与 legacy GPIO base。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs，不申请 GPIO 线或改变电平。

```sh
for chip in /sys/class/gpio/gpiochip*; do
    printf '%s\n' "== $chip =="
    cat "$chip/dev"
    cat "$chip/label"
done
```

实际输出（学习者提供；6 个对象均为同一现象）：

```text
== /sys/class/gpio/gpiochip0 ==
cat: /sys/class/gpio/gpiochip0/dev: No such file or directory
gpio0
...
== /sys/class/gpio/gpiochip509 ==
cat: /sys/class/gpio/gpiochip509/dev: No such file or directory
rk806-gpio
```

观察：6 个 legacy GPIO sysfs 对象均没有 `dev` 属性，故此前“从该目录读取主/次设备号”的假设被否定。这是该内核 GPIO sysfs 接口未导出此属性，不是 GPIO 控制器故障，也不影响已完成的 DTS/label/设备路径映射。改用通用 sysfs character-device 索引 `/sys/dev/char/254:<minor>`；其中 254 来自已验证的 `/dev/gpiochipN` 主设备号。

### 步骤 13：以通用 sysfs character-device 索引完成编号映射

目的：用已确认的 GPIO character-device 主设备号 254，反查次设备号 0–5 的真实设备路径，从而精确关联 `/dev/gpiochipN` 与已识别的控制器。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs 符号链接，不申请 GPIO 线或改变电平。

```sh
for minor in 0 1 2 3 4 5; do
    printf '%s\n' "== /dev/gpiochip$minor (254:$minor) =="
    readlink -f "/sys/dev/char/254:$minor"
done
```

实际输出（学习者提供）：

```text
== /dev/gpiochip0 (254:0) ==
/sys/devices/platform/pinctrl/fd8a0000.gpio/gpiochip0
== /dev/gpiochip1 (254:1) ==
/sys/devices/platform/pinctrl/fec20000.gpio/gpiochip1
== /dev/gpiochip2 (254:2) ==
/sys/devices/platform/pinctrl/fec30000.gpio/gpiochip2
== /dev/gpiochip3 (254:3) ==
/sys/devices/platform/pinctrl/fec40000.gpio/gpiochip3
== /dev/gpiochip4 (254:4) ==
/sys/devices/platform/pinctrl/fec50000.gpio/gpiochip4
== /dev/gpiochip5 (254:5) ==
/sys/devices/platform/feb20000.spi/spi_master/spi2/spi2.0/rk806-pinctrl.0.auto/gpiochip5
```

观察：character-device 次设备号 0–4 按本次启动顺序分别对应 5 个 SoC bank `gpio0`–`gpio4`；次设备号 5 对应 RK806 PMIC GPIO。结合步骤 11 的 legacy bases，当前完整映射为：`/dev/gpiochip0` ↔ `gpio0`/base 0，`gpiochip1` ↔ `gpio1`/base 32，`gpiochip2` ↔ `gpio2`/base 64，`gpiochip3` ↔ `gpio3`/base 96，`gpiochip4` ↔ `gpio4`/base 128，`gpiochip5` ↔ `rk806-gpio`/base 509。映射来自当前运行内核，不应无验证地迁移到其他内核或板卡。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| FIT DTS 中的常见 GPIO LED 绑定 | 可能存在或不存在匹配项 | 无匹配，`rg` 退出状态 1 | GPIO LED 候选未发现 |
| 运行时 LED 类设备 | 可能为空或包含设备 | 存在 `mmc0::`，关联 `fe2e0000.mmc` | MMC LED 对象待解释 |
| 通用 LED trigger 属性 | 可能存在或不存在 | `trigger` 路径不存在 | 不适用/待列出实际属性 |
| LED class 亮度接口 | 可能存在或不存在 | 存在 `brightness`（root 可写）与 `max_brightness`（只读） | 值域与效果待验证 |
| LED class 亮度值域 | 当前值和最大值可读 | 当前 0，最大 255 | 值域已确认，物理效果待验证 |
| 最大亮度写入及恢复 | 内核可能接受或拒绝 255，随后应恢复原值 | 写入读回 255；两颗实体 LED 未见变化；学习者报告已恢复 0 | 逻辑状态改变，物理对应未证实；恢复结果为用户提供 |
| GPIO character-device 与 `gpioinfo` | 可能有多个控制器，工具可能存在或缺失 | `/dev/gpiochip0` 至 `/dev/gpiochip5` 存在且仅 root 可访问；`gpioinfo` 无输出 | 已有 GPIO character-device 接口；当前无可直接调用的查看工具 |
| DTS GPIO 控制器 | 可能有多个 bank 和 aliases | `gpio0`–`gpio4` 指向 5 个 `rockchip,gpio-bank`；每个范围为 32 条线 | DTS 侧 5 个主 bank 已定位 |
| 运行时控制器来源 | 5 个 DTS bank 与第 6 个来源待核对 | 5 个 label/路径逐一匹配 SoC bank；`rk806-gpio` 有 3 条线，位于 SPI2 的 RK806 子设备 | 第 6 个为 RK806 PMIC GPIO |
| legacy sysfs `dev` 属性 | 可能含主/次设备号 | 6 个 gpiochip 均无此文件 | 该映射方法不适用，改查 `/sys/dev/char` |
| character-device 与 legacy base 映射 | 6 个 `/dev/gpiochipN` 应各有设备路径 | 次设备号 0–4 依次对应 SoC `gpio0`–`gpio4`，5 对应 RK806 GPIO | 当前启动下映射已闭合 |

## 结论

本次只读搜索未找到可直接作为首个实验对象的常见 GPIO LED 设备树绑定，因此不应贸然向 GPIO 写值或假定板载 LED 的接线。运行时虽存在一个关联 MMC 控制器的 `mmc0::` LED 类对象，但它不在此路径提供通用 `trigger` 属性。其逻辑亮度范围为 0–255，写入 255 后可读回 255，但学习者观察两颗实体 LED 均无可见变化。因此它不适合作为当前的可见 GPIO LED 实验对象。学习者报告逻辑值已恢复为 0；板载两颗实体 LED 的控制路径仍待确认。

运行内核已导出 `/dev/gpiochip0` 至 `/dev/gpiochip5` 这 6 个 root 专用 GPIO character devices，但 `gpioinfo` 不能直接调用。DTS 的 5 个 `rockchip,gpio-bank` 主节点（`gpio0`–`gpio4`）已由运行时 label 和设备路径逐一确认；每个有 32 条线。额外控制器为 SPI2 上 RK806 PMIC 提供的 `rk806-gpio`，有 3 条线。通过 `/sys/dev/char/254:<minor>`，已完成 character-device 次设备号与 legacy GPIO base 的当前启动映射。

因此，本实验“筛选和理解第一个简单设备的 GPIO 控制器候选”的目标已完成。实际外接 LED、RGB 或传感器实验因 R1 V2 扩展口逐针孔位、候选模块型号和电气条件未确认而暂缓；它们应在硬件资料齐全后另开实验，不应阻塞内核驱动主线的继续学习。

## 关联知识与问题

- 支持的知识点：[GPIO 控制器、pinctrl 与运行时 gpiochip](../note/gpio-controller-and-pinctrl.md)；设备树绑定描述硬件关系；缺少一次文本匹配不是硬件不存在的结论。
- 关联问题：无。

## 后续行动

- [x] 在 R1 上只读列出 `/sys/class/leds`，确认存在关联 `fe2e0000.mmc` 的 `mmc0::` 对象。
- [x] 尝试读取 `mmc0::` 的通用 `trigger` 属性；路径不存在，不能按 LED trigger 模型解释。
- [x] 列出 `mmc0::` 实际导出的属性，确认有可写 `brightness` 和只读 `max_brightness`。
- [x] 读取当前亮度与最大亮度，确认当前值 0、最大值 255。
- [x] 将亮度从当前 255 恢复为实验前的 0；学习者报告已恢复，原始读回未保留。
- [x] 排除 `mmc0::` 作为当前可见 GPIO LED 实验对象。
- [x] 检查运行时 GPIO character-device 节点与 `gpioinfo` 工具可用性：存在 6 个 root 专用 `/dev/gpiochipN`，`gpioinfo` 不可直接调用。
- [x] 用绝对路径在已保存 DTS 中定位 GPIO 控制器节点、别名和 `gpio-ranges`：DTS 有 5 个主 bank，不将其直接视作运行时编号或排针编号。
- [x] 从运行时 sysfs 读取每个 gpiochip 的 label、线数和设备路径：5 个 SoC bank 与 DTS 对应，第 6 个为 RK806 PMIC 的 3 条 GPIO。
- [x] 尝试读取每个 legacy GPIO sysfs gpiochip 的 `dev` 属性；6 个对象均不存在该属性，改用通用 `/sys/dev/char` 索引。
- [x] 从 `/sys/dev/char/254:<minor>` 反查每个 GPIO character device 的设备路径，完成当前启动的次设备号与 legacy base 映射。
- [ ] 在硬件资料齐全后另开实验，核对 R1 V2 扩展排针和模块电气条件，再选择外接 LED、RGB 或传感器对象。
