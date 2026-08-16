---
title: "设备树的 model 与 compatible"
type: note
status: verified
created: 2026-08-07
updated: 2026-08-12
tags: [rk3588, devicetree, boot, kernel]
aliases: ["DT", "DTB", "FDT", "设备树模型", "设备树兼容列表"]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[experiment/exp-20260812-001-locate-running-kernel-config]]"
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

### 根节点、单元数与别名

根节点 `/` 是整棵设备树的入口。其子节点中的 `#address-cells` 和 `#size-cells` 规定子节点属性（常见为 `reg`）如何编码地址和长度；本板根节点均为 `<0x02>`，即各使用两个 32 位单元。它们不是“有两个地址”或“内存为 2 字节”，而是二进制字段的表示规则。

`aliases` 是短名称到实际节点路径的映射。例如本板 FIT 内 Linux DTB 中的 `ethernet1 = "/ethernet@fe1c0000"` 指向以太网节点。别名用于方便引用，不创建硬件，也不表示内核已经为该节点加载驱动。

节点名中的 `@` 后通常是单元地址。例如 `ethernet@fe1c0000` 的 `reg = <0x00 0xfe1c0000 0x00 0x10000>` 按根节点 2/2 单元规则表示寄存器地址 `0xfe1c0000`、范围 `0x10000`（64 KiB）。该节点的 `compatible` 从 Rockchip GMAC 回退到 Synopsys DesignWare MAC 4.20a；`status = "okay"` 表示设备树启用该节点，但不等同于驱动已绑定或网线已连通。

要验证驱动绑定，应查看运行时证据而非只看 DTS。例如本板 `ethtool -i eth0` 报告 `driver: st_gmac`，说明内核已为该网络接口绑定此驱动。它与 GMAC 节点的存在是一条相关证据链，但要追溯到精确的设备—驱动匹配规则，还应检查 sysfs 驱动链接或模块别名。空的 `bus-info` 或 `firmware-version` 不单独表示故障；有些驱动不会通过 ethtool 填充这些字段。

本板进一步验证：`/sys/class/net/eth0/device/driver` 解析为 `/sys/bus/platform/drivers/rk_gmac-dwmac`。该链接从网络接口的底层设备指向已绑定的平台驱动。`ethtool` 报告的 `st_gmac` 与 sysfs 的 `rk_gmac-dwmac` 是厂商内核在不同接口呈现的名称；不要仅因名称不同就判断驱动冲突。要追查它们与具体源码和 `compatible` 表的精确关系，仍需读取模块元数据或内核源码。

`/sys/class/net/eth0/device` 进一步解析为 `/sys/devices/platform/fe1c0000.ethernet`。其中 `fe1c0000` 与 DTS 的节点名 `ethernet@fe1c0000` 和 `reg` 起始地址一致，说明平台设备名称保留了该硬件实例的地址信息。结合平台驱动链接，可从一个 Linux 网络接口一路追到其底层平台设备和已绑定驱动。

最后，读取该设备的运行时 OF 节点：`tr '\0' '\n' < /sys/class/net/eth0/device/of_node/compatible`，输出 `rockchip,rk3588-gmac`、`snps,dwmac-4.20a`，与已反编译 GMAC 节点的 `compatible` 完全一致。至此，针对本板 `eth0` 的可观察证据链为：设备树 `compatible` → `fe1c0000.ethernet` 平台设备 → `rk_gmac-dwmac` 平台驱动 → `eth0` 网络接口。它验证了实际绑定关系；若要说明内核源码如何选择该驱动，仍需另查该内核的驱动匹配表。

### 设备树到网络接口的最小驱动模型

以下是理解当前实验所需的最小模型；通用过程由 Linux 设备树/驱动模型资料说明，`eth0` 链条中的具体名称已由本板实验验证。

1. **设备树节点**描述硬件资源或硬件功能单元，例如 `ethernet@fe1c0000` 描述 SoC 内的 GMAC 控制器。节点不必总是一块独立的实体设备：也可能描述总线、时钟或容器。
2. **`compatible`**是驱动匹配的依据。平台驱动声明它支持的字符串，内核据此选择合适的驱动；具体到通用的多项列表提供回退匹配。
3. **平台设备**是内核从设备树节点创建的内核设备对象，主要参与内核的设备/驱动模型，而不是直接供用户程序操作。`/sys/devices/platform/fe1c0000.ethernet` 是其在 sysfs 中可观察到的表示。
4. **驱动**在绑定后取得 `reg`、`interrupts`、`clocks`、`resets`、`power-domains`、`phy-mode` 等资源并执行初始化。`status = "okay"` 只表示设备树启用该节点、允许尝试匹配；它不保证 probe 成功，更不保证网线、IP 或 DHCP 正常。
5. **网络接口**是 GMAC 驱动成功初始化后向网络子系统注册的结果；本板为 `eth0`。用户通过该接口配置地址和传输数据，而不是直接操作平台设备。

设备树匹配还需要满足一个更早的条件：对应驱动必须被内核编译进来或能被加载。本板运行内核的 `/proc/config.gz` 已确认 `CONFIG_STMMAC_ETH=y`、`CONFIG_DWMAC_ROCKCHIP=y`，即 Synopsys MAC 核心与 Rockchip 平台支持均内建；这解释了内核具备处理该节点的代码。配置为 `=y` 不能单独证明实例已工作，仍需与 `compatible`、sysfs 驱动链接和 `eth0` 注册等运行时证据一起使用。完整配置读取及 probe 日志见[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。

本板的启动日志补齐了这条链路中的 probe 过程：`rk_gmac-dwmac fe1c0000.ethernet` 读取 RGMII/延迟/时钟方向等参数，识别 DWMAC4/5 核心，绑定 `RTL8211F Gigabit Ethernet` 外置 PHY，注册 PTP 时钟，最终使 `eth0` 以 RGMII RXID、1 Gbps 全双工工作。这些状态均来自本次启动日志；其中“某些 IRQ、clock 或 regulator 未找到”的日志是否只是可选资源缺失，仍需按具体 DTS 属性与驱动代码验证，不能仅凭最终链路正常就忽略。

对 FIT 内 Linux FDT 的持久化副本反编译后，GMAC 节点中的 `phy-mode = "rgmii-rxid"`、`clock_in_out = "input"`、`tx_delay = <0x44>`、`rx_delay = <0x18>`，与该启动日志的 RGMII RXID、来自 PHY 的输入时钟和 `0x44`/`0x18` 延时逐项一致。这是“设备树属性被驱动读取”的本板证据。节点的 `phy-handle = <0x103>` 指向其 `mdio` 子节点内的 `phy@0`；该节点 `reg = <0x00>` 表示 PHY 位于 MDIO 地址 0，与日志和运行时 sysfs 路径 `stmmac-1:00` 的地址部分一致。`ethernet-phy-ieee802.3-c22` 是 Clause 22 通用 PHY 描述，不能单独证明型号；运行时从该对象读取的 PHY ID 为 `0x001cc916`，并按该 ID 绑定 `RTL8211F Gigabit Ethernet` PHY 驱动。当前 `CONFIG_REALTEK_PHY=y`，故该驱动内建在当前内核。即：DTS 指出“在哪里、怎么连”，PHY 子系统借助真实 ID 决定“由哪个具体 PHY 驱动处理”；PHY ID 本身由芯片制造时固化，DTS、内核配置和驱动绑定都不设置它。PHY ID 位字段仍需单独学习。

本板已从运行时节点读取 `status`，实际输出为 `okay`。它与 `of_node/compatible`、sysfs 驱动链接和 `eth0` 同时存在，构成完整的成功样例；解释时仍应保持因果方向：`okay` 是允许内核尝试初始化的配置条件，已绑定驱动和已注册 `eth0` 才是初始化成功的观察证据。

```text
DTB 节点及 compatible
        ↓ 内核创建
平台设备
        ↓ 匹配并 probe
已绑定驱动
        ↓ 注册到子系统
eth0 网络接口

sysfs 软链接只展示这些内核对象已经形成的关系；它不是 DTS 创建的，也不参与匹配。
```

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

学习者已通过 SSH/SCP 将该文件复制到 Arch 主机的临时路径，并完成主机侧核对：大小仍为 151552 字节，`file` 的 FDT v17 头部字段一致，SHA-256 为 `51cb9beb30f4b6221d13aa8c85bef9d957cea86afd8c164dbb72c356205d068c`。这使该临时文件可作为本次启动运行时 FDT 的主机侧比较样本；它仍不证明 eMMC 中 FIT 的 `fdt` 是同一字节流。

当前板端 Shell 的 `command -v dtc` 没有输出，表明 `dtc` 不在当前 `PATH` 中。为保持板端环境基线稳定，应先在 Arch 主机确认或准备该工具，再决定是否需要复制运行时 FDT 进行反编译。

主机侧最初也没有可调用的 `dtc`，但随后 `dtc -v` 返回 `DTC v1.8.1`。`pacman -Qi dtc` 进一步确认已安装包为 `1:1.8.1-1`，于 2026-08-07T20:53:39+08:00 单独指定安装，且有数字签名验证。已验证范围与未验证操作见[dtc 工具记录](../tool/dtc.md)。

### FIT `fdt` 与运行时 FDT 的已知关系

学习者已对 FIT 中 `fdt` 的完整范围（`p3` 偏移 `0x800`、长度 147826 字节）校验 SHA-256，结果匹配 FIT 声明。随后从同一范围执行 `strings` 筛选，得到根标识字符串：

```text
rockchip,rk3588s-evb4-lp4x-v10
rockchip,rk3588
Rockchip RK3588S EVB4 LP4X V10 Board
```

其中模型字符串前在 `strings` 输出中出现的单个 `7` 是相邻二进制字节被当作可打印字符显示，不是 `model` 文本的一部分；运行时 `model` 已独立读取过准确值。其余结果包含大量 `rockchip,rk3588-*` 外设节点的兼容字符串，这符合一个 RK3588 板级设备树包含时钟、USB、PCIe、显示、音频等多个硬件节点的结构。

**已验证**：FIT 中的 `fdt` 与运行时 FDT 具有相同的根 `model` 与根 `compatible` 文本。已在主机保存并核对 FIT `fdt` 副本：大小 147826 字节、SHA-256 `abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546`；运行时 FDT 为 151552 字节、SHA-256 `51cb9beb30f4b6221d13aa8c85bef9d957cea86afd8c164dbb72c356205d068c`。二者大小不同，因此不可能是同一份未修改的字节流；仍可能存在启动加载器修补、另一份 DTB 或其他构建差异。比较最早的二进制差异与反编译结果后才能继续收敛。

`cmp` 已返回 1。最早差异位于 FDT 头部 `totalsize` 字段，正好反映 147826 与 151552 的大小差异；后续早期差异也位于头部偏移/大小字段。头部不同并不能回答设备节点是否不同，因而下一步应将两份 DTB 都反编译为 DTS，再比较节点和属性语义。

DTS 比较确认运行时树新增或填充了 `/memreserve/`、根 `memory`、显示时序和 logo 属性、以太网 `local-mac-address`、`chosen` 启动参数，以及 DRM logo 的非零内存范围。设备序列号、MAC 与厂商配置内容不记录到仓库。**推测**：这些具有运行期特征的字段由启动加载器或更早固件在 DTB 交给内核前补充；尚无证据指定唯一写入组件或代码位置。

完整 DTS diff 为 73 行、5 个差异块，范围正好覆盖上述五类节点区域。这支持“FIT 基础 DTB 加有限运行期补充”的模型；它不证明补充者一定是 U-Boot，也不证明 FIT 是唯一的设备树输入来源。

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
9. 根节点的 `#address-cells`、`#size-cells` 定义子节点地址/长度的编码规则；`aliases` 只是到真实节点路径的快捷引用。
10. 本板 `eth0` 的运行时 `of_node/compatible` 与 GMAC DTS 节点完全一致，可用 sysfs 将设备树节点、平台设备、驱动和网络接口关联起来。

## 参考资料

- Linux Kernel Documentation, “Linux and the Devicetree”, https://docs.kernel.org/devicetree/usage-model.html，访问于 2026-08-07。
- Devicetree Specification, “Flattened Devicetree (DTB) Format”, https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html，访问于 2026-08-07。
- Arch Linux, “dtc package”, https://archlinux.org/packages/extra/x86_64/dtc/，访问于 2026-08-07。
