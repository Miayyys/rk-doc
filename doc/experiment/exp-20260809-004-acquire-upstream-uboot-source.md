---
title: "EXP-20260809-004 获取并验证上游 U-Boot 源码"
type: experiment
status: verified
created: 2026-08-09
updated: 2026-08-11
tags: [rk3588, uboot, source, git]
related:
  - "[[resource/u-boot-v2026-07-upstream-source]]"
  - "[[experiment/exp-20260809-003-check-uboot-host-build-prerequisites]]"
  - "[[tool/aarch64-linux-gnu-gcc]]"
  - "[[status/current]]"
---

# EXP-20260809-004 获取并验证上游 U-Boot 源码

## 目标

取得一个固定版本、可复现的上游 U-Boot 学习基线，并验证其 tag、提交与工作树状态。

## 环境与前置条件

- 执行端：Arch Linux 主机 fish Shell；目录为 `~/Study/rk3588`，随后进入 `src/u-boot-upstream`。
- 前置：U-Boot 基础构建命令已盘点通过，见[EXP-20260809-003](exp-20260809-003-check-uboot-host-build-prerequisites.md)。
- 来源：[U-Boot 官方获取源码文档](https://docs.u-boot.org/en/latest/build/source.html)说明 GitHub 是上游源码镜像；选用固定发布 tag `v2026.07`，不使用滚动 `master`。

## 风险与恢复

- 影响范围：在主机 `src/u-boot-upstream/` 创建第三方 Git 工作树并占用磁盘、访问网络；不访问 R1。
- 备份：不改动 eMMC、板端系统或厂商镜像。
- 恢复方法：如需重新获取，可删除该特定主机工作目录后重新克隆；本实验未执行删除操作。

## 步骤与证据

### 步骤 1：克隆固定发布标签

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588`。

```fish
mkdir -p src
git clone --depth 1 --branch v2026.07 https://github.com/u-boot/u-boot.git src/u-boot-upstream
```

实际结果（学习者提供）：已完成克隆；完整传输输出和退出码未保存。

观察：`--depth 1` 取得单提交浅克隆，`--branch v2026.07` 选择固定发布标签。克隆完成本身尚不足以证明实际检出身份，下一步读取 Git 状态、精确 tag 和 HEAD。

### 步骤 2：验证检出身份

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

```fish
git status --short --branch
git describe --tags --exact-match
git rev-parse HEAD
```

实际输出（学习者提供）：

```text
## HEAD（非分支）
v2026.07
ece349ade2973e220f524ce59e59711cc919263f
```

观察：检出 tag 时 `HEAD（非分支）` 是正常 detached HEAD 状态；短状态没有列出文件变更。精确 tag 与提交 ID 共同确认本地工作树固定在 `v2026.07` 对应提交。未验证 tag 签名，也未验证它适配 R1。

### 步骤 3：列出顶层关键源码目录

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

```fish
ls -d arch board boot cmd common configs doc drivers include
```

实际输出（学习者提供）：

```text
arch   boot  common   doc      include
board  cmd   configs  drivers
```

观察：学习者已在固定版本源码中直接确认这些顶层目录存在。它们将成为接下来的阅读入口：`configs/` 用于查找构建配置候选，`arch/`、`board/` 和 `drivers/` 分别承载架构、板级和驱动相关代码；`cmd/` 对应 U-Boot 交互命令实现。目录存在不表示其中已有 R1 的上游适配，具体候选配置仍待筛选。

### 步骤 4：筛选上游 RK3588 defconfig 候选

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

```fish
find configs/ -maxdepth 1 -type f -name '*rk3588*' -printf '%f\n' | sort
```

实际输出（学习者提供）：

```text
cm3588-nas-rk3588_defconfig
coolpi-4b-rk3588s_defconfig
coolpi-cm5-evb-rk3588_defconfig
coolpi-cm5-genbook-rk3588_defconfig
evb-rk3588_defconfig
gameforce-ace-rk3588s_defconfig
generic-rk3588_defconfig
jaguar-rk3588_defconfig
khadas-edge2-rk3588s_defconfig
mnt-reform2-rk3588_defconfig
nanopc-t6-rk3588_defconfig
nanopi-r6c-rk3588s_defconfig
nanopi-r6s-rk3588s_defconfig
neu6a-io-rk3588_defconfig
neu6b-io-rk3588_defconfig
nova-rk3588s_defconfig
odroid-m2-rk3588s_defconfig
orangepi-5-max-rk3588_defconfig
orangepi-5-plus-rk3588_defconfig
orangepi-5-rk3588s_defconfig
orangepi-5-ultra-rk3588_defconfig
quartzpro64-rk3588_defconfig
rock5a-rk3588s_defconfig
rock5b-rk3588_defconfig
rock-5c-rk3588s_defconfig
rock-5-itx-rk3588_defconfig
sige7-rk3588_defconfig
tiger-rk3588_defconfig
toybrick-rk3588_defconfig
turing-rk1-rk3588_defconfig
```

观察：上游 v2026.07 中有多份 RK3588/RK3588S defconfig，反映不同板子的 RAM、PMIC、存储、PHY、引脚和启动介质差异。列表中没有以 `r1` 或 `youyeetoo` 命名的候选；这不是“不支持”的证明，但没有证据允许把 `evb-rk3588_defconfig`、`generic-rk3588_defconfig` 或任何其他候选当作 R1 配置。当前阶段仅完成候选发现，不选择、不构建、不烧录。

### 步骤 5：阅读 EVB RK3588 候选 defconfig

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：从配置中识别编译目标、默认设备树、调试串口和 FIT/SPL 相关能力；不把候选配置当成 R1 适配结论。

```fish
head -n 160 configs/evb-rk3588_defconfig
```

实际输出（学习者提供，完整输出在 160 行范围内）：

```text
CONFIG_ARM=y
CONFIG_SKIP_LOWLEVEL_INIT=y
CONFIG_COUNTER_FREQUENCY=24000000
CONFIG_ARCH_ROCKCHIP=y
CONFIG_DEFAULT_DEVICE_TREE="rockchip/rk3588-evb1-v10"
CONFIG_ROCKCHIP_RK3588=y
CONFIG_SPL_SERIAL=y
CONFIG_TARGET_EVB_RK3588=y
CONFIG_SYS_LOAD_ADDR=0xc00800
CONFIG_DEBUG_UART_BASE=0xFEB50000
CONFIG_DEBUG_UART_CLOCK=24000000
CONFIG_DEBUG_UART=y
CONFIG_FIT=y
CONFIG_FIT_VERBOSE=y
CONFIG_SPL_FIT_SIGNATURE=y
CONFIG_SPL_LOAD_FIT=y
CONFIG_LEGACY_IMAGE_FORMAT=y
CONFIG_DEFAULT_FDT_FILE="rockchip/rk3588-evb1-v10.dtb"
# CONFIG_DISPLAY_CPUINFO is not set
CONFIG_DISPLAY_BOARDINFO_LATE=y
CONFIG_SPL_MAX_SIZE=0x40000
# CONFIG_SPL_RAW_IMAGE_SUPPORT is not set
CONFIG_SPL_ATF=y
CONFIG_CMD_GPIO=y
CONFIG_CMD_GPT=y
CONFIG_CMD_MMC=y
CONFIG_CMD_USB=y
# CONFIG_CMD_SETEXPR is not set
CONFIG_CMD_REGULATOR=y
# CONFIG_SPL_DOS_PARTITION is not set
CONFIG_SPL_OF_CONTROL=y
CONFIG_OF_LIVE=y
CONFIG_OF_SPL_REMOVE_PROPS="clock-names interrupt-parent assigned-clocks assigned-clock-rates assigned-clock-parents"
CONFIG_SPL_DM_SEQ_ALIAS=y
CONFIG_SPL_SYSCON=y
CONFIG_SPL_CLK=y
CONFIG_ROCKCHIP_GPIO=y
CONFIG_SYS_I2C_ROCKCHIP=y
CONFIG_MISC=y
CONFIG_SUPPORT_EMMC_RPMB=y
CONFIG_MMC_DW=y
CONFIG_MMC_DW_ROCKCHIP=y
CONFIG_MMC_SDHCI=y
CONFIG_MMC_SDHCI_SDMA=y
CONFIG_MMC_SDHCI_ROCKCHIP=y
CONFIG_DWC_ETH_QOS=y
CONFIG_DWC_ETH_QOS_ROCKCHIP=y
CONFIG_PHY_REALTEK=y
CONFIG_PHY_ROCKCHIP_INNO_USB2=y
CONFIG_PHY_ROCKCHIP_NANENG_COMBOPHY=y
CONFIG_PHY_ROCKCHIP_USBDP=y
CONFIG_SPL_PINCTRL=y
CONFIG_PWM_ROCKCHIP=y
CONFIG_SPL_RAM=y
CONFIG_BAUDRATE=1500000
CONFIG_DEBUG_UART_SHIFT=2
CONFIG_SYS_NS16550_MEM32=y
CONFIG_SYSRESET=y
CONFIG_USB=y
CONFIG_USB_XHCI_HCD=y
CONFIG_USB_EHCI_HCD=y
CONFIG_USB_EHCI_GENERIC=y
CONFIG_USB_OHCI_HCD=y
CONFIG_USB_OHCI_GENERIC=y
CONFIG_USB_DWC3=y
CONFIG_USB_DWC3_GENERIC=y
CONFIG_ERRNO_STR=y
```

观察：

- `CONFIG_TARGET_EVB_RK3588=y`、`CONFIG_ARCH_ROCKCHIP=y` 与 `CONFIG_ROCKCHIP_RK3588=y` 表示这是上游 RK3588 EVB 构建目标。
- 默认设备树源码名是 `rockchip/rk3588-evb1-v10`，默认 DTB 文件名为 `rockchip/rk3588-evb1-v10.dtb`；它与 R1 运行时树的 `rk3588s-evb4-lp4x-v10` 不同，因此没有证据表明该配置可直接用于 R1。
- `CONFIG_DEBUG_UART_BASE=0xFEB50000` 与 `CONFIG_BAUDRATE=1500000` 分别匹配本板已观察到的 U-Boot Debug UART 基址和串口波特率；该一致性只说明早期串口设置相似，不证明板级电源、内存、设备树或镜像布局兼容。
- `CONFIG_FIT`、`CONFIG_SPL_LOAD_FIT`、`CONFIG_SPL_FIT_SIGNATURE`、`CONFIG_SPL_ATF` 说明此上游候选具备 FIT/SPL/ATF 相关代码路径或能力开关。它不能单独证明当前厂商 U-Boot 使用同一配置；厂商 U-Boot 版本为 2017.09，而此源码基线为 v2026.07。

### 步骤 6：检查默认 DTS 的预期旧路径

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：验证 `CONFIG_DEFAULT_DEVICE_TREE` 是否直接对应 `arch/arm/dts/` 下的完整 DTS 文件。该假设来自旧版常见源码布局，必须以当前 v2026.07 的实际文件树验证。

```fish
ls arch/arm/dts/ | grep rk3588-evb1
```

实际输出（学习者提供；退出码未记录）：

```text
rk3588-evb1-v10-u-boot.dtsi
```

观察：预期的 `arch/arm/dts/rk3588-evb1-v10.dts` 没有在该筛选输出中出现，只有 `rk3588-evb1-v10-u-boot.dtsi`。文件名表明它**可能**是 U-Boot 专用的设备树补充片段，但完整 DTS 的实际位置与最终合并方式尚未验证。此前“直接从 `arch/arm/dts/rk3588-evb1-v10.dts` 阅读”的路径假设被当前证据否定。

### 步骤 7：定位完整 DTS 与 U-Boot 专用片段

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：在整个固定源码树中定位 EVB1 完整 DTS 与同名 U-Boot `.dtsi`，验证当前版本的文件布局。

```fish
rg --files -g 'rk3588-evb1-v10.dts' -g 'rk3588-evb1-v10-u-boot.dtsi'
```

实际输出（学习者提供）：

```text
dts/upstream/src/arm64/rockchip/rk3588-evb1-v10.dts
arch/arm/dts/rk3588-evb1-v10-u-boot.dtsi
```

观察：完整 DTS 位于 `dts/upstream/src/arm64/rockchip/`，U-Boot 专用 `.dtsi` 位于 `arch/arm/dts/`。这确认当前上游树将两类来源分开放置；两者在构建时如何关联或合并尚未由此命令验证。

### 步骤 8：阅读基础 EVB1 DTS 的开头

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：识别完整板级 DTS 对通用 RK3588 描述的继承关系，以及本文件添加的板级属性。

```fish
sed -n '1,120p' dts/upstream/src/arm64/rockchip/rk3588-evb1-v10.dts
```

实际输出（学习者提供；以下为与继承、根节点和串口直接相关的摘录，省略中间的 ADC 按键、音频、放大器等板级节点；显示内容在 `backlight` 节点开始处结束）：

```dts
/dts-v1/;

#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/input/input.h>
#include <dt-bindings/pinctrl/rockchip.h>
#include <dt-bindings/soc/rockchip,vop2.h>
#include <dt-bindings/usb/pd.h>
#include "rk3588.dtsi"

/ {
	model = "Rockchip RK3588 EVB1 V10 Board";
	compatible = "rockchip,rk3588-evb1-v10", "rockchip,rk3588";

	aliases {
		ethernet0 = &gmac0;
		mmc0 = &sdhci;
	};

	chosen {
		stdout-path = "serial2:1500000n8";
	};

	/* 省略：adc-keys、analog-sound、放大器等 EVB1 板级节点 */
};
```

观察：

- `rk3588.dtsi` 是该板级 DTS 引用的通用 SoC 描述；`dt-bindings` 头文件提供 GPIO、输入键值、引脚和 USB 等属性使用的常量定义。
- 此 `.dts` 在根节点声明 EVB1 的 `model` 与 `compatible`，并增加 EVB1 的别名、按键、音频等板级连线。它与 R1 运行时的 `rk3588s-evb4-lp4x-v10` 标识不同。
- `chosen/stdout-path` 选择 `serial2:1500000n8`。它与 R1 已观察到的 serial2/1,500,000 baud 串口事实相似，但本次阅读不证明 R1 使用此 EVB1 DTS，也不证明 U-Boot 必然读取这一属性。
- `ethernet0 = &gmac0`、`mmc0 = &sdhci` 是别名到节点引用；节点的实际定义与属性需要继续沿包含链或引用查看。

### 步骤 9：阅读 EVB1 的 U-Boot 专用设备树片段

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：确认该片段是否重复完整板级硬件描述，或只添加 U-Boot 启动阶段的配置。

```fish
head -n 160 arch/arm/dts/rk3588-evb1-v10-u-boot.dtsi
```

实际输出（学习者提供）：

```dts
// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#include "rk3588-u-boot.dtsi"

/ {
	chosen {
		u-boot,spl-boot-order = "same-as-spl", &sdhci;
	};
};
```

观察：该文件没有重复 EVB1 的音频、按键或电源节点；它包含更通用的 `rk3588-u-boot.dtsi`，并给 `/chosen` 增加了 `u-boot,spl-boot-order` 属性。属性位于 U-Boot 专用片段中，表明它用于 U-Boot 的 SPL 启动配置；`"same-as-spl"` 与 `&sdhci` 的精确顺序语义、`&sdhci` 在该候选板上的具体硬件节点，以及此片段如何被基础 DTS 合并，均待从 U-Boot 源码验证。

### 步骤 10：搜索 SPL boot order 的文档与代码引用

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：同时定位属性的说明、Rockchip 解析代码候选和其他板级用例，建立“DTS 属性 → 文档/代码”的阅读入口。

```fish
rg -n 'spl-boot-order'
```

实际输出（学习者提供；以下保留文档、Rockchip 代码和当前 EVB1 用例，省略其余同类板级 `.dtsi` 命中项）：

```text
doc/device-tree-bindings/chosen.txt
75:u-boot,spl-boot-order property
84:be configured with the spl-boot-order property under the /chosen node.
103:        u-boot,spl-boot-order = "same-as-spl", &sdmmc, "/sdhci@fe330000";
110:This property is a companion-property to the u-boot,spl-boot-order and

arch/arm/mach-rockchip/spl-boot-order.c
135:                    "u-boot,spl-boot-order", elem, NULL));
187:         * /chosen/u-boot,spl-boot-order.
202:                        "u-boot,spl-boot-order", elem, NULL));

arch/arm/mach-rockchip/Makefile
11:obj-spl-$(CONFIG_SPL_ROCKCHIP_COMMON_BOARD) += spl.o spl-boot-order.o spl_common.o

arch/arm/dts/rk3588-evb1-v10-u-boot.dtsi
10:        u-boot,spl-boot-order = "same-as-spl", &sdhci;
```

观察：当前源码树有该属性的专门文档；Rockchip 的 `spl-boot-order.c` 含该属性字符串的直接引用，且 Makefile 在 `CONFIG_SPL_ROCKCHIP_COMMON_BOARD` 条件下将 `spl-boot-order.o` 编入 SPL。搜索还列出多个板级 `.dtsi` 用例。属性的完整语法与 `same-as-spl` 精确定义尚未读取，下一步优先阅读本树文档。

### 步骤 11：阅读 SPL boot order 属性规范

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：根据同版本 U-Boot 文档确认 `u-boot,spl-boot-order` 的元素类型、顺序规则和 `same-as-spl` 的定义。

```fish
sed -n '70,120p' doc/device-tree-bindings/chosen.txt
```

实际输出（学习者提供，省略该段之前的标题）：

```text
If the SPL is configured through the device-tree, the boot-order can
be configured with the spl-boot-order property under the /chosen node.
Each list element of the property should specify a device to be probed
in the order they are listed: references (i.e. implicit paths), a full
path or an alias is expected for each entry.

A special specifier "same-as-spl" can be used at any position in the
boot-order to direct U-Boot to insert the device the SPL was booted
from there.  Whether this is indeed inserted or silently ignored (if
it is not supported on any given SoC/board or if the boot-device is
not available to continue booting from) is implementation-defined.
Note that if "same-as-spl" expands to an actual node for a given
board, the corresponding node may appear multiple times in the
boot-order (as there currently exists no mechanism to suppress
duplicates from the list).

Example
-------
/ {
	chosen {
		u-boot,spl-boot-order = "same-as-spl", &sdmmc, "/sdhci@fe330000";
	};
};

u-boot,spl-boot-device property
-------------------------------

This property is a companion-property to the u-boot,spl-boot-order and
will be injected automatically by the SPL stage to notify a later stage
of where said later stage was booted from.

You should not define this property yourself in the device-tree, as it
may be overwritten without warning.
```

观察：属性是一个按书写顺序探测的设备列表；每一项可为节点引用、完整路径或 alias。`"same-as-spl"` 在其所在位置请求插入 SPL 实际启动来源，但是否插入取决于 SoC/板级实现与后续阶段可达性，且可能与显式设备重复。故 EVB1 的 `"same-as-spl", &sdhci` 可解释为“先尝试继承 SPL 启动来源，再尝试 `&sdhci`”，但必须由 Rockchip 实现确认前项是否生效；不能据此推断 R1 厂商镜像使用同一顺序。文档还说明 `u-boot,spl-boot-device` 应由 SPL 自动注入，不能在静态 DTS 中自行定义。

### 步骤 12：阅读 Rockchip 的 SPL boot order 实现

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：验证 Rockchip v2026.07 如何将 `/chosen/u-boot,spl-boot-order` 转换为 SPL 的启动设备列表。

```fish
sed -n '100,220p' arch/arm/mach-rockchip/spl-boot-order.c
```

实际输出（学习者提供；关键函数全文）：

```c
void board_boot_order(u32 *spl_boot_list)
{
	int idx = 0;

	/* Add RAM boot for maskrom mode boot over USB */
	if (BROM_BOOTSOURCE_ID_ADDR && CONFIG_IS_ENABLED(RAM_DEVICE) &&
	   read_brom_bootsource_id() == BROM_BOOTSOURCE_USB) {
		spl_boot_list[idx++] = BOOT_DEVICE_RAM;
	}

	/* In case of no fdt (or only plat), use spl_boot_device() */
	if (!CONFIG_IS_ENABLED(OF_CONTROL) || CONFIG_IS_ENABLED(OF_PLATDATA)) {
		spl_boot_list[idx++] = spl_boot_device();
		return;
	}

	const void *blob = gd->fdt_blob;
	int chosen_node = fdt_path_offset(blob, "/chosen");
	int elem;
	int boot_device;
	int node;
	const char *conf;

	if (chosen_node < 0) {
		debug("%s: /chosen not found, using spl_boot_device()\n",
		     __func__);
		spl_boot_list[idx++] = spl_boot_device();
		return;
	}

	for (elem = 0;
	    (conf = fdt_stringlist_get(blob, chosen_node,
					"u-boot,spl-boot-order", elem, NULL));
	    elem++) {
		const char *alias;

		/* Handle the case of 'same device the SPL was loaded from' */
		if (strncmp(conf, "same-as-spl", 11) == 0) {
			conf = board_spl_was_booted_from();
			if (!conf)
				continue;
		}

		/* First check if the list element is an alias */
		alias = fdt_get_alias(blob, conf);
		if (alias)
			conf = alias;

		/* Try to resolve the config item (or alias) as a path */
		node = fdt_path_offset(blob, conf);
		if (node < 0) {
			debug("%s: could not find %s in FDT\n", __func__, conf);
			continue;
		}

		/* Try to map this back onto SPL boot devices */
		boot_device = spl_node_to_boot_device(node);
		if (boot_device < 0) {
			debug("%s: could not map node %s to a boot-device\n",
			      __func__, conf);
			continue;
		}

		spl_boot_list[idx++] = boot_device;
	}

	/* If we had no matches, fall back to spl_boot_device */
	if (idx == 0)
		spl_boot_list[0] = spl_boot_device();
}
```

观察：

- 函数先在 MaskROM USB + RAM 设备条件成立时，将 `BOOT_DEVICE_RAM` 放到列表开头。
- 没有可用 FDT、没有 `/chosen` 或没有任何可映射项时，回退到 `spl_boot_device()`。
- 有 `/chosen` 时，它按元素序号读取 `u-boot,spl-boot-order`；`same-as-spl` 被替换为 `board_spl_was_booted_from()` 的运行时返回值，若返回空则跳过。
- 随后代码依次尝试 alias 解析、FDT 路径解析和节点到 `BOOT_DEVICE_*` 的映射，成功才加入 `spl_boot_list`。
- 这验证了上游 Rockchip v2026.07 的具体处理路径；当前 R1 使用厂商 U-Boot 2017.09，不能据此断言其实现完全相同。`board_spl_was_booted_from()`、`spl_node_to_boot_device()` 以及 `&sdhci` 编译后如何进入该字符串读取路径仍待追踪。

### 步骤 13：定位运行时来源与节点映射函数入口

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：定位 `same-as-spl` 的运行时来源函数与设备树节点映射函数的定义位置，保留日后深入阅读的入口。

```fish
rg -n 'board_spl_was_booted_from|spl_node_to_boot_device' arch/arm/mach-rockchip
```

实际输出（学习者提供）：

```text
arch/arm/mach-rockchip/spl.c
51:const char *board_spl_was_booted_from(void)

arch/arm/mach-rockchip/spl-boot-order.c
17: * spl_node_to_boot_device() - maps from a DT-node to a SPL boot device
35:static int spl_node_to_boot_device(int node)
86: * board_spl_was_booted_from() - retrieves the of-path the SPL was loaded from
97:__weak const char *board_spl_was_booted_from(void)
141:			conf = board_spl_was_booted_from();
159:		boot_device = spl_node_to_boot_device(node);
183:		/* Revert spl_node_to_boot_device() logic to find appropriate SPI flash device */
208:				conf = board_spl_was_booted_from();
267:	const char *bootrom_ofpath = board_spl_was_booted_from();
```

观察：搜索定位了 `board_spl_was_booted_from()` 的 Rockchip 默认弱实现和 `spl.c` 中的板级实现，以及 `spl_node_to_boot_device()` 的本文件静态映射函数。当前已能解释“属性如何进入启动设备列表”的完整层次；不继续逐行追踪函数实现，以避免脱离当前 U-Boot 学习目标。若后续需要自定义启动顺序、定位 MaskROM→SPL 来源或设计 Linux+Zephyr 启动链，可从这些入口继续。

### 步骤 14：首次尝试隔离输出目录的 defconfig 生成（路径错误）

执行端：Arch Linux 主机 fish Shell；学习者当时已位于 U-Boot 源码目录。

目的：仅生成 EVB RK3588 的隔离 `.config`，不编译或烧录。此前给出的命令假定从仓库根目录执行，与实际当前目录不一致。

```fish
make -C src/u-boot-upstream O=/home/loser/Study/rk3588/build/uboot-evb-rk3588 CROSS_COMPILE=aarch64-linux-gnu- evb-rk3588_defconfig
```

实际输出（学习者提供）：

```text
make: 进入目录“/home/loser/Study/rk3588/src/u-boot-upstream”
make: *** src/u-boot-upstream: 没有那个文件或目录。 停止。
make: 离开目录“/home/loser/Study/rk3588/src/u-boot-upstream”
```

观察：命令未完成 defconfig 生成。原因是学习者已经在 `src/u-boot-upstream`，而命令仍使用仅适用于仓库根目录的 `-C src/u-boot-upstream` 相对路径形式。该失败不涉及 R1、烧录或源码修改；隔离输出目录是否创建尚未检查。下一次应在当前源码目录省略 `-C`。

### 步骤 15：在当前源码目录生成隔离 defconfig

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：在仓库的 `build/uboot-evb-rk3588/` 隔离输出目录生成上游 EVB RK3588 配置，不编译目标镜像。

```fish
make O=/home/loser/Study/rk3588/build/uboot-evb-rk3588 CROSS_COMPILE=aarch64-linux-gnu- evb-rk3588_defconfig
```

实际输出（学习者提供）：

```text
  HOSTCC  scripts/basic/fixdep
  GEN     Makefile
  HOSTCC  scripts/kconfig/conf.o
  YACC    scripts/kconfig/zconf.tab.[ch]
  LEX     scripts/kconfig/zconf.lex.c
  HOSTCC  scripts/kconfig/zconf.tab.o
  HOSTLD  scripts/kconfig/conf
#
# configuration written to .config
```

观察：命令完成并报告配置写入 `.config`。`HOSTCC`、`YACC`、`LEX` 与 `HOSTLD` 表明此阶段只在主机构建 Kconfig 配置工具；没有出现 AArch64 目标编译或 U-Boot 镜像名称。因使用 `O=`，预期 `.config` 位于隔离输出目录，但需下一步直接核对路径和内容，不能仅凭输出假定。

### 步骤 16：核对隔离输出目录中的 `.config`

执行端：Arch Linux 主机 fish Shell；当前目录不影响此绝对路径检查。

目的：验证 defconfig 的完整配置输出位于预期隔离目录，而不是写入上游源码树。

```fish
stat -c 'path=%n; type=%F; size=%s bytes' /home/loser/Study/rk3588/build/uboot-evb-rk3588/.config
```

实际输出（学习者提供）：

```text
path=/home/loser/Study/rk3588/build/uboot-evb-rk3588/.config; type=一般文件; size=57552 bytes
```

观察：`.config` 是隔离输出目录中的普通文件，大小为 57552 字节。它是 Kconfig 展开后的完整配置，通常比精简的 `configs/*_defconfig` 大得多；尚未读取其具体配置项，也未开始目标编译。

### 步骤 17：对照隔离 `.config` 的关键项

执行端：Arch Linux 主机 fish Shell；当前目录不影响此绝对路径检查。

目的：验证 defconfig 中关心的设备树、调试串口和启动载荷选项是否进入最终完整配置。

```fish
rg -n '^(CONFIG_(DEFAULT_DEVICE_TREE|DEFAULT_FDT_FILE|DEBUG_UART_BASE|BAUDRATE|FIT|SPL_LOAD_FIT|SPL_ATF))=' /home/loser/Study/rk3588/build/uboot-evb-rk3588/.config
```

实际输出（学习者提供）：

```text
207:CONFIG_DEFAULT_DEVICE_TREE="rockchip/rk3588-evb1-v10"
286:CONFIG_DEBUG_UART_BASE=0xFEB50000
452:CONFIG_FIT=y
466:CONFIG_SPL_LOAD_FIT=y
553:CONFIG_DEFAULT_FDT_FILE="rockchip/rk3588-evb1-v10.dtb"
721:CONFIG_SPL_ATF=y
1795:CONFIG_BAUDRATE=1500000
```

观察：选取的关键项均出现在完整 `.config`，且值与已读 defconfig 一致。它验证了“精简 defconfig → Kconfig 展开 `.config`”的结果；未查询 `CONFIG_SPL_FIT_SIGNATURE`，也尚未进行 AArch64 目标编译或生成 U-Boot 镜像。

### 步骤 18：首次完整构建在主机工具阶段停止

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：首次执行完整构建，观察主机工具、交叉编译与目标产物的先后关系。该命令只会更新隔离输出目录，不连接或烧录 R1。

```fish
make O=/home/loser/Study/rk3588/build/uboot-evb-rk3588 CROSS_COMPILE=aarch64-linux-gnu- -j(nproc)
```

其中 `-j(nproc)` 是 fish 命令替换：先运行 `nproc` 取得可用处理器数，再将数值传给 make 的 `-j`，以并行执行独立构建任务。它不会改变配置或目标代码，因此日志可交错出现。

实际输出（学习者提供，错误前的完整日志未保存）：

```text
HOSTLD  scripts/dtc/fdtoverlay
error: command 'swig' failed: No such file or directory
make[3]: *** [/home/loser/Study/rk3588/src/u-boot-upstream/scripts/dtc/pylibfdt/Makefile:33：rebuild] 错误 1
make[2]: *** [/home/loser/Study/rk3588/src/u-boot-upstream/scripts/Makefile.build:497：scripts/dtc/pylibfdt] 错误 2
make[2]: *** 正在等待未完成的任务....
make[1]: *** [/home/loser/Study/rk3588/src/u-boot-upstream/Makefile:2403：scripts_dtc] 错误 2
make: *** [Makefile:189：__sub-make] 错误 2
```

观察：失败路径是主机侧 `scripts/dtc/pylibfdt`，报错直接点名不可执行的 `swig`。这不能归因于 AArch64 交叉编译器、EVB1 与 R1 的板级差异或开发板；此刻也不能宣称未生成任何目标文件，因为尚未盘点输出目录。下一步仅只读确认主机 PATH 是否能解析 `swig`，见[ISSUE-20260811-001](../issue/issue-20260811-001-uboot-build-missing-swig.md)。

### 步骤 19：安装 SWIG 后的 pylibfdt Python API 编译失败

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：在已安装 `swig` 后继续同一次主机侧构建，定位下一处实际阻塞；不改动 U-Boot 源码或开发板。

实际输出（学习者提供，节选）：

```text
PYMOD   rebuild
scripts/dtc/pylibfdt/libfdt_wrap.c:5618:20: error: implicit declaration of function ‘PyInt_AsLong’; did you mean ‘PyLong_AsLong’?
scripts/dtc/pylibfdt/libfdt_wrap.c:6679:19: error: implicit declaration of function ‘PyString_FromString’; did you mean ‘PyLong_FromString’?
... '-I/usr/include/python3.14' ... '-c', 'scripts/dtc/pylibfdt/libfdt_wrap.c' ... returned non-zero exit status 1.
```

补充只读源码证据：`scripts/dtc/pylibfdt/Makefile` 的 `pymod` 命令以 `$(PYTHON3) setup.py ... build_ext --inplace` 构建绑定；同树 `libfdt.i_shipped` 直接包含 `PyString_FromString` 与 `PyInt_AsLong`。这解释了这些名称为何会出现在生成的 `libfdt_wrap.c` 中。

观察：`PyInt_*`、`PyString_*` 是 Python 2 API，而编译命令采用 Python 3.14 头文件。当前可确认的是该接口输入与当前解释器接口不匹配，尚不能仅凭日志断言应降级 Python、升级 U-Boot 或修改接口文件。下一步先只读记录 Python 和 SWIG 的精确版本，见[ISSUE-20260811-001](../issue/issue-20260811-001-uboot-build-missing-swig.md)。

### 步骤 20：确认主机构建工具版本

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：记录实际供 `pylibfdt` 使用的 Python 与生成包装代码的 SWIG 版本，避免将“Python 3”笼统当作单一环境。

```fish
python3 --version; and swig -version
```

实际输出（学习者提供）：

```text
Python 3.14.6

SWIG Version 4.5.0
Compiled with g++ [x86_64-pc-linux-gnu]
Configured options: +pcre
```

观察：工具链版本已明确为 Python 3.14.6 与 SWIG 4.5.0。生成的 `libfdt_wrap.c` 头部显示 `SWIG_VERSION 0x040500`，与该 SWIG 版本一致；其是否包含旧 Python API 的兼容别名尚待检查。该检查不改变源码、构建目录或开发板。

### 步骤 21：验证生成文件中不存在旧 API 兼容别名

执行端：Arch Linux 主机 fish Shell；当前目录不影响绝对路径检查。

目的：检查 SWIG 4.5 生成的包装 C 文件是否自己定义旧 Python API 到 Python 3 API 的映射。

```fish
rg -n -C 2 '#define PyInt_AsLong|#define PyString_FromString' /home/loser/Study/rk3588/build/uboot-evb-rk3588/scripts/dtc/pylibfdt/libfdt_wrap.c; or echo 'no compatibility aliases found'
```

实际输出（学习者提供）：

```text
no compatibility aliases found
```

观察：模式未匹配，fish 于是执行 `or` 分支。这证实生成文件没有为这两个旧 API 定义兼容别名。结合接口模板中的直接调用、SWIG 4.5.0 与 Python 3.14.6，可闭合当前编译失败的因果链；仍需完整盘点模板中的旧 API，才能界定最小修复范围。

### 步骤 22：盘点接口模板中的旧 Python API 与条件路径

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：完整找出模板里的 `PyInt_*`、`PyString_*` 用法，并区分当前 Python 3 构建是否会编译它们。

```fish
rg -n -C 2 'Py(Int|String)_' scripts/dtc/pylibfdt/libfdt.i_shipped
```

实际输出（学习者提供）：

```text
1036: resultobj = PyString_FromString(...)
1065: $1 = PyBytes_AsString($input);
1066: %#else
1067: $1 = PyString_AsString($input);
1073: depth = (int) PyInt_AsLong($input);
```

观察：`PyString_FromString` 与 `PyInt_AsLong` 没有受条件编译保护，会进入当前 Python 3 的生成 C 文件；两者正是编译器报错的名称。`PyString_AsString` 位于 `%#else`，其中 `%#` 要求 SWIG 将指令原样交给 C 预处理器；其配对条件在此前行中是 Python 3 使用 `PyBytes_AsString`，因此当前 Python 3 构建不会编译该 `else` 内容。最小活跃修复范围由此收敛为前两处；尚未编辑源码。

### 步骤 23：创建 pylibfdt 兼容性实验分支

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：将即将进行的最小补丁与已验证的 `v2026.07` detached HEAD 学习基线分开，使后续差异可审阅、可回退。

```fish
git switch -c study/pylibfdt-py3-swig45
git status --short --branch
```

实际结果（学习者提供）：已创建分支；原始状态输出未保存。

观察：学习者报告分支创建完成。该操作仅创建本地 Git 引用，不会改动工作树文件、构建输出或 R1；具体分支状态和是否存在既有文件变更尚未由保存的输出验证。下一步将在此分支中只编辑两处已确认的无条件旧 API，并立即审阅 diff。

### 步骤 24：在实验分支应用并审阅两行 Python 3 API 补丁

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream` 的 `study/pylibfdt-py3-swig45` 分支。

目的：只替换当前 Python 3 构建实际会编译的两处旧 API，不触及 Python 2 条件分支，并在重建前审阅精确差异。

实际 diff（学习者提供）：

```diff
- resultobj = PyString_FromString(
+ resultobj = PyUnicode_FromString(

- depth = (int) PyInt_AsLong($input);
+ depth = (int) PyLong_AsLong($input);
```

观察：差异只含预期的两行替换。`fdt_string()` 产生属性名的 C 字符串，`PyUnicode_FromString()` 对应 Python 3 的 Unicode 字符串构造；深度参数的 Python 整数转换改用 `PyLong_AsLong()`。学习者提供的组合命令输出中未出现 `git diff --check` 诊断，且展示 diff 没有额外行；因此可继续重建验证。尚未提交该补丁，尚未改动开发板。

### 步骤 25：补丁后重建越过 pylibfdt，停在 Binman 外部载荷检查

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream` 的实验分支。

目的：验证两行 Python 3 API 补丁能否使构建通过 pylibfdt，并观察下一个真实构建依赖。

实际输出（学习者提供，节选）：

```text
BINMAN  .binman_stamp
Image 'simple-bin' is missing external blobs and is non-functional: rockchip-tpl atf-bl31
... external TPL is required to initialize DRAM ... ROCKCHIP_TPL=/path/to/ddr.bin
... build with BL31=/path/to/bl31.bin
Image 'simple-bin' is missing optional external blobs but is still functional: tee-os
Some images are invalid
make: *** [Makefile:189：__sub-make] 错误 2
```

观察：本次输出已到达 `BINMAN`，且不再出现先前 pylibfdt 的 Python API 编译错误，故两行补丁对 ISSUE-20260811-001 的回归验证通过。当前阻塞转为 Binman 的外部 TPL 与 BL31 输入缺失；`tee-os` 被标为 optional。Binman 明确把无 TPL/BL31 的 `simple-bin` 标为 non-functional，不能将任何已生成残留文件当作可启动镜像，更不能用于 R1。建立[ISSUE-20260811-002](../issue/issue-20260811-002-uboot-missing-external-boot-blobs.md)继续跟踪。

### 步骤 26：读取 Rockchip Binman 描述的外部载荷节点

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：从 U-Boot 的 Rockchip 通用 Binman DTS 读取 `simple-bin` 所需外部载荷的声明，而不是依据报错文字猜测其镜像位置。

```fish
sed -n '70,195p' arch/arm/dts/rockchip-u-boot.dtsi
```

实际输出（学习者提供，相关片段）：

```dts
#ifdef CONFIG_ROCKCHIP_EXTERNAL_TPL
        rockchip-tpl {
        };

#ifdef CONFIG_ARM64
        @atf-SEQ {
                fit,operation = "split-elf";
                description = "ARM Trusted Firmware";
                ...
                atf-bl31 {
                };
        };
        @tee-SEQ {
                fit,operation = "split-elf";
                description = "TEE";
                ...
                tee-os {
                        optional;
                };
        };
```

观察：`rockchip-tpl` 受 `CONFIG_ROCKCHIP_EXTERNAL_TPL` 条件控制；`@atf-SEQ` 是 Binman 的序号占位镜像节点，不是硬件设备节点。其 `split-elf` 表示由 Binman 从 BL31 ELF 提取适合 FIT 的装载段；这与 R1 厂商早期 FIT 日志中的多个 `atf-*` 校验项形成概念对应，但不能说明两者二进制相同。`tee-os` 标记 `optional`，解释了 Binman 对其的不同严重级别。学习者粘贴了两个 `@tee-SEQ` 片段，但缺少各自外围 `#if/#endif`，不能据此断言源码中有同名重复生效节点。下一步读取最终 `.config`，确认实际选择的条件。

### 步骤 27：核对最终配置的外部载荷条件

执行端：Arch Linux 主机 fish Shell；当前目录不影响绝对路径检查。

目的：将 Binman DTS 的条件节点与实际 EVB 构建配置逐项对应，区分“源码可能包含”与“本次构建已启用”。

```fish
rg -n '^(CONFIG_(ARM64|ROCKCHIP_EXTERNAL_TPL|SPL_ATF))=' /home/loser/Study/rk3588/build/uboot-evb-rk3588/.config
```

实际输出（学习者提供）：

```text
54:CONFIG_ARM64=y
235:CONFIG_ROCKCHIP_EXTERNAL_TPL=y
721:CONFIG_SPL_ATF=y
```

观察：三项均为 `y`。`CONFIG_ARM64` 使 AArch64 的 BL31/TEE Binman 分支参与，`CONFIG_ROCKCHIP_EXTERNAL_TPL` 使外部 DDR TPL 节点参与，`CONFIG_SPL_ATF` 则与 SPL/ATF 启动链配置一致。它们解释本次 EVB 构建为何要求外部 TPL 与 BL31；不证明这些输入已存在、也不证明任意同名文件适合 R1。

### 步骤 28：阅读上游对 TF-A 与外部 BL31/TPL 的通用说明

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：区分“自行构建 TF-A”与“使用 Rockchip 提供的 BL31”，并确认 TPL 在上游构建示例中通过环境变量作为外部输入传入。

实际输出（学习者提供，节选）说明：

- TF-A 是构建 ARM64 Rockchip SoC 镜像所需组件；示例用 `PLAT=rk3399` 构建，文档要求改成所需 Rockchip 平台。
- 若某 SoC 的 TF-A 代码没有公开，文档建议使用 Rockchip 提供的 BL31 二进制，并定位 `rkbin` 来源。
- 随后的 rk3576 示例以 `BL31=...rk3576_bl31...elf` 与 `ROCKCHIP_TPL=...rk3576_ddr...bin` 作为输入，再运行对应 defconfig 与构建。

观察：这份资料解释 `BL31` 与 `ROCKCHIP_TPL` 的角色和输入形式，但其中实际文件名属于 rk3576 示例，不能替代 RK3588、更不能替代 R1。输出在 “To build rk3588 boards:” 处截断，下一步只读取其后 RK3588 专用的几行。

### 步骤 29：读取上游 RK3588 EVB 的外部载荷示例

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：核对上游 `evb-rk3588_defconfig` 需要的外部输入名称，并将其与当前 Binman 缺失项和构建配置对应。

实际输出（学习者提供）：

```bash
export BL31=../rkbin/bin/rk35/rk3588_bl31_v1.33.elf
export ROCKCHIP_TPL=../rkbin/bin/rk35/rk3588_ddr_lp4_2112MHz_lp5_2736MHz_v1.09.bin
make evb-rk3588_defconfig
make CROSS_COMPILE=aarch64-linux-gnu-
```

观察：该上游示例与当前选用的 `evb-rk3588_defconfig` 一致，并明确了 Binman 所期望的输入类型：`BL31` 是 RK3588 BL31 ELF，`ROCKCHIP_TPL` 是带 DDR 参数命名的 RK3588 二进制。示例紧随 “To build rk3588 boards:” 标题，因而可作为**上游 RK3588 EVB 主机构建**的资料依据。它不等于 R1 配方：R1 运行时设备树为 `rk3588s-evb4-lp4x-v10`，而当前配置默认 EVB1；尚未下载、核验或使用上述文件，未进入后续 Flashing 段落。

### 步骤 30：比较 EVB 默认 DTS 与 R1 的板级身份

执行端：Arch Linux 主机 fish Shell；当前目录：`~/Study/rk3588/src/u-boot-upstream`。

目的：验证当前主机构建的默认板级 DTS 是否与 R1 运行时设备树为同一具体板型，而不是只比较二者共有的通用 SoC 字符串。

```fish
rg -n '^(\s*(model|compatible)\s*=)' \
  dts/upstream/src/arm64/rockchip/rk3588-evb1-v10.dts
```

实际输出（学习者提供，节选）：

```text
17: model = "Rockchip RK3588 EVB1 V10 Board";
18: compatible = "rockchip,rk3588-evb1-v10", "rockchip,rk3588";
```

观察：命令同样列出音频、电源、USB-C 等子节点的 `compatible`，但整板身份只由根节点的前两行给出。它与 R1 运行时根节点 `Rockchip RK3588S EVB4 LP4X V10 Board`、`rockchip,rk3588s-evb4-lp4x-v10`, `rockchip,rk3588` 不同。两者共有 `rockchip,rk3588` 只是通用 SoC 回退兼容项；这可让部分 SoC 级驱动共享匹配，不表示 DDR 参数、PMIC、引脚、外设连接或启动载荷可互用。因此上游 EVB 的 BL31/TPL 示例只能作为主机打包学习样例，不能直接用于 R1。

### 步骤 31：检查本地 rkbin 获取状态

执行端：Arch Linux 主机 fish Shell；当前目录不影响绝对路径检查。

目的：在取得外部输入前确认是否已有独立的 `rkbin` 工作树，避免覆盖未知本地文件。

```fish
test -d /home/loser/Study/rk3588/src/rkbin; and echo 'rkbin exists'; or echo 'rkbin is not present'
```

实际输出（学习者提供）：

```text
rkbin is not present
```

观察：本地不存在 `src/rkbin`，因此没有可供核验的 BL31/TPL 文件，也没有覆盖既有目录的风险。下一步可从上游文档指定的仓库创建独立、浅克隆的 EVB 主机打包资源；该目录已加入 `.gitignore`，不会把第三方源码整体纳入本学习仓库。克隆不会改动 R1、U-Boot 源码或现有构建输出。

### 步骤 32：报告 rkbin 浅克隆完成，等待身份核验

执行端：Arch Linux 主机；目标目录：`/home/loser/Study/rk3588/src/rkbin`。

学习者报告：已完成上游 `rockchip-linux/rkbin` 的浅克隆。该报告尚未包含 Git 提交、分支状态或文件清单，因此当前只记录为“已获取、待核验”，不将其中任何二进制视为已验证输入。

### 步骤 33：核验 rkbin 工作树与精确提交

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/rkbin`。

目的：确认已克隆仓库的当前分支、工作树状态和可复现的 HEAD。

```fish
git status --short --branch; and git log -1 --format='commit=%H%nsubject=%s'
```

实际输出（学习者提供）：

```text
## master...origin/master
commit=ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4
subject=README: Update
```

观察：`master...origin/master` 表示本地 `master` 跟踪远端同名分支；没有额外文件状态行，故当前输出未显示工作树修改或提交领先/落后标记。精确 HEAD 已记录为 `ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4`。该浅克隆不是固定 release/tag，也没有执行签名验证；下一步只核对文档点名的两个文件是否存在、其大小和 SHA-256。

### 步骤 34：验证文档点名的精确文件名是否仍存在

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/rkbin`。

目的：以 `test -f` 验证上游 U-Boot 文档列出的 RK3588 EVB BL31/TPL 精确路径，避免按相似文件名替换输入。

实际输出（学习者提供）：

```text
missing: bin/rk35/rk3588_bl31_v1.33.elf
missing: bin/rk35/rk3588_ddr_lp4_2112MHz_lp5_2736MHz_v1.09.bin
```

观察：当前 `rkbin` HEAD 中没有文档写出的两份精确文件，因此尚未计算任何文件哈希，也不能继续设置构建变量。**推测**：U-Boot 文档示例与当前 `rkbin` 默认分支的文件版本或命名已发生变化。先列出当前 HEAD 中的 RK3588 BL31/DDR 候选文件，随后再判断应更新文档版本、固定资源提交，还是选择其他学习路径；不得按名称近似程度直接选用。

### 步骤 35：列出当前 rkbin 的 RK3588 BL31/DDR 候选

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/rkbin`。

目的：获取当前资源快照中的实际候选清单，区分 BL31 版本变化与 DDR TPL 的频率/变体差异。

实际输出（学习者提供）：

```text
bin/rk35/rk3588_bl31_v1.54.elf
bin/rk35/rk3588_ddr_lp4_1848MHz_lp5_2112MHz_eyescan_v1.20.bin
bin/rk35/rk3588_ddr_lp4_1848MHz_lp5_2112MHz_v1.21.bin
bin/rk35/rk3588_ddr_lp4_2112MHz_lp5_2400MHz_eyescan_v1.20.bin
bin/rk35/rk3588_ddr_lp4_2112MHz_lp5_2400MHz_v1.21.bin
```

观察：当前 HEAD 有一个更新的 BL31 名称（v1.54），以及四个 DDR TPL 候选。DDR 候选至少按 LP4/LP5 标注频率、是否含 `eyescan`、版本号区分；仅从文件名不能证明哪一个对应旧文档的 EVB 示例，更不能判断 R1 的适用性。下一步记录当前资源快照的提交时间与浅克隆状态，再决定是否需要追溯文档对应的历史版本。

### 步骤 36：记录 rkbin 当前提交的作者时间与浅克隆状态

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/rkbin`。

目的：确定文档示例与当前资源快照不一致时，是否具备本地历史可供追溯；同时区分 Git 的作者时间与远端分支的实际最新状态。

```fish
git log -1 --format='commit=%H%nauthor-date=%aI%nsubject=%s'
git rev-parse --is-shallow-repository
```

实际输出（学习者提供）：

```text
commit=ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4
author-date=2025-12-29T11:11:15+08:00
subject=README: Update
true
```

观察：`%aI` 输出的是作者时间，表明这条提交由作者在该时间写成；它不单独证明远端 `master` 最后更新于该时刻。`true` 直接确认本地仓库是浅克隆，本地没有更早提交可用于查找文档点名的旧文件。`--depth 1` 限制的是历史深度，而不是提交年龄：若远端分支顶端本身长期没有新提交，浅克隆得到数月前的顶端是正常现象。下一步以只读的 `git ls-remote` 读取远端当前 `master` 提交，并同时查看本地提交者时间，避免把本地快照误当作远端现状。

### 步骤 37：核验远端 master 与本地快照是否一致

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/rkbin`。

目的：验证“当前文件缺失”是否只是本地浅克隆过旧，还是远端 `master` 本身已与 U-Boot 文档引用的资源版本发生漂移。

```fish
git show -s --format='commit=%H%nauthor-date=%aI%ncommitter-date=%cI%nsubject=%s' HEAD
git ls-remote origin refs/heads/master
```

实际输出（学习者提供）：

```text
commit=ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4
author-date=2025-12-29T11:11:15+08:00
committer-date=2025-12-30T19:41:07+08:00
subject=README: Update
ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4	refs/heads/master
```

观察：远端 `master` 与本地 HEAD 的提交 ID 完全一致，因此本地克隆没有落后；远端当前顶端提交者时间为 `2025-12-30T19:41:07+08:00`。这排除了“仅因浅克隆没有取得较新的 master 资源”的解释。**已验证结论**：U-Boot `v2026.07` 文档引用的 `v1.33`/`v1.09` 路径，与当前 rkbin `master` 的资源集合存在版本或命名漂移。漂移的具体历史提交和旧文件内容尚未验证；不得因此选择任何当前 DDR TPL。

### 步骤 38：按精确路径定位 rkbin 历史变更

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/rkbin`。

目的：不读取旧二进制内容，只根据 Git 历史定位 U-Boot 文档所列 BL31/TPL 路径的引入和替换提交。

```fish
git log --all --format='%H %cI %s' -- bin/rk35/rk3588_bl31_v1.33.elf bin/rk35/rk3588_ddr_lp4_2112MHz_lp5_2736MHz_v1.09.bin
```

实际输出（学习者提供）：

```text
8eada29ea5a0903a58324c49c7aa52aeccfd1b49 2023-03-02T20:43:53+08:00 rk3588: ddr: update ddrbin to v1.10
b95a8e92903079e7ddf1ef43e4bac22d73c2d20a 2023-01-10T11:29:17+08:00 rk3588: bl31: update version to v1.34
27d8af0c9204a6094368e0eeea220841f5853d03 2022-12-07T16:36:18+08:00 rk3588: bl31: update version to v1.33
8ba55b28833382dcca033db78bca99f096530535 2022-11-21T17:51:06+08:00 rk3588: ddr: update ddrbin to v1.09
```

观察：路径历史显示 v1.09 DDR 与 v1.33 BL31 都曾被提交到 rkbin；其后分别出现“更新到 v1.10”和“更新到 v1.34”的提交，这解释了为何当前 `master` 不再保留旧名称。提交 `27d8...` 位于 v1.09 引入之后、v1.34 替换之前，**推测**它是同时包含文档所列两个路径的候选历史点。下一步仅用 `git ls-tree` 查看该提交的树条目，验证两条路径是否确实同时存在；不读取 blob 内容，也不将其用于构建。

### 步骤 39：盘点 Binman 停止前的顶层构建输出

执行端：Arch Linux 主机 fish Shell；当前目录不影响绝对路径检查。

目的：区分已完成的编译/中间打包产物与被外部 TPL 缺失影响的最终启动镜像，不重建、不使用任何二进制。

```fish
find /home/loser/Study/rk3588/build/uboot-evb-rk3588 -maxdepth 1 -type f -printf '%f\t%s bytes\n' | sort
```

实际输出：完整原始清单见[uboot-evb-output-inventory-20260811.txt](../_assets/uboot-evb-output-inventory-20260811.txt)。

观察：`.config`、`generated_defconfig` 和命令追踪文件证明隔离构建目录已配置；`u-boot`、`u-boot-nodtb.bin`、`u-boot.dtb`、`u-boot-dtb.bin`、`u-boot.itb`、`simple-bin.fit.itb`、符号表和映射文件均为非零，说明构建在 Binman 最终报错前已生成多类目标代码、设备树和打包中间物。`idbloader.img` 与 `mkimage-in-simple-bin.mkimage-rockchip-tpl` 均为 0 字节，和先前缺少 `rockchip-tpl` 的报错相符；但仅凭文件名和大小不能证明任何非零镜像可启动，尤其不能用于 R1。下一步用 `file` 识别少数代表文件的格式。

### 步骤 40：识别代表性构建输出的格式

执行端：Arch Linux 主机 fish Shell；当前目录不影响绝对路径检查。

目的：以文件头识别结果区分带有明确格式容器的文件与无自描述头的裸二进制，并避免把 `file` 的启发式猜测当作构建结论。

```fish
file /home/loser/Study/rk3588/build/uboot-evb-rk3588/{u-boot,u-boot-nodtb.bin,u-boot.dtb,u-boot-dtb.bin,u-boot-rockchip.bin,idbloader.img,simple-bin.fit.itb}
```

实际输出：完整原始输出见[uboot-evb-file-types-20260811.txt](../_assets/uboot-evb-file-types-20260811.txt)。

观察：`u-boot` 被识别为 AArch64、静态链接、保留调试信息且未 strip 的 ELF，可作为源码/符号调试入口；`u-boot.dtb` 是 DTB v17。`simple-bin.fit.itb` 同样被识别为 DTB v17，这与 FIT 以 FDT 格式作为容器的机制一致，并不表示它只是普通板级 DTB。`idbloader.img` 为 empty，与先前 0 字节长度一致。相反，`u-boot-nodtb.bin`、`u-boot-dtb.bin` 被猜为 PCX，`u-boot-rockchip.bin` 被猜为 ISO-8859 文本；这些文件没有被证实具有对应格式，`file` 对无可靠魔数的原始字节使用启发式规则，故这三项分类不能当作格式结论。下一步通过长度关系和字节前缀/后缀验证 `u-boot-dtb.bin` 是否由 `u-boot-nodtb.bin` 与 DTB 拼接而成。

### 步骤 41：验证 U-Boot 裸程序与 DTB 的拼接关系

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/u-boot-upstream`。

目的：以长度、前缀和后缀三项独立证据验证 `u-boot-dtb.bin` 的组成，而不根据文件名或 `file` 猜测。

```fish
printf 'nodtb + dtb = %s; combined = %s\n' (math "$nodtb_size + $dtb_size") $combined_size
cmp -n $nodtb_size $out/u-boot-nodtb.bin $out/u-boot-dtb.bin; and echo 'prefix matches'
dd if=$out/u-boot-dtb.bin bs=1 skip=$nodtb_size status=none | cmp -s - $out/u-boot.dtb; and echo 'suffix matches DTB'
```

实际输出（学习者提供）：

```text
nodtb + dtb = 1022568; combined = 1022568
prefix matches
suffix matches DTB
```

观察：长度相加相等、前 819048 字节逐字节匹配 `u-boot-nodtb.bin`、剩余字节逐字节匹配 `u-boot.dtb` 三项同时成立。**已验证结论**：本次 EVB 构建的 `u-boot-dtb.bin` 精确为 `u-boot-nodtb.bin || u-boot.dtb`。这也解释它和 `u-boot-nodtb.bin` 被 `file` 同样误判为 PCX：二者开头字节相同。该结论描述构建产物关系，不验证任何镜像可启动，更不适用于 R1。

### 步骤 42：比较同尺寸 U-Boot 产物的完整内容

执行端：Arch Linux 主机 fish Shell；当前目录：`/home/loser/Study/rk3588/src/u-boot-upstream`。

目的：用完整内容哈希判断同尺寸的 `.bin`、`.img` 是否只是不同名称，避免只按大小推断。

```fish
sha256sum $out/u-boot.bin $out/u-boot-dtb.bin $out/u-boot.img $out/u-boot-dtb.img
```

实际输出（学习者提供）：

```text
0038913b60d294994733bef2f6c532c81446380a3eaffaf3aeb72189a7d92a3e  u-boot.bin
0038913b60d294994733bef2f6c532c81446380a3eaffaf3aeb72189a7d92a3e  u-boot-dtb.bin
1957bce247f4e665e03022db5060a1b70eb75a4d2fb9f943f6a8c8f397ec1d9a  u-boot.img
1957bce247f4e665e03022db5060a1b70eb75a4d2fb9f943f6a8c8f397ec1d9a  u-boot-dtb.img
```

观察：`u-boot.bin` 与 `u-boot-dtb.bin` 的 SHA-256 相同，故它们是字节完全相同的两个文件名；这与步骤 41 的拼接结论一致。`u-boot.img` 与 `u-boot-dtb.img` 也完全相同，但其哈希不同于 `.bin` 组；二者大小比 `.bin` 组多 984 字节。**推测**：`.img` 组由镜像工具添加了头部和/或填充。下一步定位主机构建出的 `dumpimage`，由 U-Boot 的工具读取 `.img` 元数据验证该推测。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 上游源码目录可获得 | `src/u-boot-upstream` 存在 | 学习者完成克隆并可进入该目录 | 通过 |
| 版本固定 | 精确 tag 为 `v2026.07` | `git describe --tags --exact-match` 输出 `v2026.07` | 通过 |
| 源码身份可复现 | HEAD 为可记录提交 ID | `ece349ade2973e220f524ce59e59711cc919263f` | 通过 |
| 无本地修改 | Git 短状态无文件项 | 仅显示 detached HEAD 状态 | 通过 |
| 顶层阅读入口可见 | 关键目录均存在 | `arch`、`board`、`boot`、`cmd`、`common`、`configs`、`doc`、`drivers`、`include` 均列出 | 通过 |
| RK3588 配置候选可发现 | 输出所有匹配 defconfig | 输出 30 份不同板型的 RK3588/RK3588S defconfig | 通过 |
| EVB 候选配置可阅读 | 可识别默认 DTS、串口和 FIT/SPL 选项 | 默认 DTS 为 EVB1 V10；串口基址/波特率匹配本板观察；FIT/SPL 选项存在 | 通过（非 R1 适配证明） |
| 默认 DTS 的 `arch/arm/dts/` 旧路径 | 发现完整 `.dts` 或证实当前布局不同 | 仅发现 `rk3588-evb1-v10-u-boot.dtsi` | 通过（旧路径假设被否定） |
| 完整 DTS 与 U-Boot `.dtsi` 的实际位置 | 两类文件可在全树定位 | 分别位于 `dts/upstream/...` 与 `arch/arm/dts/...` | 通过（合并方式待查） |
| 基础 EVB1 DTS 的继承与板级属性 | 可识别通用 include、板级标识与 stdout 配置 | 引用 `rk3588.dtsi`；声明 EVB1 标识及 `serial2:1500000n8` | 通过（非 R1 适配证明） |
| U-Boot 专用片段的内容 | 识别是否为启动阶段补充 | 引用 `rk3588-u-boot.dtsi`；添加 `u-boot,spl-boot-order` | 通过（属性语义待查） |
| SPL boot order 的解释入口 | 定位文档、代码候选和板级用例 | 找到 `chosen.txt`、Rockchip `spl-boot-order.c` 和多个 `.dtsi` | 通过（完整语义待读） |
| SPL boot order 的语法与顺序规则 | 识别元素形式和 `same-as-spl` 规则 | 文档确认有序列表、三种设备表示和实现定义的插入行为 | 通过（Rockchip 实现待读） |
| Rockchip 对 SPL boot order 的处理 | 确认读取、替换、解析和回退路径 | `board_boot_order()` 按顺序解析并生成 `spl_boot_list` | 通过（映射函数待追） |
| 运行时来源与节点映射入口 | 定位两个转换函数的实现位置 | 已定位默认/板级来源函数及节点映射函数 | 通过（实现细节暂不展开） |
| 隔离输出目录的 defconfig 生成 | 从正确目录调用 make 并生成 `.config` | 目录上下文与 `-C` 参数重复，命令停止 | 未通过（命令路径需更正） |
| 更正后的隔离 defconfig 生成 | 配置工具运行并写入 `.config` | make 报告 `configuration written to .config` | 通过（路径待核对） |
| `.config` 隔离路径与类型 | 普通文件位于指定输出目录 | 57552 字节普通文件，路径符合 `O=` | 通过 |
| defconfig 关键项展开结果 | DTS、串口与 FIT/SPL/ATF 选项进入 `.config` | 所查 7 个选项均存在且值符合 defconfig | 通过 |
| 首次完整构建 | 继续通过主机工具阶段并开始目标构建 | `scripts/dtc/pylibfdt` 找不到 `swig` 后停止 | 阻塞（主机依赖待确认） |
| 安装 SWIG 后继续构建 | pylibfdt 包装代码可用 Python 3 头文件编译 | `PyInt_*`、`PyString_*` 未声明，编译命令使用 Python 3.14 头文件 | 阻塞（兼容性待确认） |
| Python/SWIG 版本 | 可准确记录主机绑定构建环境 | Python 3.14.6；SWIG 4.5.0 | 通过 |
| 旧 API 兼容别名 | 生成文件定义 `PyInt_*`/`PyString_*` 映射或明确不存在 | 输出 `no compatibility aliases found` | 通过（不存在） |
| 旧 API 活跃路径盘点 | 区分 Python 3 实际参与编译的旧 API | 两处无条件用法、一处 Python 2 `else` 分支 | 通过 |
| 兼容性实验分支 | 创建独立本地分支且工作树状态可观察 | 学习者报告已创建；原始状态输出未保存 | 部分通过 |
| 两行 Python 3 API 补丁 | 仅替换 Python 3 活跃的旧 API | 展示 diff 仅包含两行预期替换 | 通过（重建待验证） |
| pylibfdt 补丁回归 | 同一构建越过 pylibfdt Python API 编译 | 已进入 `BINMAN .binman_stamp`，未重现旧 API 错误 | 通过 |
| 完整 Rockchip 启动镜像 | Binman 获得全部必需外部载荷 | 缺少 `rockchip-tpl`、`atf-bl31`，镜像 non-functional | 阻塞 |
| Binman 外部载荷声明 | 在源码中定位 TPL、BL31、OP-TEE 的镜像节点 | 读取 `rockchip-tpl` 条件节点、`@atf-SEQ` 和 optional `tee-os` | 通过（配置值待核对） |
| 外部载荷条件的最终配置 | 确认本次 EVB 构建是否启用 AArch64、外部 TPL 与 SPL/ATF | 三项均为 `y` | 通过 |
| 上游 TF-A/外部载荷通用说明 | 区分 TF-A 源码构建与外部 BL31/TPL 输入 | 文档给出 TF-A 平台构建原则和 rk3576 的 `BL31`/`ROCKCHIP_TPL` 示例 | 通过（RK3588 专用示例待读） |
| RK3588 EVB 外部载荷示例 | 取得与当前 defconfig 对应的输入名称 | 文档指定 `rk3588_bl31...elf`、`rk3588_ddr...bin` 与 `evb-rk3588_defconfig` | 通过（非 R1 适配证明） |
| EVB1 与 R1 的根节点身份 | 核对是否同一具体板型 | EVB1 为 `rk3588-evb1-v10`；R1 运行时为 `rk3588s-evb4-lp4x-v10`，仅通用 SoC 回退项相同 | 通过（不兼容假设被否定） |
| `rkbin` 本地状态 | 发现现有目录或确认可新建 | 输出 `rkbin is not present` | 通过 |
| `rkbin` 浅克隆 | 建立独立上游资源目录 | `true`；当前 HEAD 作者时间为 `2025-12-29T11:11:15+08:00` | 通过 |
| `rkbin` 仓库身份 | 工作树无修改并记录精确 HEAD | `master...origin/master`；HEAD `ecb4fcbe954edf38b3ae037d5de6d9f5bccf81f4` | 通过（未验证 tag/签名） |
| `rkbin` 远端 master 与本地快照 | 排除本地克隆落后的解释 | 远端 `refs/heads/master` 与本地 HEAD 均为 `ecb4fcbe...`；提交者时间为 `2025-12-30T19:41:07+08:00` | 通过（资源版本/命名漂移已确认） |
| 文档路径的 rkbin 历史 | 定位旧 BL31/TPL 的引入与替换提交 | v1.09 DDR、v1.33 BL31 均有历史提交，之后分别更新到 v1.10、v1.34 | 通过（同一提交点共存待核验） |
| Binman 停止前的顶层输出 | 区分已生成与明显未生成的产物 | 多类 U-Boot、DTB、FIT、符号/映射文件非零；`idbloader.img`、TPL mkimage 输入为 0 字节 | 通过（格式与可启动性待查） |
| 代表产物的格式识别 | 区分 ELF、DTB、FIT 与非自描述二进制 | ELF、DTB、FIT/DTB 容器与 empty 可识别；三个原始二进制的 `file` 猜测不作为结论 | 通过 |
| `u-boot-dtb.bin` 组成 | 验证是否为裸程序与 DTB 的拼接 | 长度、前缀和后缀比较均匹配 | 通过 |
| 同尺寸产物内容关系 | 区分名称别名与不同封装 | `.bin` 两文件哈希相同；`.img` 两文件哈希相同；两组哈希不同 | 通过（`.img` 封装待识别） |
| 文档列出的 RK3588 EVB 文件 | 两个精确路径均存在并可计算哈希 | 两个路径均输出 `missing:` | 不通过（版本/命名漂移待查） |
| 当前 RK3588 外部载荷候选 | 获得 BL31/DDR 文件的实际清单 | 1 个 BL31、4 个按频率/变体区分的 DDR TPL | 通过（选择依据待查） |

## 结论

已获得并验证一个干净的上游 U-Boot `v2026.07` 学习基线。它可用于阅读源码和之后的主机构建尝试；不应据此推断其与 R1 厂商启动布局、设备树或烧录格式兼容。EVB 候选的串口和 FIT/SPL 特征与当前板卡的部分观测一致，但根节点已直接确认其默认 DTS 为 EVB1 V10，而本板运行时 DTS 为 RK3588S EVB4 LP4X V10；两者仅共享通用 SoC 回退兼容项，不能直接构建或烧录。当前 U-Boot 源码布局中，EVB1 完整 DTS 位于 `dts/upstream/`，U-Boot 专用 `.dtsi` 位于 `arch/arm/dts/`；基础 DTS 已显示其继承 `rk3588.dtsi` 并添加 EVB1 板级内容，U-Boot 片段则添加 SPL boot order。属性的通用语法、Rockchip v2026.07 的读取/回退路径，以及来源/映射函数入口均已确认；不继续深挖函数实现，保留入口供后续项目需要时回溯。更正后的 defconfig 已生成并验证为隔离输出目录的 57552 字节普通 `.config`；所查设备树、串口及 FIT/SPL/ATF 项与 defconfig 一致。两行 Python 3 API 补丁已使构建越过 pylibfdt；而 `CONFIG_ARM64`、`CONFIG_ROCKCHIP_EXTERNAL_TPL`、`CONFIG_SPL_ATF` 均为 `y`，故 Binman 要求外部 TPL 与 BL31 是本次 EVB 配置的可解释结果。上游文档已给出相应 RK3588 EVB 的 BL31、TPL 文件名和构建变量形式；当前 rkbin `master` 已核验为同一远端顶端，但不保留文档点名的精确文件，故版本/命名漂移已确认。任何候选均未被选用或传入构建。AArch64 目标是否已部分编译及最终镜像状态均未盘点，开发板未被访问。

## 关联知识与问题

- 源码资源：[U-Boot v2026.07 上游源码](../resource/u-boot-v2026-07-upstream-source.md)。
- 当前厂商启动证据：[FIT 笔记](../note/uboot-fit-image.md)。

## 后续行动

- [x] 读取最终 `.config` 的外部 TPL、BL31/ATF 和 AArch64 条件项，确认本次 EVB 构建为什么包含这些 Binman 节点。
- [x] 读取上游文档中 RK3588 专用的 `BL31`/`ROCKCHIP_TPL` 示例；不下载、不烧录。
- [x] 对比当前 EVB1 默认设备树与 R1 运行时 EVB4 LP4X 标识，明确该示例不能直接作为 R1 输入选择依据。
- [x] 记录当前 `rkbin` HEAD 的作者时间与浅克隆状态；本地无法追溯旧历史。
- [x] 对比远端 `master` 与本地 HEAD，确认同为 `ecb4fcbe...`，提交者时间为 `2025-12-30T19:41:07+08:00`；当前资源与文档精确文件存在漂移。
- [x] 在不选择当前候选的前提下，追溯历史提交并定位文档点名的旧资源引入/替换提交。
- 未用树对象验证 `27d8...` 是否同时包含文档列出的 v1.33 BL31 和 v1.09 DDR TPL 路径：学习者判断继续追溯对当前目标价值不足，停止于已定位的版本演进结论。
