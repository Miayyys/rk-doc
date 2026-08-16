---
title: "EXP-20260812-001 定位 R1 运行中内核的构建配置"
type: experiment
status: verified
created: 2026-08-12
updated: 2026-08-12
tags: [rk3588, r1, linux, kernel, config]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[status/current]]"
---

# EXP-20260812-001 定位 R1 运行中内核的构建配置

## 目标

确认能否取得当前正在运行的厂商 Linux 内核配置，为后续解释设备树节点为何能匹配到 `eth0` 的驱动提供事实依据。

## 环境与前置条件

- 执行端：R1 目标 Linux 的 root Shell。
- 内核：Linux `5.10.110`、`aarch64`；完整构建标识见[EXP-20260807-002](exp-20260807-002-boot-linux-via-debug-uart.md)。
- 已知链路：`eth0` 已关联到 `fe1c0000.ethernet` 平台设备和 `rk_gmac-dwmac` 驱动；见[设备树笔记](../note/device-tree-model-and-compatible.md)。

## 风险与恢复

- 影响范围：仅读取 procfs 与 `/boot` 的候选配置路径。
- 备份：不需要。
- 恢复方法：不修改系统，无恢复操作。

## 步骤与证据

### 步骤 1：检查运行时内核配置的常见导出路径

目的：确认内核是否启用了 `IKCONFIG_PROC` 一类的运行时配置导出，或在 `/boot` 保留同版本配置副本。预期至少列出一个存在路径；无输出不代表内核无配置，只代表这两个常见入口不可用。

```sh
ls -l /proc/config.gz /boot/config-$(uname -r) 2>/dev/null
```

实际输出（学习者提供）：

```text
-r--r--r-- 1 root root 41579 Nov 22 04:57 /proc/config.gz
```

观察：`/proc/config.gz` 存在、可由 root 读取且为 41579 字节。它是当前运行内核导出的**内核构建配置**，不是 U-Boot 的 `.config`：U-Boot 在 Linux 内核之前运行，且其配置只影响 U-Boot 自身的功能和产物；此文件由已启动的 Linux 内核在 procfs 中导出，将用于核对 Linux 网络/平台驱动选项。显示的时间字段来源未验证，不能视为内核或配置构建时间。

### 步骤 2：读取当前网卡驱动的相关内核选项

目的：将已观察的 `st_gmac`/`rk_gmac-dwmac` 运行时名称，与当前内核实际编入的驱动配置对应起来。预期相关项为 `=y`、`=m` 或未启用；缺少某个名称不代表故障。

```sh
zcat /proc/config.gz | grep -E '^(CONFIG_(NET_VENDOR_STMICRO|STMMAC_ETH|DWMAC_ROCKCHIP|NET_VENDOR_ROCKCHIP|ROCKCHIP_GMAC)|# CONFIG_(NET_VENDOR_STMICRO|STMMAC_ETH|DWMAC_ROCKCHIP|NET_VENDOR_ROCKCHIP|ROCKCHIP_GMAC) is not set)'
```

实际输出（学习者提供）：

```text
CONFIG_NET_VENDOR_STMICRO=y
CONFIG_STMMAC_ETH=y
CONFIG_STMMAC_ETHTOOL=y
CONFIG_DWMAC_ROCKCHIP=y
CONFIG_DWMAC_ROCKCHIP_TOOL=y
```

观察：四个驱动相关选项均为 `=y`，表示它们编进当前 Linux 内核映像，而非以可独立加载的 `.ko` 模块形式提供。`NET_VENDOR_STMICRO` 是包含 Synopsys/STMicro 网络驱动选项的配置菜单；`STMMAC_ETH` 对应 Synopsys DesignWare MAC（STMMAC）核心；`STMMAC_ETHTOOL` 解释该核心提供 ethtool 相关接口；`DWMAC_ROCKCHIP` 是 Rockchip 平台的 DWMAC 支持。它们与设备树中的 `snps,dwmac-4.20a`、`rockchip,rk3588-gmac` 以及运行时的 `st_gmac`、`rk_gmac-dwmac` 名称相互一致。`DWMAC_ROCKCHIP_TOOL=y` 的具体用户接口和作用范围尚未读取配置帮助或源码，不能仅凭名称断言其用途。

### 步骤 3：读取 GMAC 平台驱动的启动 probe 日志

目的：将内核配置、设备树节点与实际初始化过程关联起来。预期出现平台设备名、MAC 核心能力、PHY 绑定和链路状态中的部分或全部；日志缺失不单独否定 sysfs 已验证的绑定关系。

```sh
dmesg | grep -Ei 'rk_gmac|stmmac|dwmac|fe1c0000|eth0'
```

实际输出（学习者提供）：

```text
[    1.802451] rk_gmac-dwmac fe1c0000.ethernet: IRQ eth_lpi not found
[    1.802553] rk_gmac-dwmac fe1c0000.ethernet: no regulator found
[    1.802558] rk_gmac-dwmac fe1c0000.ethernet: clock input or output? (input).
[    1.802563] rk_gmac-dwmac fe1c0000.ethernet: TX delay(0x44).
[    1.802567] rk_gmac-dwmac fe1c0000.ethernet: RX delay(0x18).
[    1.802574] rk_gmac-dwmac fe1c0000.ethernet: integrated PHY? (no).
[    1.802579] rk_gmac-dwmac fe1c0000.ethernet: cannot get clock mac_clk_rx
[    1.802583] rk_gmac-dwmac fe1c0000.ethernet: cannot get clock mac_clk_tx
[    1.802595] rk_gmac-dwmac fe1c0000.ethernet: cannot get clock clk_mac_speed
[    1.802598] rk_gmac-dwmac fe1c0000.ethernet: clock input from PHY
[    1.802814] rk_gmac-dwmac fe1c0000.ethernet: init for RGMII_RXID
[    1.802873] rk_gmac-dwmac fe1c0000.ethernet: User ID: 0x30, Synopsys ID: 0x51
[    1.802878] rk_gmac-dwmac fe1c0000.ethernet:         DWMAC4/5
[    1.802883] rk_gmac-dwmac fe1c0000.ethernet: DMA HW capability register supported
[    1.802886] rk_gmac-dwmac fe1c0000.ethernet: RX Checksum Offload Engine supported
[    1.802890] rk_gmac-dwmac fe1c0000.ethernet: TX Checksum insertion supported
[    1.802894] rk_gmac-dwmac fe1c0000.ethernet: Wake-Up On Lan supported
[    1.802917] rk_gmac-dwmac fe1c0000.ethernet: TSO supported
[    1.802921] rk_gmac-dwmac fe1c0000.ethernet: Enable RX Mitigation via HW Watchdog Timer
[    1.802926] rk_gmac-dwmac fe1c0000.ethernet: Enabled Flow TC (entries=2)
[    1.802930] rk_gmac-dwmac fe1c0000.ethernet: TSO feature enabled
[    1.802934] rk_gmac-dwmac fe1c0000.ethernet: Using 32 bits DMA width
[    8.613047] rk_gmac-dwmac fe1c0000.ethernet eth0: PHY [stmmac-1:00] driver [RTL8211F Gigabit Ethernet] (irq=POLL)
[    8.616080] dwmac4: Master AXI performs any burst length
[    8.616106] rk_gmac-dwmac fe1c0000.ethernet eth0: No Safety Features support found
[    8.616126] rk_gmac-dwmac fe1c0000.ethernet eth0: IEEE 1588-2008 Advanced Timestamp supported
[    8.616406] rk_gmac-dwmac fe1c0000.ethernet eth0: registered PTP clock
[    8.616743] rk_gmac-dwmac fe1c0000.ethernet eth0: configuring for phy/rgmii-rxid link mode
[   12.713274] rk_gmac-dwmac fe1c0000.ethernet eth0: Link is Up - 1Gbps/Full - flow control rx/tx
[   12.713342] IPv6: ADDRCONF(NETDEV_CHANGE): eth0: link becomes ready
[   23.857603] rk_gmac-dwmac fe1c0000.ethernet eth0: Link is Down
[   28.929672] rk_gmac-dwmac fe1c0000.ethernet eth0: Link is Up - 1Gbps/Full - flow control rx/tx
```

观察：`rk_gmac-dwmac fe1c0000.ethernet` 与先前 sysfs 的平台设备/驱动路径直接一致。日志依次显示：驱动读取 RGMII 接口、TX/RX 延迟和时钟方向等板级参数；识别 Synopsys DWMAC4/5 核心及 DMA、校验和卸载、TSO 等硬件能力；绑定外置 `RTL8211F Gigabit Ethernet` PHY；注册 IEEE 1588 PTP 时钟；最后将 `eth0` 配置为 `rgmii-rxid` 并协商到 1 Gbps 全双工。后续 Link Down/Up 是链路状态变化记录，单凭这段日志不能确定其原因；最后一条显示链路恢复为 Up。

`IRQ eth_lpi not found`、`no regulator found` 和三个 `cannot get clock ...` 是 probe 期间未找到的资源项。它们不应被忽略，但本次 probe 后续完成、PHY 已绑定且链路已协商成功，因此不能据此断言当前网卡初始化失败；这些项究竟是可选资源、设备树缺项还是存在功能限制，需要结合设备树属性和驱动源码后再判断。

### 步骤 4：将 FIT 内 Linux FDT 持久化并复核哈希

目的：避免将可复查的 DTB/DTS 只放在重启会清空的主机 `/tmp`，并确认重新读取的 `p3` FIT 内 `fdt` 仍是先前已验证的载荷。预期 SHA-256 与 FIT 元数据中的 `fdt` 哈希一致。

执行端：Arch 主机 fish Shell。`dd` 在板端仅读取 `/dev/mmcblk0p3` 的已知 FDT 范围；重定向只写入 Git 忽略的本机 `build/local/`，不改动 eMMC。

```fish
set artifact_dir /home/loser/Study/rk3588/build/local/r1-20260812
mkdir -p $artifact_dir
ssh -o ConnectTimeout=5 root@10.42.0.192 'dd if=/dev/mmcblk0p3 bs=1 skip=2048 count=147826 status=none' > $artifact_dir/linux-fit-fdt.dtb
sha256sum $artifact_dir/linux-fit-fdt.dtb
```

实际输出（学习者提供）：

```text
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546
```

观察：该值与 p3 FIT 元数据中 `fdt` 子镜像声明、以及先前对同一偏移/长度范围的校验值完全一致。因而可以将 `build/local/r1-20260812/linux-fit-fdt.dtb` 作为该次重新提取的、可跨主机重启保留的分析副本；它仍是 Linux FIT 内的原始 FDT，并非运行时已被 U-Boot 补充过的 FDT。

### 步骤 5：把 GMAC DTS 属性与 probe 日志逐项对照

目的：检验前一步日志中由驱动报告的板级参数，是否确实来自 FIT 内 Linux FDT，而不是只按名称推断。预期可以看到接口模式、时钟方向与延时属性。

执行端：Arch 主机 fish Shell。先以 `dtc` 只读反编译持久化 DTB，再读取 GMAC 节点；不接触板端存储。

```fish
set artifact_dir /home/loser/Study/rk3588/build/local/r1-20260812
dtc -I dtb -O dts -o $artifact_dir/linux-fit-fdt.dts $artifact_dir/linux-fit-fdt.dtb
rg -n -A 55 '^[[:space:]]*ethernet@fe1c0000[[:space:]]*\{' $artifact_dir/linux-fit-fdt.dts
```

实际输出节选（学习者提供）：

```text
4575-        phy-mode = "rgmii-rxid";
4576-        clock_in_out = "input";
4577-        snps,reset-gpio = <0xfb 0x0f 0x01>;
4578-        snps,reset-active-high;
4579-        snps,reset-delays-us = <0x00 0x4e20 0x186a0>;
4580-        pinctrl-names = "default";
4581-        pinctrl-0 = <0xfc 0xfd 0xfe 0xff 0x100 0x101 0x102>;
4582-        tx_delay = <0x44>;
4583-        rx_delay = <0x18>;
4584-        phy-handle = <0x103>;
```

观察：四项已可直接对应启动日志：`phy-mode = "rgmii-rxid"` 对应 `init for RGMII_RXID`；`clock_in_out = "input"` 对应 `clock input from PHY`；`tx_delay = <0x44>` 与 `rx_delay = <0x18>` 分别对应日志中的 `TX delay(0x44)`、`RX delay(0x18)`。这说明这些值来自该 DTB 的 GMAC 节点，而非仅是驱动默认值的假设。

`phy-handle = <0x103>` 是到另一个设备树节点的内部引用，尚未定位目标；它不是硬件地址。`snps,reset-gpio`、复位极性/延时和 `pinctrl-0` 也已存在，但三元组中每一单元的精确含义、复位时序和各 pinctrl 标签均未读取 binding 或目标节点，暂不解释。

### 步骤 6：解析 GMAC 到 PHY 的设备树引用

目的：确定 `phy-handle` 实际指向的节点，并区分 PHY 的通用设备树描述、MDIO 地址与运行时识别出的具体芯片型号。预期 `phandle` 的目标位于 GMAC 的 MDIO 子总线下，且 `reg` 与启动日志中的 PHY 地址一致。

执行端：Arch 主机 fish Shell；只读取步骤 5 生成的 DTS。

```fish
set artifact_dir /home/loser/Study/rk3588/build/local/r1-20260812
rg -n -B 18 -A 28 'phandle = <0x103>;' $artifact_dir/linux-fit-fdt.dts
```

实际输出（学习者提供）：

```text
phy-handle = <0x103>;

mdio {
        compatible = "snps,dwmac-mdio";
        #address-cells = <0x01>;
        #size-cells = <0x00>;

        phy@0 {
                compatible = "ethernet-phy-ieee802.3-c22";
                reg = <0x00>;
                phandle = <0x103>;
        };
};
```

观察：GMAC 的 `phy-handle = <0x103>` 精确指向其 `mdio` 子节点内的 `phy@0`。MDIO 子节点的地址/长度单元分别为 1/0，`phy@0` 的 `reg = <0x00>` 因而表示 MDIO 地址 0，不是内存映射寄存器地址。先前日志 `PHY [stmmac-1:00]` 中的 `:00` 与此地址相符，闭合了“GMAC → MDIO 总线 → 地址 0 的 PHY”这段关系。

`ethernet-phy-ieee802.3-c22` 只说明该节点按 IEEE 802.3 Clause 22 的通用 PHY 方式描述，不含 RTL8211F 的厂商型号。具体型号来自驱动在地址 0 读取 PHY ID 后的运行时识别，且本次启动日志已报告 `RTL8211F Gigabit Ethernet`。因此不能倒过来把 DTS 的通用 compatible 当作该型号的直接声明。

### 步骤 7：用运行时 `phydev` 验证 MDIO 地址与 PHY ID

目的：从已运行的内核对象读取 PHY 所在总线、地址和硬件标识，交叉验证设备树的 `mdio/phy@0`。预期 sysfs 路径末尾含地址 `:00`，并返回一个十六进制 PHY ID。

执行端：R1 目标 Linux 的 root Shell。仅读取 sysfs，不改变网络配置、eMMC 或驱动。

```sh
readlink -f /sys/class/net/eth0/phydev
cat /sys/class/net/eth0/phydev/phy_id
```

实际输出（学习者提供）：

```text
/sys/devices/platform/fe1c0000.ethernet/mdio_bus/stmmac-1/stmmac-1:00
0x001cc916
```

观察：路径再次保留了 `fe1c0000.ethernet` 平台设备，并显示该 MAC 驱动建立的 MDIO 总线 `stmmac-1`。末尾 `stmmac-1:00` 表示该总线上的地址 0，和 DTS 的 `phy@0`、`reg = <0x00>` 完全一致。`0x001cc916` 是内核从该地址读取出的 PHY ID 原始值；它与同次启动日志对该对象的 `RTL8211F Gigabit Ethernet` 识别共同构成运行时证据。此处尚未拆解 PHY ID 的 OUI、型号和修订字段，也不以字符串/路径名称替代硬件 ID。

### 步骤 8：读取已绑定的 PHY 驱动

目的：确认 PHY ID 识别后实际绑定的是哪一个 PHY 驱动，并与 MAC 平台驱动区分。预期路径位于 `mdio_bus` 的驱动目录；它不应与 `rk_gmac-dwmac` 的 platform 驱动路径混同。

执行端：R1 目标 Linux 的 root Shell；仅读取 sysfs。

```sh
readlink -f /sys/class/net/eth0/phydev/driver
```

实际输出（学习者提供）：

```text
/sys/bus/mdio_bus/drivers/RTL8211F Gigabit Ethernet
```

观察：该链接证明地址 0 的 `phydev` 已绑定名为 `RTL8211F Gigabit Ethernet` 的 MDIO PHY 驱动。DTS 没有直接用该字符串选择驱动：它只提供 MDIO 总线、地址 0、通用 Clause 22 PHY 描述和与 GMAC 的 `phy-handle` 连接。内核在扫描该地址时读取 PHY ID `0x001cc916`，再由 PHY 子系统在已注册的 PHY 驱动中按 ID/掩码匹配并绑定此驱动；这也解释了 sysfs 驱动路径与 DTS `compatible` 文本不同。

该驱动的可用性仍来自内核构建配置（通常以 Realtek PHY 配置项控制），但仅凭链接不能判断它是内建还是模块；需读取当前内核配置确认。

### 步骤 9：确认 Realtek PHY 驱动的构建方式

目的：核对当前运行内核是否将 Realtek PHY 驱动内建，避免把已绑定的运行时驱动误认为必然是可加载模块。

执行端：R1 目标 Linux 的 root Shell；只解压读取 procfs 导出的配置。

```sh
zcat /proc/config.gz | grep -E '^(CONFIG_REALTEK_PHY=|# CONFIG_REALTEK_PHY is not set)'
```

实际输出（学习者提供）：

```text
CONFIG_REALTEK_PHY=y
```

观察：`=y` 表示 Realtek PHY 驱动代码已编进当前内核映像，而不是通过独立 `.ko` 模块在启动后加载。这解释了内核能够在扫描到地址 0 的 PHY ID 后立即绑定 RTL8211F 驱动，但不“设置”或改变 PHY ID。

### 步骤 10：确认 GPIO 核心与 Rockchip GPIO 控制器的构建方式

目的：在已建立的“设备树 GPIO 控制器 → 运行时 gpiochip”链路上，确认当前内核是否同时包含 GPIO 核心框架和 Rockchip 控制器驱动。预期配置项为 `=y`、`=m` 或未启用；它们不提供某个物理排针的安全接线结论。

执行端：R1 目标 Linux 的 root Shell；只解压读取 procfs 导出的配置。

```sh
zcat /proc/config.gz | grep -E '^CONFIG_(GPIOLIB|GPIO_ROCKCHIP)='
```

实际输出（学习者提供）：

```text
CONFIG_GPIOLIB=y
CONFIG_GPIO_ROCKCHIP=y
```

观察：`CONFIG_GPIOLIB=y` 表明 Linux 通用 GPIO 子系统已直接编进当前内核；`CONFIG_GPIO_ROCKCHIP=y` 表明 Rockchip GPIO 控制器驱动也直接编进内核映像。二者与此前观察到的 5 个 `rockchip,gpio-bank` 设备树节点、`/dev/gpiochip0` 至 `/dev/gpiochip4` 以及对应平台设备路径相容。`=y` 的含义是“内建”，而非运行时动态加载的 `.ko` 模块；它不说明控制器已为全部引脚建立安全的 GPIO 复用，也不说明可直接操作任意排针。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 当前内核配置可从常见路径取得 | `/proc/config.gz` 或 `/boot/config-$(uname -r)` 至少一个存在 | `/proc/config.gz` 存在且可读 | 通过 |
| GMAC 驱动相关配置 | STMMAC/DWMAC 相关项可从运行内核配置读取 | `STMMAC_ETH`、`STMMAC_ETHTOOL`、`DWMAC_ROCKCHIP`、`DWMAC_ROCKCHIP_TOOL` 均为 `y` | 通过 |
| GMAC probe 与链路建立 | 平台设备实际匹配驱动、绑定 PHY 并注册接口 | `rk_gmac-dwmac fe1c0000.ethernet` 绑定 RTL8211F PHY，`eth0` 链路协商为 1 Gbps Full | 通过 |
| FIT 内 FDT 的持久化副本 | 重提取哈希与已验证 FDT 载荷一致 | SHA-256 为 `abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546` | 通过 |
| DTS 参数与 probe 日志对应 | 关键日志参数能在 GMAC 节点找到 | RGMII RXID、输入时钟、TX/RX 延时均逐项一致 | 通过 |
| GMAC 至 PHY 的引用 | `phy-handle` 指向可解释的 PHY 节点 | 指向 `mdio/phy@0`，`reg = <0x00>` 与日志 `stmmac-1:00` 对应 | 通过 |
| 运行时 PHY 对象 | sysfs 路径与 PHY ID 可读 | `stmmac-1:00`，PHY ID 为 `0x001cc916` | 通过 |
| PHY 驱动绑定 | `phydev` 有 MDIO PHY 驱动链接 | 指向 `RTL8211F Gigabit Ethernet` | 通过 |
| Realtek PHY 驱动构建方式 | 配置可说明内建或模块 | `CONFIG_REALTEK_PHY=y` | 内建 |
| GPIO 核心与 Rockchip 控制器构建方式 | 配置可说明内建或模块 | `CONFIG_GPIOLIB=y`、`CONFIG_GPIO_ROCKCHIP=y` | 均内建 |

## 结论

已获得当前运行 Linux 内核的配置入口，并确认 R1 运行内核内建 STMMAC、Rockchip DWMAC、Realtek PHY、Linux GPIO 核心和 Rockchip GPIO 控制器支持。启动日志进一步确认 `rk_gmac-dwmac` 对 `fe1c0000.ethernet` 完成 probe、绑定 RTL8211F 外置 PHY、注册 PTP 时钟并使 `eth0` 协商到 1 Gbps 全双工。持久化的 FIT 内 Linux FDT 又将 RGMII 模式、时钟方向、TX/RX 延时以及 `phy-handle → mdio/phy@0` 与该 probe 日志逐项闭合；运行时 `phydev` 进一步验证它实际成为 `stmmac-1:00`，读取的 PHY ID 为 `0x001cc916`，并绑定内建的 RTL8211F PHY 驱动。GPIO 配置与设备树中 `rockchip,gpio-bank`、运行时 SoC gpiochip 的事实相容。这形成“配置存在 → FDT 参数 → 平台设备/驱动匹配 → 运行时接口”的两条最小证据链。资源缺失日志的具体影响、PHY ID 的位字段解码、GPIO 驱动的 probe 代码和引脚/复位配置仍待验证。不能以主机上游 U-Boot 的 `.config` 或其他内核版本的默认配置替代本配置。

## 关联知识与问题

- 支持的知识点：设备树节点、平台设备、驱动与网络接口之间的关系。
- 关联问题：无。

## 后续行动

- [x] 读取 `STMMAC`、Rockchip DWMAC 和相关厂商网络配置项；相关核心/平台支持均内建（`=y`）。
- [x] 读取内核启动日志中 GMAC 的 probe 消息，确认驱动、PHY、PTP 与链路建立。
- [x] 对照 FIT 内 Linux DTS 的 `ethernet@fe1c0000` 属性，定位与日志中 RGMII、延迟和时钟方向对应的字段。
- [x] 解析 `phy-handle = <0x103>` 的目标节点，确认其为 `mdio/phy@0`，MDIO 地址为 0。
- [x] 从运行时 `phydev` 路径读取 PHY ID，确认 `stmmac-1:00` 与 DTS 地址 0 一致，PHY ID 为 `0x001cc916`。
- [x] 读取 PHY 的已绑定驱动链接，确认其为 `RTL8211F Gigabit Ethernet`，并与 MAC 平台驱动区分。
- [x] 从运行内核配置读取 Realtek PHY 选项，确认 `CONFIG_REALTEK_PHY=y`，驱动内建。
- [x] 从运行内核配置读取 GPIO 核心与 Rockchip GPIO 选项，确认 `CONFIG_GPIOLIB=y`、`CONFIG_GPIO_ROCKCHIP=y`，二者均内建。
- [ ] 如需继续，读取与当前内核版本匹配的 PHY 驱动源码，验证 PHY ID 匹配表和 ID 位字段处理；当前尚未取得对应厂商内核源码。
