---
title: "EXP-20260807-002 通过 Debug UART 启动并登录 R1 Linux"
type: experiment
status: verified
created: 2026-08-07
updated: 2026-08-09
tags: [rk3588, r1, linux, boot, uart, devicetree]
related:
  - "[[experiment/exp-20260807-001-connect-debug-uart]]"
  - "[[status/current]]"
  - "[[issue/issue-20260807-001-maskrom-and-linux-boot]]"
  - "[[note/linux-kernel-command-line]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[note/r1-emmc-partition-layout]]"
  - "[[note/uboot-fit-image]]"
  - "[[tool/dtc]]"
  - "[[issue/issue-20260809-003-r1-dhcp-lease-missing]]"
  - "[[issue/issue-20260809-004-ufw-blocks-shared-nat-forward]]"
  - "[[issue/issue-20260809-005-r1-ssh-public-key-rejected]]"
---

# EXP-20260807-002 通过 Debug UART 启动并登录 R1 Linux

## 目标

验证 R1 是否能启动 Linux，并确认 Debug UART 可接收启动日志和提供交互 Shell。

## 环境与前置条件

- 通过 `/dev/ttyUSB0` 使用 `picocom` 监听 R1 Debug UART，参数为 1500000 baud、8N1、无流控。
- **已验证**：当前用户有串口访问权限，详见 [EXP-20260807-001](exp-20260807-001-connect-debug-uart.md)。
- 运行时设备树的 `model` 已识别为通用的 Rockchip EVB4 LP4X V10 名称；实际 DTB 文件、来源和其与 R1 实物板型的对应关系尚未确认。图形桌面是否实际出画尚未验证。

## 步骤与证据

### 步骤 1：重新上电并观察启动输出

动作：在 `picocom` 已打开时重新上电 R1。

实际输出（学习者提供的启动日志节选）：

```text
[   20.047897] rkcif-mipi-lvds1: rkcif_update_sensor_info: stream[1] get remote terminal sensor failed!
[   20.047903] rkcif_tools_id1: update sensor info failed -19
[   20.048067] rkcif-mipi-lvds1: rkcif_update_sensor_info: stream[2] get remote terminal sensor failed!
[   20.048073] rkcif_tools_id2: update sensor info failed -19

snap
root@R1:~#
```

观察：带秒数前缀的内核日志与可交互的 `root@R1:~#` 提示符表明 Linux 内核和用户空间已启动。

### 步骤 2：验证 Shell 交互

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
ls
```

实际输出：

```text
snap
```

观察：Shell 接受命令并返回目录内容，确认 UART 不仅能接收日志，也可进行交互。

### 步骤 3：识别运行中内核

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
uname -a
```

实际输出：

```text
Linux R1 5.10.110 #4 SMP Sun Sep 29 10:38:13 CST 2024 aarch64 aarch64 aarch64 GNU/Linux
```

观察：系统运行 Linux 5.10.110 内核，主机名为 `R1`，目标架构为 64 位 ARM（`aarch64`）。`#4` 和日期是内核构建标识，不代表本次启动时间。

### 步骤 4：识别根文件系统来源

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
findmnt -no SOURCE /
```

实际输出：

```text
/dev/mmcblk0p6
```

观察：根文件系统挂载自 MMC 子系统的第一个块设备 `mmcblk0` 的第 6 分区。`mmcblk` 同时可能表示 eMMC 或 TF/SD，不能只凭此命名确认物理介质类型。

### 步骤 5：确认 MMC 物理介质类型

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
cat /sys/block/mmcblk0/device/type
```

实际输出：

```text
MMC
```

观察：`MMC` 表示该设备使用 eMMC 的 MultiMediaCard 协议，而非 SD 卡协议；结合 R1 的板载 eMMC 规格，可确认根文件系统 `/dev/mmcblk0p6` 位于 eMMC。

### 步骤 6：记录登录欢迎信息中的发行版

实际输出（学习者提供的登录欢迎信息节选）：

```text
Welcome to Ubuntu 22.04 LTS (GNU/Linux 5.10.110 aarch64)

 * Documentation:  https://help.ubuntu.com
 * Management:     https://landscape.canonical.com
 * Support:        https://ubuntu.com/advantage
```

观察：欢迎信息表明用户空间为 Ubuntu 22.04 LTS。该信息不能独自判断是否安装或默认启动图形桌面。

### 步骤 7：确认 systemd 默认启动目标

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
systemctl get-default
```

实际输出：

```text
graphical.target
```

观察：系统默认目标为 `graphical.target`，表明该 Ubuntu 配置为默认启动图形会话，而不是仅启动多用户文本模式。此项不验证 HDMI 或其他显示设备是否实际成功出画。

### 步骤 8：读取内核启动参数

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
cat /proc/cmdline
```

实际输出：

```text
storagemedia=emmc androidboot.storagemedia=emmc androidboot.mode=normal  androidboot.verifiedbootstate=orange rw rootwait earlycon=uart8250,mmio32,0xfeb50000 console=ttyFIQ0 irqchip.gicv3_pseudo_nmi=0 root=PARTUUID=614e0000-0000
```

观察：启动参数包含 Android 兼容字段，但这不代表当前运行 Android；根文件系统已由登录欢迎信息和 eMMC 挂载证据确认是 Ubuntu。`root=PARTUUID=614e0000-0000` 是启动加载器交给内核的根分区标识，内核将其解析为当前的 `/dev/mmcblk0p6`。`console=ttyFIQ0` 与 `earlycon` 解释了 Debug UART 的内核日志来源。详细解释见[内核启动参数笔记](../note/linux-kernel-command-line.md)。

### 步骤 9：读取发行版标准身份文件

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
cat /etc/os-release
```

实际输出（学习者提供；退出码未记录）：

```text
PRETTY_NAME="Ubuntu 22.04 LTS"
NAME="Ubuntu"
VERSION_ID="22.04"
VERSION="22.04 (Jammy Jellyfish)"
VERSION_CODENAME=jammy
ID=ubuntu
ID_LIKE=debian
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
UBUNTU_CODENAME=jammy
```

观察：`/etc/os-release` 确认当前用户空间为 Ubuntu 22.04 LTS，代号 Jammy Jellyfish，并表明其发行版家族与 Debian 兼容。该结果确认的是用户空间发行版，不能据此判断具体桌面环境或镜像类别。

### 步骤 10：读取运行时设备树模型

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
tr -d '\0' < /proc/device-tree/model; printf '\n'
```

实际输出（学习者提供；退出码未记录）：

```text
Rockchip RK3588S EVB4 LP4X V10 Board
```

观察：当前内核加载的设备树 `model` 属性声明的是 Rockchip RK3588S EVB4 LP4X V10 评估板风格的名称。其中 `RK3588S` 是 SoC 型号，`LP4X` 表示内存类型；该字符串未出现 `youyeetoo R1`。这确认当前运行时 DTB 使用了通用/参考板命名，但**不能单凭此项断定实物不是 R1**；厂商镜像可能复用参考板设备树。需要继续检查 `compatible` 属性和 DTB 来源。

### 步骤 11：读取运行时设备树兼容列表

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
tr '\0' '\n' < /proc/device-tree/compatible
```

实际输出（学习者提供；退出码未记录）：

```text
rockchip,rk3588s-evb4-lp4x-v10
rockchip,rk3588
```

观察：根节点的 `compatible` 是按“最具体到最通用”排列的字符串列表。第一行声明当前 DTB 最具体匹配 Rockchip RK3588S EVB4 LP4X V10；第二行声明它还兼容通用 RK3588 平台，以便内核选择通用支持代码。两行均未出现 R1 商品名，因此它们描述的是当前加载 DTB 的兼容关系，不是读取 PCB 丝印的结果。

### 步骤 12：确认内核暴露运行时 FDT 二进制

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
ls -l /sys/firmware/fdt
```

实际输出（学习者提供；退出码未记录）：

```text
-r-------- 1 root root 151552 Nov 22 05:45 /sys/firmware/fdt
```

观察：`/sys/firmware/fdt` 存在且仅允许 root 读取，大小为 151552 字节。它是内核导出的运行时 Flattened Device Tree（FDT）二进制，可用于后续研究当前实际使用的设备树，而不必先读取 eMMC 分区中的文件。`ls` 显示的 `Nov 22 05:45` 时间字段来源和含义尚未验证，不能据此推断 DTB 的构建时间、镜像制作时间或板卡当前时间。

### 步骤 13：验证运行时 FDT 格式头

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
file /sys/firmware/fdt
```

实际输出（学习者提供；退出码未记录）：

```text
/sys/firmware/fdt: Device Tree Blob version 17, size=151552, boot CPU=0, string block size=7470, DT structure block size=141980
```

观察：`file` 成功解析 FDT 头，确认该文件是有效的 Device Tree Blob。`version 17` 是 DTB **二进制布局**版本，不是 Linux 内核版本、SoC 版本、板型版本或构建日期。`size=151552` 与 `ls` 的文件大小一致；`boot CPU=0` 是设备树头记录的启动 CPU 物理 ID；`string block size=7470` 和 `DT structure block size=141980` 分别是属性名称字符串区与节点/属性结构区的字节数。它们描述二进制内部布局，不能单独说明系统性能、内存容量或 DTB 来源。

### 步骤 14：检查板端设备树编译器可用性

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
command -v dtc
```

实际输出（学习者提供；退出码未记录）：

```text
无输出
```

观察：在当前 Root Shell 的 `PATH` 中未找到 `dtc`（Device Tree Compiler）。这不能穷尽证明系统任何位置都没有该程序，但足以说明不能直接在当前环境按命令名调用它。为避免为了读取 DTB 而改变板端环境，后续先检查 Arch 主机是否具备该工具。

### 步骤 15：检查 Arch 主机设备树编译器可用性

执行端：Arch Linux 主机 Shell；当前目录：未记录。

```sh
command -v dtc
```

实际输出（学习者提供；退出码未记录）：

```text
无输出
```

观察：Arch 主机的当前 `PATH` 中也没有可直接调用的 `dtc`。板端和主机均尚未具备该工具，因此下一步应只读查询 Arch 官方包元数据，确认安装来源、用途和依赖，再由学习者决定是否安装。

### 步骤 16：验证主机设备树编译器版本

执行端：Arch Linux 主机 Shell；当前目录：未记录。

```sh
dtc -v
```

实际输出（学习者提供；退出码未记录）：

```text
Version: DTC v1.8.1
```

观察：主机现在可以直接执行 `dtc`，版本为 1.8.1，说明 Device Tree Compiler 已完成安装并进入当前 Shell 的可执行路径。安装命令、安装时间、包来源和安装的依赖尚未记录，不能从版本输出单独推断；下一步通过 `pacman -Qi dtc` 读取已安装包的元数据。

### 步骤 17：读取已安装 `dtc` 包元数据

执行端：Arch Linux 主机 Shell；当前目录：`~`。

```sh
pacman -Qi dtc
```

实际输出（学习者提供；退出码未记录）：

```text
名字           : dtc
版本           : 1:1.8.1-1
描述           : Device Tree Compiler
架构           : x86_64
URL            : https://www.devicetree.org/
软件许可       : GPL-2.0-or-later
提供           : libfdt.so=1-64
依赖于         : bash  glibc  libyaml
可选依赖       : python: Python bindings [已安装]
安装后大小     : 685.73 KiB
打包者         : George Rawlinson <grawlinson@archlinux.org>
编译日期       : 2026年07月03日 星期五 08时11分43秒
安装日期       : 2026年08月07日 星期五 20时53分39秒
安装原因       : 单独指定安装
安装脚本       : 否
验证者         : 数字签名
```

观察：主机已安装 `dtc` 包 `1:1.8.1-1`，其用途为 Device Tree Compiler，安装原因为学习者单独指定。已安装版本提供 `libfdt.so=1-64`；运行时依赖为 `bash`、`glibc` 和 `libyaml`，Python 绑定为已安装的可选依赖。包元数据显示安装日期为 2026-08-07T20:53:39+08:00，且有数字签名验证；具体安装命令未记录。

### 步骤 18：检查板端网络接口链路状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`/`。

```sh
ip -br link
```

实际输出（学习者提供；MAC 地址已脱敏；退出码未记录）：

```text
lo               UNKNOWN        00:00:00:00:00:00 <LOOPBACK,UP,LOWER_UP>
can0             DOWN           <NOARP,ECHO>
eth0             DOWN           xx:xx:xx:xx:xx:xx <NO-CARRIER,BROADCAST,MULTICAST,UP>
```

观察：`lo` 是只供本机使用的回环接口；`can0` 是 CAN 总线接口，不是普通以太网传输通道，且当前为 DOWN。`eth0` 的管理状态已启用（标志含 `UP`），但 `NO-CARRIER` 表示未检测到物理以太网链路，因此当前不能用它向主机传输 FDT。可能原因包括未接网线、对端未接通或未完成物理层协商；本实验未区分这些原因。

### 步骤 19：检查传统启动目录

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
ls -la /boot
```

实际输出（学习者提供；退出码未记录）：

```text
total 8
drwxr-xr-x  2 youyeetoo youyeetoo 4096 Mar 19  2024 .
drwxr-xr-x 24 root      root      4096 Nov 22 04:57 ..
```

观察：当前根文件系统中的 `/boot` 是空目录，没有常见的内核镜像、DTB、`extlinux/` 或启动配置文件。这不能说明 eMMC 上没有启动组件；它只排除了“组件位于当前已挂载根文件系统的 `/boot`”这一假设。RK3588 系统可能从独立 eMMC 分区加载 Loader、U-Boot、内核或 DTB，需继续检查 `mmcblk0` 分区布局。

### 步骤 20：建立 eMMC 分区地图

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
lsblk -o NAME,SIZE,FSTYPE,PARTLABEL,PARTUUID,MOUNTPOINTS /dev/mmcblk0
```

实际输出（学习者提供；退出码未记录）：

```text
NAME      SIZE FSTYPE PARTLABEL PARTUUID                             MOUNTPOINTS
mmcblk0  28.8G
├─mmcblk0p1
│           4M        uboot     70190000-0000-412d-8000-5ae500003bdf
├─mmcblk0p2
│           4M        misc      a4640000-0000-4a75-8000-420d00001b32
├─mmcblk0p3
│          64M        boot      7a3f0000-0000-446a-8000-702f00006273
├─mmcblk0p4
│         128M        recovery  a6450000-0000-4a07-8000-7cf200005735
├─mmcblk0p5
│          32M        backup    c2610000-0000-4264-8000-7eff00001135
├─mmcblk0p6
│          14G ext4   rootfs    614e0000-0000-4b53-8000-1d28000054a9 /
├─mmcblk0p7
│         128M ext2   oem       fb190000-0000-4870-8000-1c1800005a79 /oem
└─mmcblk0p8
         14.4G ext2   userdata  67330000-0000-4629-8000-61730000400d /userdata
```

观察：当前 eMMC 有 8 个分区。`p1` 标签为 `uboot`，`p3` 标签为 `boot`，二者均未识别出普通文件系统；`p6` 标签为 `rootfs`、格式为 ext4、挂载到 `/`。内核命令行中 `root=PARTUUID=614e0000-0000` 与 `p6` PARTUUID 的起始部分相符，交叉确认根文件系统来自 `p6`。因此 `p3` 是当前 DTB/内核镜像的强候选位置，但分区标签只表示布局意图，尚不能证明其中实际包含 DTB；必须读取分区头验证其镜像格式。详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。

### 步骤 21：识别 `boot` 分区起始镜像头

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
file -s /dev/mmcblk0p3
```

实际输出（学习者提供；退出码未记录）：

```text
/dev/mmcblk0p3: Device Tree Blob version 17, size=1536, boot CPU=0, string block size=190, DT structure block size=1004
```

观察：`p3` 的**起始位置**是一个有效的 DTB，FDT 格式版本为 17，头部声明该 blob 的总大小为 1536 字节。这里的 1536 字节只描述分区开头的这一个 FDT，不能据此推断整个 64 MiB `p3` 分区都是 DTB。它也不等同于当前运行时 FDT：后者大小为 151552 字节，明显不是同一个完整 blob。这个小 DTB 是启动参数、镜像内嵌组件还是其他用途，及其后续分区内容，均待验证。

### 步骤 22：读取起始 FDT 中的可打印字符串

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
dd if=/dev/mmcblk0p3 bs=1536 count=1 status=none | strings -n 3
```

实际输出（学习者提供；退出码未记录）：

```text
U-Boot FIT source file for arm
images
fdt
flat_dt
arm64
none
hash
]u%
,sha256
kernel
kernel
arm64
1linux
none
hash
S7zi
,sha256
resource
multi
arm64
none
hash
+-f
f$V
,sha256
configurations
:conf
conf
Qfdt
Ukernel
\resource
signature
,sha256,rsa2048
bpss
jdev
xfdt
kernel
multi
	description
data
type
arch
compression
load
algo
entry
default
rollback-index
fdt
kernel
multi
padding
key-name-hint
sign-images
data-position
data-size
timestamp
totalsize
version
value
ENTRlogo.bmp
```

观察：`U-Boot FIT source file for arm` 以及 `images`、`configurations` 共同确认这 1536 字节 FDT 是 U-Boot FIT 元数据树，而不是板级设备树本体。它描述了 `fdt`（类型 `flat_dt`）、`kernel` 和 `resource` 三类载荷，并出现 SHA-256 哈希、`sha256,rsa2048` 签名配置以及 `data-position`、`data-size` 属性名。当前只读字符串没有保留节点层次或属性数值，不能据此确定载荷位置、大小、选中的配置或签名是否强制验证。`strings` 也会从哈希等二进制字节中产生偶然文本，因此不能单独解释 `]u%`、`S7zi`、`ENTRlogo.bmp` 等片段。详见[FIT 笔记](../note/uboot-fit-image.md)。

### 步骤 23：检查板端 `dumpimage` 可用性

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
dumpimage
```

实际输出（学习者提供；退出码未记录）：

```text
-bash: dumpimage: command not found
```

观察：当前板端 Shell 无法按命令名找到 `dumpimage`，因此暂时不能在板端使用 U-Boot 专用工具结构化列出 FIT。该结果不影响 FIT 格式结论，也不表示镜像损坏。没有执行软件安装；后续优先确认能否把 1536 字节元数据通过现有串口以文本形式传到已安装 `dtc` 的 Arch 主机。

### 步骤 24：检查板端 Base64 编码工具

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
command -v base64
```

实际输出（学习者提供；退出码未记录）：

```text
/usr/bin/base64
```

观察：板端可直接调用 `base64`。这为通过串口传输少量二进制数据提供了不改动 eMMC 的方法：先从 `p3` 只读出 FIT 元数据的 1536 字节，再编码为可打印文本，由主机解码后用 `dtc` 解析。工具存在本身不验证编码参数或传输完整性，下一步需实际编码并核对长度。

### 步骤 25：解码 FIT 元数据到主机并核对长度

学习者报告：已将板端输出的 Base64 文本复制并粘贴到 Arch 主机的 `base64 -d` 标准输入，生成临时文件 `/tmp/r1-p3-fit.dtb`。完整 Base64 文本未保存到本实验记录。

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
stat -c 'size=%s bytes; type=%F' /tmp/r1-p3-fit.dtb
```

实际输出（学习者提供；退出码未记录）：

```text
size=1536 bytes; type=一般文件
```

观察：解码后的主机临时文件长度为 1536 字节，与板端 `dd` 的严格读取范围和 FIT 头声明的元数据长度一致。`一般文件` 只说明它是主机文件系统中的普通文件，不是 FIT 格式证明。长度相符降低了截断风险，但不能单独证明内容正确，仍需用 `file` 重新检查 FDT 头。

### 步骤 26：在主机重新识别 FDT 头

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
file /tmp/r1-p3-fit.dtb
```

实际结果（学习者提供；完整文本未保留）：

```text
和预期输出一样
```

观察：学习者确认主机 `file` 的识别结果符合“1536 字节 Device Tree Blob”的预期。这与长度检查共同表明，传输后的临时文件仍具有可解析的 FDT 头；但由于完整 `file` 文本未保存，不能在此记录更多头部字段。该检查不验证 FIT 载荷哈希或签名，只足以安全进入主机侧反编译。

### 步骤 27：将 FIT 元数据反编译为 DTS

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
dtc -I dtb -O dts -o /tmp/r1-p3-fit.dts /tmp/r1-p3-fit.dtb
```

实际输出：无输出（学习者提供）。

学习者随后执行：

```sh
ls /tmp/r1-p3-fit.*
```

实际输出（学习者提供）：

```text
/tmp/r1-p3-fit.dtb  /tmp/r1-p3-fit.dts
```

观察：`dtc` 成功将 FIT/FDT 二进制转换成可读 DTS，并在主机 `/tmp` 生成输出文件。命令无错误输出且输出文件存在，说明这份传输后的元数据可被 DTC 解析。此步骤尚未读取 DTS 内容，因此 FIT 的实际载荷偏移、大小与配置引用仍待确认。

### 步骤 28：读取 FIT 的结构化载荷布局

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
sed -n '1,200p' /tmp/r1-p3-fit.dts
```

实际输出（学习者提供；退出码未记录）：

```dts
/dts-v1/;

/memreserve/	0x000070538c000000 0x0000000000000600;
/ {
	version = <0x00>;
	totalsize = <0x22ce800>;
	timestamp = <0x66f8c524>;
	description = "U-Boot FIT source file for arm";

	images {

		fdt {
			data-size = <0x24172>;
			data-position = <0x800>;
			type = "flat_dt";
			arch = "arm64";
			compression = "none";
			load = <0xffffff00>;

			hash {
				value = <0xabd1c6c3 0x20e5de7c 0xd91c90d1 0xc56eb108 0x22be19c4 0x15d7525 0x9aaf04a9 0x6fe87546>;
				algo = "sha256";
			};
		};

		kernel {
			data-size = <0x220da00>;
			data-position = <0x24a00>;
			type = "kernel";
			arch = "arm64";
			os = "linux";
			compression = "none";
			entry = <0xffffff01>;
			load = <0xffffff01>;

			hash {
				value = <0x5e8fc7f4 0x85e3a8ef 0x71f73d0c 0x69d6520b 0xd753377a 0x697f1ec9 0x83f8ec62 0x449ceeb>;
				algo = "sha256";
			};
		};

		resource {
			data-size = <0x9c000>;
			data-position = <0x2232400>;
			type = "multi";
			arch = "arm64";
			compression = "none";

			hash {
				value = <0x492cbec9 0x8ecfdd2b 0x2d66f036 0x94127d87 0x44ec8933 0x9716b60b 0x9d662456 0xbc6569c6>;
				algo = "sha256";
			};
		};
	};

	configurations {
		default = "conf";

		conf {
			rollback-index = <0x00>;
			fdt = "fdt";
			kernel = "kernel";
			multi = "resource";

			signature {
				algo = "sha256,rsa2048";
				padding = "pss";
				key-name-hint = "dev";
				sign-images = "fdt", "kernel", "multi";
			};
		};
	};
};
```

观察：默认配置 `conf` 明确选择 `fdt`、`kernel` 与 `resource`。三个 `data-position` 与 `data-size` 形成连续布局：`fdt` 为 `0x800` 开始的 147826 字节，`kernel` 为 `0x24a00` 开始的 35707392 字节，`resource` 为 `0x2232400` 开始的 638976 字节；`totalsize=0x22ce800`（36497408 B）位于资源之后的 1024 字节填充末端。`fdt` 载荷大小 147826 字节，不等于运行时 FDT 的 151552 字节，因此不能把两者当作相同原始 blob。`signature` 节点描述了算法、填充、键提示和签名对象，但本输出不足以证明 U-Boot 实际启用、拥有对应公钥或已成功验证签名。详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。

### 步骤 29：校验实际 `fdt` 载荷的 SHA-256

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
dd if=/dev/mmcblk0p3 bs=1 skip=2048 count=147826 status=none | sha256sum
```

实际哈希字段（学习者提供；标准输入文件名字段未记录）：

```text
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546
```

观察：该值与 FIT `fdt/hash/value` 的八个 32 位单元拼接后的 SHA-256 完全一致。这直接验证 `p3` 偏移 `0x800` 开始、长度 `0x24172` 的实际字节就是 FIT 所声明的 `fdt` 载荷。该哈希是内容完整性校验，不证明 U-Boot 已执行它，也不证明配置签名有效；`kernel` 和 `resource` 的哈希尚未验证。

### 步骤 30：在 FIT `fdt` 载荷中筛选板级标识

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
dd if=/dev/mmcblk0p3 bs=1 skip=2048 count=147826 status=none | strings -n 3 | grep -Ei 'rk3588|evb4|model|compatible'
```

实际输出（学习者提供；以下保留根标识和代表性外设兼容字符串。输出另含大量重复的 `rockchip,rk3588-*-gate-link`、PWM、I2C、I2S、SPDIF 等节点字符串，未单独保存完整原始文本）：

```text
rockchip,rk3588s-evb4-lp4x-v10
rockchip,rk3588
7Rockchip RK3588S EVB4 LP4X V10 Board
rockchip,rk3588-clock-gate-link
rockchip,rk3588-csi2-dcphy
rockchip,rk3588-rknpu
rockchip,rk3588-dwc3
rockchip,rk3588-pcie
rockchip,rk3588-gmac
rockchip,rk3588-dw-mshc
rockchip,rk3588-pinctrl
	compatible
model
regulator-compatible
```

观察：FIT `fdt` 中出现的根 `compatible` 与运行时读取值完全相同，也包含相同的 `model` 文本。`strings` 在模型文字前显示的 `7` 来自相邻的可打印二进制字节，不能当作属性值的一部分；运行时以 NUL 分隔方式读取的模型值没有该字符。其余大量 `rk3588-*` 字符串是多个外设节点的 `compatible` 属性，不能据此逐项断言硬件已经接线、启用或驱动成功。FIT `fdt` 的大小仍为 147826 字节，而运行时 FDT 是 151552 字节，因此二者的逐字节关系仍待确认。

### 步骤 31：检查 ZMODEM 发送工具与以太网物理链路

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
sz
ip -br link
```

实际输出（学习者提供；`ip -br link` 保留本次完整输出）：

```text
-bash: sz: command not found
lo               UNKNOWN        00:00:00:00:00:00 <LOOPBACK,UP,LOWER_UP>
can0             DOWN           <NOARP,ECHO>
eth0             UP             1e:a8:e4:78:ee:77 <BROADCAST,MULTICAST,UP,LOWER_UP>
```

观察：`sz: command not found` 表明当前 Shell 中没有可直接调用的 ZMODEM 发送程序；没有安装或修改系统。`eth0` 同时具有管理状态 `UP` 和链路状态 `LOWER_UP`，说明网卡已启用且检测到与对端的物理以太网连接。这取代了先前的 `NO-CARRIER` 状态，但尚未显示 IP 地址、默认路由或外网连通性；不能据此断言 DHCP 已成功。

### 步骤 32：检查以太网接口地址

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
ip -br addr show dev eth0
```

实际输出（学习者提供）：

```text
eth0             UP
```

观察：简洁地址视图没有列出 `inet`（IPv4）或 `inet6`（IPv6）地址。因此目前只能确认物理链路存在，不能通过该接口进行 IP 通信。原因尚未确定，可能涉及 DHCP 客户端、网络管理服务或其连接配置；在识别实际负责网络的服务前，不手工配置地址。

### 步骤 33：识别运行中的网络管理服务

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
systemctl --no-pager --type=service --state=running | grep -Ei 'NetworkManager|systemd-networkd|networking|connman'
```

实际输出（学习者提供）：

```text
NetworkManager.service        loaded active running Network Manager
```

观察：NetworkManager 正在运行，因此它是当前最直接的网络配置与 DHCP 状态观察点。该结果不说明 `eth0` 已有可用连接，也不说明 DHCP 请求已经成功；下一步应向 NetworkManager 查询该设备的状态，而不是直接重启服务或手工配置地址。

### 步骤 34：查看 NetworkManager 的以太网连接状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
nmcli device status
```

实际输出（学习者提供）：

```text
DEVICE  TYPE      STATE                                  CONNECTION
eth0    ethernet  connecting (getting IP configuration)  Wired connection 1
can0    can       unmanaged                              --
lo      loopback  unmanaged                              --
```

观察：NetworkManager 已将 `eth0` 关联到 `Wired connection 1`，且状态为 `connecting (getting IP configuration)`。这表明物理链路之后 DHCP/IP 配置流程已启动，但尚未完成；此刻没有证据表明 DHCP 失败。`can0` 与 `lo` 显示为 `unmanaged` 是 NetworkManager 未管理它们的状态，不是以太网问题。

### 步骤 35：等待 DHCP/IP 配置完成后复查状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
nmcli device status
```

实际输出（学习者提供）：

```text
DEVICE  TYPE      STATE         CONNECTION
eth0    ethernet  disconnected  --
can0    can       unmanaged     --
lo      loopback  unmanaged     --
```

补充环境（学习者提供）：本次网线的另一端连接 Arch Linux 主机，并非路由器或交换机。

观察：NetworkManager 的 IP 配置流程已结束，`eth0` 回到 `disconnected`，没有地址或活动连接。板端与主机间的 `UP,LOWER_UP` 已证明物理层连通；但直接连接主机不会自动提供 DHCP、地址、路由或互联网。**推测**：主机端未提供 DHCP 服务，或直连双方尚未建立匹配的地址配置。应先在 Arch 主机识别连接该线的网卡和链路状态，再决定是否研究 DHCP 共享或静态地址；当前不改动任何网络配置。

### 步骤 36：识别 Arch 主机直连网卡

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
nmcli device status
ip -br link
```

实际输出（学习者提供）：

```text
DEVICE        TYPE      STATE         CONNECTION
wlo1          wifi      已连接        jililooss
enp108s0      ethernet  已连接        enp108s0
lo            loopback  连接（外部）  lo
Meta          tun       连接（外部）  Meta
p2p-dev-wlo1  wifi-p2p  已断开        --

lo               UNKNOWN        00:00:00:00:00:00 <LOOPBACK,UP,LOWER_UP>
enp108s0         UP             08:bf:b8:c2:8a:1b <BROADCAST,MULTICAST,UP,LOWER_UP>
wlo1             UP             dc:46:28:1f:48:20 <BROADCAST,MULTICAST,UP,LOWER_UP>
Meta             UNKNOWN        <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP>
```

观察：结合“网线直连 Arch 主机”的环境信息，`enp108s0` 是当前主机侧的直连以太网接口：它被 NetworkManager 标为已连接，且标志含 `UP,LOWER_UP`。`wlo1` 是另一条已连接的 Wi-Fi 上网链路，`Meta` 是隧道接口；它们不应被误当作通往板子的以太网端口。主机 `enp108s0` 是否配置 IP 地址、DHCP 服务或连接共享尚未验证。

### 步骤 37：读取主机直连接口的地址

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
ip -br addr show dev enp108s0
```

实际输出（学习者提供）：

```text
enp108s0         UP             192.168.0.1/24 fe80::abf:b8ff:fec2:8a1b/64
```

观察：主机在直连网段配置了 IPv4 地址 `192.168.0.1/24`，其子网范围为 `192.168.0.0/24`；`fe80::/64` 是该接口的 IPv6 链路本地地址。主机和板端物理层都已确认，但该地址本身不能证明 NetworkManager 正在提供 DHCP 或互联网连接共享。应读取连接配置的 IPv4 方法后再判断板端未获得地址的原因。

### 步骤 38：读取主机直连连接的 IPv4 方法

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
nmcli -g ipv4.method,ipv4.addresses,ipv4.gateway connection show enp108s0
```

实际输出（学习者提供；第三个字段无输出）：

```text
manual
192.168.0.1/24
```

观察：`enp108s0` 使用 `manual` IPv4 方法，主机静态配置为 `192.168.0.1/24`，没有记录 IPv4 网关。它不是 NetworkManager 的 `shared` 模式，因此不能期待它自动在这条直连链路上为 R1 提供 DHCP 租约。为验证直连通路，可在板端临时配置同一子网的未使用地址（例如 `192.168.0.2/24`）；这只改变运行时网络状态，不写入 eMMC，重启后会消失。

### 步骤 39：用临时静态地址验证主机—板子 IP 通信

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
ip address add 192.168.0.2/24 dev eth0
ip -br addr show dev eth0
ping 192.168.0.1
```

实际输出（学习者提供；`ping` 由 Ctrl-C 主动中断）：

```text
eth0             UP             192.168.0.2/24
PING 192.168.0.1 (192.168.0.1) 56(84) bytes of data.
64 bytes from 192.168.0.1: icmp_seq=1 ttl=64 time=0.979 ms
64 bytes from 192.168.0.1: icmp_seq=2 ttl=64 time=0.695 ms
^C
--- 192.168.0.1 ping statistics ---
2 packets transmitted, 2 received, 0% packet loss, time 1001ms
rtt min/avg/max/mdev = 0.695/0.837/0.979/0.142 ms
```

观察：R1 的运行时 `eth0` 成功配置 `192.168.0.2/24`，并可与主机 `192.168.0.1` 双向进行 ICMP 通信，2 个请求均收到回复。这验证了网卡、网线、物理链路、ARP 和同一 IPv4 子网内的主机—板子通信。该地址由 `ip address add` 临时添加，不写入 eMMC，重启后消失；尚未验证互联网转发或 DNS。

### 步骤 40：确认板端有线连接的地址获取方式

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
nmcli -g ipv4.method connection show 'Wired connection 1'
```

实际输出（学习者提供）：

```text
auto
```

观察：`Wired connection 1` 使用自动 IPv4 方法，即 DHCP 客户端模式。若主机的 `enp108s0` 后续配置为 NetworkManager `shared`，R1 现有连接配置可请求主机提供的 DHCP 租约，无需为每次启动持久化手动地址。主机共享配置尚未执行。

### 步骤 41：启用主机共享后观察地址结果

执行端：Arch Linux 主机与 R1 目标 Linux 的 Shell；当前目录分别为 `~/Study/rk3588` 与 `~`。

实际输出（学习者提供）：

```text
# Arch 主机：ip -br addr show dev enp108s0
enp108s0         UP             10.42.0.1/24 fe80::abf:b8ff:fec2:8a1b/64

# R1：ip -br addr show dev eth0
eth0             UP             169.254.80.143/16 fe80::122e:5ffa:f228:b2ce/64
```

补充现象（学习者提供）：R1 ping `10.42.0.1` 不通；该命令的完整输出未保留。

观察：主机直连接口现为 `10.42.0.1/24`，符合 NetworkManager `shared` 模式常用的私有网段形式，但尚未重新读取连接属性确认该模式。R1 未取得预期的 `10.42.0.0/24` DHCP 地址，而是使用 `169.254.80.143/16` IPv4 链路本地地址。链路本地地址表示该接口没有获得普通 IPv4 配置后仍能在 `169.254.0.0/16` 范围内本地通信；它与 `10.42.0.1/24` 不在同一子网，且没有默认路由时 ping 失败符合预期。下一步应检查主机 DHCP 端口是否监听，而不把 ping 失败误归因于网线或网卡。

### 步骤 42：检查主机共享 DHCP/DNS 监听端口

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
sudo ss -ulpn | grep -E ':(67|53)\b'
```

实际输出（学习者提供）：

```text
UNCONN 0      0                             10.42.0.1:53         0.0.0.0:*    users:(("dnsmasq",pid=94100,fd=6))
UNCONN 0      0                               0.0.0.0:67         0.0.0.0:*    users:(("dnsmasq",pid=94100,fd=4))
```

观察：`dnsmasq` 在主机上监听 UDP 67（DHCP）和 `10.42.0.1:53`（DNS），表明共享模式的 DHCP/DNS 服务已启动。R1 仍未获租约的原因不能归结为“主机没有 DHCP 服务”；应回到 R1 观察其当前 NetworkManager 设备状态和连接是否仍处于断开、失败或其他状态，再决定是否触发一次新的 DHCP 尝试。

### 步骤 43：重读 R1 在主机共享启动后的设备状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
nmcli device status
```

实际输出（学习者提供）：

```text
DEVICE  TYPE      STATE         CONNECTION
eth0    ethernet  disconnected  --
can0    can       unmanaged     --
lo      loopback  unmanaged     --
```

观察：R1 的 `eth0` 仍为 `disconnected`，没有活动连接。主机 DHCP 服务已在此之前启动，而设备没有自动重新发起请求；因此下一步应通过 NetworkManager 对该设备发起一次重连，使已确认的 `ipv4.method=auto` 配置重新请求 DHCP。该操作改变运行时连接状态，但不修改 eMMC 中的系统镜像。

### 步骤 44：触发 R1 有线连接重新激活

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
nmcli device connect eth0
nmcli device status
```

实际输出（学习者提供）：

```text
^CError: nmcli terminated by signal Interrupt (2)
Error: Connection activation failed: (0) No reason given.

DEVICE  TYPE      STATE                                  CONNECTION
eth0    ethernet  connecting (getting IP configuration)  Wired connection 1
can0    can       unmanaged                              --
lo      loopback  unmanaged                              --
```

观察：学习者主动以 Ctrl-C 中断了前台 `nmcli device connect` 命令，因此其“activation failed”只说明 CLI 被信号中止，不能作为 NetworkManager 激活失败的根因。随后 `nmcli device status` 显示 `eth0` 仍在 `connecting (getting IP configuration)`，证明 NetworkManager 后台的 DHCP/IP 配置流程仍在进行。DHCP 尝试可能等待数十秒后才成功或超时；在这一轮结束前不重复触发新的连接请求。

### 步骤 45：记录 DHCP 尝试最终状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

实际输出（学习者提供）：

```text
disconnected
```

观察：当前 DHCP 尝试结束后，R1 `eth0` 回到 `disconnected`，未取得租约。主机已确认有 DHCP 监听，故创建 [ISSUE-20260809-003](../issue/issue-20260809-003-r1-dhcp-lease-missing.md) 跟踪该问题。下一步以主机抓包区分“请求未到达”与“未收到或未接受响应”。

### 步骤 46：捕获一次 R1 DHCP 重连报文

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。主机先运行 `sudo tcpdump -ni enp108s0 -vvv 'udp port 67 or udp port 68'`，学习者随后在 R1 触发一次 `eth0` 重连。

实际输出：完整原始抓包保存在[host-enp108s0-dhcp-capture-20260809.txt](../_assets/host-enp108s0-dhcp-capture-20260809.txt)。

观察：主机抓到同一源 MAC `1e:a8:e4:78:ee:77` 的多个 DHCP Discover（`0.0.0.0:68` 到广播 `255.255.255.255:67`），该 MAC 与 R1 `eth0` 已记录的 MAC 一致。因此 R1 的 DHCP 请求已穿过物理链路并到达主机。所保存的抓包中没有 DHCPOFFER、DHCPREQUEST 或 DHCPACK；当前证据将问题收敛到主机端 DHCP 服务处理或本机防火墙路径，详见[ISSUE-20260809-003](../issue/issue-20260809-003-r1-dhcp-lease-missing.md)。

### 步骤 47：读取共享 dnsmasq 的启动参数

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
sudo ps -ww -fp 94100
```

实际输出（学习者提供）：

```text
UID          PID    PPID  C STIME TTY          TIME CMD
nobody     94100     776  0 15:56 ?        00:00:00 /usr/bin/dnsmasq --conf-file=/dev/null --no-hosts --keep-in-foreground --bind-interfaces --except-interface=lo --clear-on-reload --strict-order --listen-address=10.42.0.1 --dhcp-range=10.42.0.10,10.42.0.254,3600 --dhcp-leasefile=/var/lib/NetworkManager/dnsmasq-enp108s0.leases --pid-file=/run/nm-dnsmasq-enp108s0.pid --conf-dir=/etc/NetworkManager/dnsmasq-shared.d
```

观察：该进程由 NetworkManager 启动，使用 `--listen-address=10.42.0.1`，并配置了 `10.42.0.10` 到 `10.42.0.254`、租期 3600 秒的 DHCP 范围。因而“共享模式未给 dnsmasq 配置地址池”的假设被排除。Discover 已到达而未见 Offer 的原因仍未知；下一步检查主机防火墙规则及其计数器。

### 步骤 48：检查主机 nftables/UFW 防火墙规则

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
sudo nft list ruleset
```

实际输出（学习者提供；以下摘录与 DHCP 根因直接相关的规则和计数器，完整终端输出未另存）：

```text
chain INPUT {
	type filter hook input priority filter; policy drop;
}

chain ufw-after-input {
	udp dport 67 counter packets 191 bytes 60074 jump ufw-skip-to-policy-input
}

chain ufw-skip-to-policy-input {
	counter packets 191 bytes 60074 drop
}

chain ufw-user-input {
	ip saddr 192.168.0.0/24 counter packets 18 bytes 3167 accept
}
```

观察：UFW 管理 IPv4 filter 表，`INPUT` 默认策略为 `drop`。UDP 67 的计数器与其最终 `drop` 计数器均为 191 个包、60074 字节，直接表明 DHCP Discover 被主机防火墙丢弃，未到达 dnsmasq；这与抓包中“Discover 有、Offer 无”一致。旧用户规则仅允许源网段 `192.168.0.0/24`，不匹配 DHCP Discover 的源地址 `0.0.0.0`。根因和最小修复范围见[ISSUE-20260809-003](../issue/issue-20260809-003-r1-dhcp-lease-missing.md)。

### 步骤 49：验证 UFW DHCP 放行后的自动租约与主机连通性

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。学习者已执行建议的最小 UFW DHCP 放行，但未保留该命令的主机标准输出。

```sh
ip -br addr show dev eth0
ping 10.42.0.1
```

实际输出（学习者提供；未保留 ping 汇总统计）：

```text
eth0             UP             10.42.0.192/24 fe80::9ff4:a6ba:cee7:2d4f/64
PING 10.42.0.1 (10.42.0.1) 56(84) bytes of data.
64 bytes from 10.42.0.1: icmp_seq=1 ttl=64 time=0.523 ms
64 bytes from 10.42.0.1: icmp_seq=2 ttl=64 time=0.660 ms
64 bytes from 10.42.0.1: icmp_seq=3 ttl=64 time=0.538 ms
```

观察：`10.42.0.192/24` 落在主机 dnsmasq 的 `10.42.0.10–254` 地址池内，证明 R1 已自动获得 DHCP 租约。主机 `10.42.0.1` 的三次 ICMP 回复验证了该租约后的同网段通信。至此 DHCP 问题已解决；DNS 和经 `wlo1` 的外网转发仍需单独测试。

### 步骤 50：测试主机共享 NAT 的外部 IPv4 连通性

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
ping -c 3 1.1.1.1
```

实际输出（学习者提供）：

```text
PING 1.1.1.1 (1.1.1.1) 56(84) bytes of data.

--- 1.1.1.1 ping statistics ---
3 packets transmitted, 0 received, 100% packet loss, time 2025ms
```

补充环境（学习者提供）：Arch 主机可 ping 百度的 IP 地址，具体命令和输出未保留。

观察：R1 的 ICMP 请求超时而非立即报告“网络不可达”，主机自身外网 IPv4 连通，因此问题不在主机 Wi-Fi 上游是否可用。需要检查主机从 `enp108s0` 到 `wlo1` 的转发路径；先前 UFW 的 `FORWARD` 默认策略为 `drop`，但尚未读取本次流量后的计数器，不能先行断言根因。

### 步骤 51：检查 UFW 转发链计数器

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
sudo nft list chain ip filter FORWARD
```

实际输出（学习者提供）：

```text
table ip filter {
	chain FORWARD {
		type filter hook forward priority filter; policy drop;
		counter packets 91 bytes 7644 jump ufw-before-logging-forward
		counter packets 91 bytes 7644 jump ufw-before-forward
		counter packets 0 bytes 0 jump ufw-after-forward
		counter packets 0 bytes 0 jump ufw-after-logging-forward
		counter packets 0 bytes 0 jump ufw-reject-forward
		counter packets 0 bytes 0 jump ufw-track-forward
	}
}
```

观察：本次 R1 外网测试后，UFW `FORWARD` 链已有 91 个包、7644 字节进入；其默认策略是 `drop`，且链内没有匹配 `enp108s0` 到 `wlo1` 的放行规则。DHCP 所需的是主机 `INPUT` 路径，而 R1 访问外网必须经过 `FORWARD`；两者是不同的防火墙位置。此现象单独记录为[ISSUE-20260809-004](../issue/issue-20260809-004-ufw-blocks-shared-nat-forward.md)。

### 步骤 52：添加最小 UFW 共享 NAT 转发规则并复测

执行端：Arch Linux 主机 Shell 与 R1 目标 Linux Shell。主机执行：

```sh
sudo ufw route allow in on enp108s0 out on wlo1 from 10.42.0.0/24
```

实际输出（学习者提供）：

```text
Rule added
```

补充现象（学习者提供）：随后 R1 外部 ping 仍不通，完整 ping 输出未保留。

观察：UFW 已接受最小的单向转发规则，但外网连通性尚未回归通过。此时不能假定规则实际命中，也不能直接继续放宽更多端口或接口；下一步读取 `ufw-user-forward` 链的规则和计数器，判断 R1 流量是否经过该规则。

### 步骤 53：检查新增 UFW 转发规则的计数器

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
sudo nft list chain ip filter ufw-user-forward
```

实际输出（学习者提供）：

```text
table ip filter {
	chain ufw-user-forward {
		ip saddr 10.42.0.0/24 iifname "enp108s0" oifname "wlo1" counter packets 0 bytes 0 accept
	}
}
```

观察：规则存在但计数为 0，说明 R1 的外网测试流量没有匹配该规则的 `oifname "wlo1"` 条件。主机还存在 `Meta` 隧道接口，且尚未查询主机到 `1.1.1.1` 的实际内核路由；应先读取该路由选择，而不是再添加盲目的放行规则。

### 步骤 54：检查板端 SSH 服务状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
systemctl is-active ssh
```

实际输出（学习者提供）：

```text
active
```

观察：板端 SSH 服务正在运行。结合已验证的主机—板子 DHCP 直连网络，SSH 可作为比手工 Base64/UART 更可靠的完整 FDT 文件传输通道。服务运行不等于主机认证已成功；下一步应从 Arch 主机尝试连接并观察首次主机密钥提示或认证结果。

### 步骤 55：读取 root 账户密码状态

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
passwd -S root
```

实际输出（学习者提供）：

```text
root P 08/23/2024 0 99999 7 -1
```

观察：状态字段 `P` 表示 root 账户设置过密码；本输出不显示密码内容。`08/23/2024` 是密码状态的上次变更日期，后续数值是账户老化策略参数。串口自动登录不揭示该密码，主机不能安全地猜测或枚举它；下一步读取 sshd 的有效认证策略，确认 root 是否允许密码或公钥登录。

### 步骤 56：读取 sshd 的有效认证策略

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
sshd -T | grep -E '^(permitrootlogin|passwordauthentication|kbdinteractiveauthentication|pubkeyauthentication) '
```

实际输出（学习者提供）：

```text
permitrootlogin yes
pubkeyauthentication yes
passwordauthentication yes
kbdinteractiveauthentication no
```

观察：当前有效策略允许 root 登录，也允许公钥认证和密码认证；键盘交互式认证关闭。因 root 密码未知，密码认证的“允许”不构成可用登录方式。公钥认证已启用，适合在主机生成或复用 SSH 密钥后，通过当前可信串口会话添加**公钥**到 root 的 `authorized_keys`；此过程不需要知道或重设 root 密码。

### 步骤 57：确认主机已有 SSH 公钥

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
ls ~/.ssh/
```

实际输出（学习者提供）：

```text
agent  id_ed25519  id_ed25519.pub  known_hosts
config  known_hosts.old
```

观察：主机已有 Ed25519 私钥 `id_ed25519` 及其对应公钥 `id_ed25519.pub`。后续仅向 R1 授权 `.pub` 公钥；私钥必须始终留在主机，不能复制到板端或粘贴到记录中。添加前先检查 R1 `/root/.ssh/authorized_keys` 是否已有内容，避免覆盖现有授权。

### 步骤 58：检查 R1 现有 root 授权公钥

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。

```sh
ls -ld /root/.ssh; ls -l /root/.ssh/authorized_keys
```

实际输出（学习者提供）：

```text
ls: cannot access '/root/.ssh': No such file or directory
ls: cannot access '/root/.ssh/authorized_keys': No such file or directory
```

观察：root 当前没有 `.ssh` 目录或 `authorized_keys`，未发现需要保留的现有 root 公钥授权。可以创建权限为 700 的目录和权限为 600 的授权文件，并仅写入主机 `id_ed25519.pub` 的公钥内容；此操作会改变 root 的 SSH 授权配置，须在显示并确认公钥后进行。

### 步骤 59：添加主机公钥并验证授权文件权限

执行端：R1 目标 Linux 的 Debug UART Shell；当前目录：`~`。学习者将 Arch 主机 `id_ed25519.pub` 的单行公钥写入新建的 `/root/.ssh/authorized_keys`，然后执行权限检查。

```sh
chmod 600 /root/.ssh/authorized_keys
stat -c 'path=%n mode=%a owner=%U group=%G size=%s' /root/.ssh /root/.ssh/authorized_keys
```

实际输出（学习者提供；命令回显在串口中相连，以下为 `stat` 输出）：

```text
path=/root/.ssh mode=700 owner=root group=root size=4096
path=/root/.ssh/authorized_keys mode=600 owner=root group=root size=97
```

观察：目录和授权文件均归 root 所有，权限分别为 700 与 600；97 字节与一条 Ed25519 公钥的常见文本长度相符。该配置授权持有主机对应私钥的实体以 root 身份通过 SSH 登录，不改变 root 密码。公钥文本本身未记录；下一步从主机强制只使用公钥认证进行验证。

### 步骤 60：测试主机 SSH 公钥认证

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
ssh -o ConnectTimeout=5 -o PreferredAuthentications=publickey -o PasswordAuthentication=no root@10.42.0.192 'id -un; hostname'
```

实际输出（学习者提供）：

```text
root@10.42.0.192: Permission denied (publickey,password).
```

观察：网络可达与 SSH 服务运行均已验证，但服务器没有接受本次主机提供的公钥。该错误不说明 root 密码错误，也不能据此推断目录权限问题；优先比较主机公钥和板端 `authorized_keys` 的指纹，再检查 sshd 的 `AuthorizedKeysFile` 有效路径。

### 步骤 61：比较主机与板端授权公钥指纹

执行端：R1 目标 Linux 的 Debug UART Shell 与 Arch Linux 主机 Shell。

```sh
# R1
ssh-keygen -lf /root/.ssh/authorized_keys

# Arch 主机
ssh-keygen -lf ~/.ssh/id_ed25519.pub
```

实际输出（学习者提供，均为同一行）：

```text
256 SHA256:3MXA9RlxfRuO7mouBBDWxc3qh777QKVbH+6CnO1OTN0 loser@archzhouk (ED25519)
```

观察：两端的位数、SHA-256 指纹、注释和算法均相同，因此 R1 的 `authorized_keys` 确实包含与主机 `id_ed25519` 对应的公钥。先前认证失败不能归因于复制了错误公钥；下一步显式指定主机私钥，并限制 SSH 仅尝试该身份，以排除客户端配置或 ssh-agent 选错密钥的可能。

### 步骤 62：显式指定对应私钥后重测 SSH 认证

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
ssh -i ~/.ssh/id_ed25519 -o IdentitiesOnly=yes -o ConnectTimeout=5 -o PasswordAuthentication=no root@10.42.0.192 'id -un; hostname'
```

实际输出（学习者提供）：

```text
root@10.42.0.192: Permission denied (publickey,password).
```

观察：`-i` 明确选择私钥，`IdentitiesOnly=yes` 禁止额外身份干扰；在公钥指纹已匹配的前提下，本次仍被拒绝，排除了“SSH 客户端选错密钥”假设。问题现收敛到 R1 sshd 的授权文件路径、文件系统权限策略或服务端其他认证限制；创建[ISSUE-20260809-005](../issue/issue-20260809-005-r1-ssh-public-key-rejected.md)继续跟踪。下一步只读出 sshd 实际配置的 `authorizedkeysfile`。

### 步骤 63：查询 sshd 的有效授权公钥路径

执行端：R1 目标 Linux 的 Debug UART Root Shell；当前目录：`~`。

```sh
sshd -T | grep -i '^authorizedkeysfile '
```

实际输出（学习者提供）：

```text
authorizedkeysfile .ssh/authorized_keys .ssh/authorized_keys2
```

观察：该相对路径相对于待认证用户的主目录解析；对 root 即 `/root/.ssh/authorized_keys` 和 `/root/.ssh/authorized_keys2`。第一个路径与已写入且指纹已匹配的文件一致，因此“sshd 读取了其他授权路径”的假设被排除。下一步检查从 `/` 到该文件的每一级目录权限，验证 sshd 的严格权限检查是否会拒绝该路径。

### 步骤 64：检查授权文件路径的所有者与权限

执行端：R1 目标 Linux 的 Debug UART Root Shell；当前目录：`~`。

```sh
ls -ld / /root /root/.ssh /root/.ssh/authorized_keys
```

实际输出（学习者提供）：

```text
drwxr-xr-x 24 root      root      4096 Nov 22 04:57 /
drwx------  7 youyeetoo youyeetoo 4096 Nov 22 09:29 /root
drwx------  2 root      root      4096 Nov 22 09:29 /root/.ssh
-rw-------  1 root      root        97 Nov 22 09:29 /root/.ssh/authorized_keys
```

观察：`.ssh` 与 `authorized_keys` 的属主和权限正确；但 root 的家目录 `/root` 虽为 700，却由普通账户 `youyeetoo` 所有。sshd 在启用严格模式时会检查授权文件及其上级目录；对 root 账户，非 root 所有的 `/root` 是公钥被拒绝的强烈证据。下一步先读取 ssh 服务日志，确认是否报告该目录所有权问题，再作最小修复；不应先更换密钥或关闭严格模式。

### 步骤 65：读取最近 SSH 服务日志

执行端：R1 目标 Linux 的 Debug UART Root Shell；当前目录：`~`。

```sh
journalctl -u ssh -n 30 --no-pager
```

实际输出（学习者提供，日志仅包含多次启动和停止 `OpenBSD Secure Shell server`、监听 22 端口及 Boot 分隔；未出现 `Authentication refused`、`bad ownership` 或 `/root` 认证拒绝行）。

观察：这 30 行日志没有保留本次公钥认证的详细拒绝原因，不能作为对 H4 的直接确认或否定。日志时间使用板端当前系统时钟（显示为 3 月 31 日），也不能与主机记录时间直接对应。继续读取 sshd 的 `strictmodes` 有效值：该开关决定服务是否对上述目录所有权实施严格检查。

### 步骤 66：确认 sshd 启用严格路径权限检查

执行端：R1 目标 Linux 的 Debug UART Root Shell；当前目录：`~`。

```sh
sshd -T | grep -i '^strictmodes '
```

实际输出（学习者提供）：

```text
strictmodes yes
```

观察：sshd 已启用严格模式，会检查授权密钥文件及其上级目录的权限和所有权。结合已验证的 `/root` 由 `youyeetoo:youyeetoo` 所有，可确认这是 root 公钥认证被拒绝的根因。最小修复是仅把 `/root` 目录本身改回 `root:root`；不使用递归选项、不改动密钥、不重启 ssh 服务。修复后必须从主机回归验证登录。

### 步骤 67：恢复 root 家目录属主

执行端：R1 目标 Linux 的 Debug UART Root Shell；当前目录：`~`。

```sh
chown root:root /root && ls -ld /root
```

实际输出（学习者提供）：

```text
drwx------ 7 root root 4096 Nov 22 09:29 /root
```

观察：`/root` 目录自身的用户和组属主已恢复为 `root:root`，模式仍为 700。该命令未使用 `-R`，因此不会递归变更 `.ssh`、授权密钥或其他 root 家目录内容；也未重启 sshd。学习者报告已能连接，但尚未保留可复现的主机命令输出；问题仍需执行一次明确的公钥登录回归测试后才能标记解决。

### 步骤 68：回归验证主机 SSH 公钥登录

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
ssh -o ConnectTimeout=5 -o PreferredAuthentications=publickey -o PasswordAuthentication=no root@10.42.0.192 'id -un; hostname'
```

实际输出（学习者提供）：

```text
root
R1
```

观察：同一主机、同一地址与先前失败的公钥优先认证命令，在仅恢复 `/root` 属主后成功输出远端用户和主机名。该回归验证将问题根因与修复结果建立直接关联；SSH 公钥登录可用，问题已解决。现在可将 SSH/SCP 用于只读导出完整运行时 FDT。

### 步骤 69：通过 SCP 导出完整运行时 FDT

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
scp -o ConnectTimeout=5 root@10.42.0.192:/sys/firmware/fdt /tmp/r1-runtime-fdt.dtb
```

实际结果（学习者提供）：复制完成；命令的完整输出和退出码未保留。学习者随后在主机完成下列核对：

```text
stat: size=151552 bytes; type=一般文件
file: Device Tree Blob version 17, size=151552, boot CPU=0, string block size=7470, DT structure block size=141980
sha256: 51cb9beb30f4b6221d13aa8c85bef9d957cea86afd8c164dbb72c356205d068c
```

观察：运行时 FDT 已报告复制到主机临时文件 `/tmp/r1-runtime-fdt.dtb`，其长度和 FDT 头部字段与板端此前读取的 `/sys/firmware/fdt` 一致；已记录主机侧 SHA-256。SCP 从板端只读打开 `/sys/firmware/fdt`，但会在主机 `/tmp` 写入文件。现在可再只读导出 FIT `fdt` 载荷，在主机进行逐字节比较。

### 步骤 70：通过 SSH 导出并核对 FIT `fdt` 载荷

执行端：Arch Linux 主机 Shell；当前目录：`~/Study/rk3588`。

```sh
ssh -o ConnectTimeout=5 root@10.42.0.192 \
  'dd if=/dev/mmcblk0p3 bs=1 skip=2048 count=147826 status=none' \
  > /tmp/r1-fit-fdt.dtb
stat -c 'size=%s bytes; type=%F' /tmp/r1-fit-fdt.dtb
sha256sum /tmp/r1-fit-fdt.dtb
```

实际输出（学习者提供）：

```text
size=147826 bytes; type=一般文件
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546  /tmp/r1-fit-fdt.dtb
```

观察：主机副本长度为 FIT 中 `fdt/data-size` 声明的 147826 字节，SHA-256 也与 FIT 声明及先前板端计算完全一致。因此它是可信的 FIT `fdt` 主机侧样本。它与 151552 字节的运行时 FDT 大小不同，已不可能逐字节完全相同；下一步用 `cmp` 确认差异并定位最早变化的位置。

### 步骤 71：比较 FIT `fdt` 与运行时 FDT 的最早字节差异

执行端：Arch Linux 主机的 fish Shell；当前目录：`~/Study/rk3588`。

```fish
cmp -s /tmp/r1-fit-fdt.dtb /tmp/r1-runtime-fdt.dtb
printf 'cmp exit=%s\n' $status
cmp -l /tmp/r1-fit-fdt.dtb /tmp/r1-runtime-fdt.dtb | head -n 10
```

实际输出（学习者提供）：

```text
cmp exit=1
     7 101 120
     8 162   0
    12  70 110
    15  45  52
    16 200 344
    35  33  35
    36 362  56
    39  45  52
    40 110 234
    45   0  10
```

补充：初次使用 Bash 的 `$?` 读取退出码时，fish 提示应使用 `$status`；改用后正确得到 `1`。

观察：`cmp` 的退出码 1 确认两份样本不同。`cmp -l` 的行号从 1 开始，前两处差异位于 FDT 头部的 `totalsize` 字段：FIT `fdt` 为 `0x24172`（147826），运行时 FDT 为 `0x25000`（151552）。随后早期差异仍位于 FDT 头部的偏移/大小字段；这些差异说明两个 blob 的布局不同，但尚不能说明哪个设备节点或属性被改动。下一步分别反编译两份 DTB，再比较可读 DTS。

### 步骤 72：比较 DTS 中的运行时补充属性

执行端：Arch Linux 主机的 fish Shell；当前目录：`~/Study/rk3588`。

```fish
diff -u /tmp/r1-fit-fdt.dts /tmp/r1-runtime-fdt.dts | head -n 80
```

实际输出（学习者提供；以下按文档脱敏规范保留差异类别，不记录设备序列号、MAC 地址或厂商配置 blob 原文）：

- 运行时 DTS 新增一条 `/memreserve/` 条目。
- 根节点新增 `serial-number`（值已脱敏）和 `memory` 节点。
- 显示输出路由新增视频时序、overscan、logo 尺寸/偏移等属性。
- 以太网节点新增 `local-mac-address`（值已脱敏）。
- `chosen` 新增厂商配置字符串（值已脱敏），且 `bootargs` 新增 eMMC、Android 兼容和 verified boot 状态字段。
- `drm-logo` 的 `reg` 从全零变为非零内存地址与长度。

观察：上述差异并非仅是 FDT 二进制头部重排；运行时树确实加入了内存、显示、网卡、启动参数和 logo 缓冲区等数据。**推测**：启动加载器或更早的固件在将 FIT 的基础 DTB 交给内核前执行了设备树 fixup；其中内存、MAC、启动参数与 logo 地址都符合这类运行期填充的典型用途。现有证据不足以断言具体由 U-Boot 的哪段代码或唯一组件写入。下一步保存完整差异到主机临时文件，只列出差异块位置，避免把设备标识写入仓库或对话。

### 步骤 73：确认完整 DTS 差异范围

执行端：Arch Linux 主机的 fish Shell；当前目录：`~/Study/rk3588`。

```fish
diff -u /tmp/r1-fit-fdt.dts /tmp/r1-runtime-fdt.dts \
  > /tmp/r1-fit-vs-runtime-fdt.diff; or true
grep -n '^@@' /tmp/r1-fit-vs-runtime-fdt.diff
wc -l /tmp/r1-fit-vs-runtime-fdt.diff
```

实际输出（学习者提供）：

```text
3:@@ -1,12 +1,19 @@
23:@@ -1167,6 +1174,23 @@
47:@@ -4555,6 +4579,7 @@
55:@@ -7689,7 +7714,8 @@
65:@@ -7728,7 +7754,7 @@
73 /tmp/r1-fit-vs-runtime-fdt.diff
```

观察：完整 diff 只有 73 行和 5 个差异块，且其范围正是步骤 72 已观察的根节点/内存、显示路由、以太网 MAC、`chosen` 与 DRM logo 节点。因此当前证据支持“基础 FIT DTB 加有限运行期补充”，而不是“完全替换为另一份板级 DTS”。完整临时 diff 含设备标识，保留在 `/tmp`，不纳入仓库。下一步转而只读识别 `p1=uboot` 分区中的启动加载器版本线索。

### 步骤 74：识别 `p1=uboot` 的文件头与启动加载器字符串

执行端：R1 目标 Linux 的 Debug UART Root Shell；当前目录：`~`。

```sh
file -s /dev/mmcblk0p1
strings -a /dev/mmcblk0p1 | grep -Ei 'U-Boot|RK3588' | head -n 30
```

实际输出（学习者提供，节选）：

```text
/dev/mmcblk0p1: Device Tree Blob version 17, size=2560, boot CPU=0, string block size=197, DT structure block size=1964
FIT Image with ATF/OP-TEE/U-Boot/MCU
U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1
board=evb_rk3588
board_name=evb_rk3588
```

其余输出还包括 `U-Boot dtb`、`rk3588-evb`、RK3588 相关时钟/引脚函数、U-Boot 环境脚本和 `U-Boot.armv8` 等文本。

初始观察：`p1` 起始 2560 字节是 FDT v17；对整个 `p1` 做 `strings` 时出现的 `FIT Image with ATF/OP-TEE/U-Boot/MCU` 与 U-Boot 版本字符串，说明该分区含有早期固件相关数据，但不能由 `strings` 确定它们是否属于起始 FDT。版本字符串 `U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1` 是发现于该分区的构建标识；`dirty` 表示构建源树在构建时含未提交改动，并不说明板端运行期间有未保存修改。

### 步骤 75：通过 SSH 提取并核对 `p1` 起始 FDT

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
ssh -o ConnectTimeout=5 root@10.42.0.192 \
  'dd if=/dev/mmcblk0p1 bs=2560 count=1 status=none' \
  > /tmp/r1-p1-fit.dtb

stat -c 'size=%s bytes; type=%F' /tmp/r1-p1-fit.dtb
```

实际输出（学习者提供）：

```text
size=2560 bytes; type=一般文件
```

观察：主机临时文件的长度与 `file -s /dev/mmcblk0p1` 报告的起始 FDT 总大小一致。`dd` 的输入是板端块设备、输出经 SSH 标准输出重定向到主机 `/tmp`；`count=1` 与 `bs=2560` 限定为仅读取起始 FDT，不写入 eMMC。其语义尚待主机侧 DTC 解析。

### 步骤 76：反编译 `p1` 起始 FDT 并纠正 FIT 假设

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
dtc -I dtb -O dts -o /tmp/r1-p1-fit.dts /tmp/r1-p1-fit.dtb
sed -n '1,260p' /tmp/r1-p1-fit.dts
```

实际输出（学习者提供，节选）：

```dts
/ {
	compatible = "rockchip,rk3588s-evb4-lp4x-v10", "rockchip,rk3588";
	model = "Rockchip RK3588S EVB4 LP4X V10 Board";

	aliases {
		serial0 = "/serial@fd890000";
		mmc0 = "/mmc@fe2e0000";
	};

	clocks {
		compatible = "simple-bus";
		...
	};
};
```

观察：输出为 RK3588S EVB4 平台硬件描述，包含根 `compatible`、`model`、设备别名和时钟节点；在已查看范围内没有 FIT 必有的 `images` 或 `configurations` 节点。因此，**已否定**“`p1` 起始 2560 字节就是 FIT 元数据”的先前推测。它更符合供启动加载器使用的板级 FDT；其实际消费者尚待确认。`strings` 在整个 `p1` 中发现的 FIT/ATF/OP-TEE/U-Boot/MCU 文本仍是有效线索，但 FIT 的确切起始偏移、结构和与该 FDT 的关系都待定位。

### 步骤 77：确认主机可用于 eMMC 备份的空间

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
df -h /home/loser/Study/rk3588
```

实际输出（学习者提供）：

```text
文件系统        大小  已用  可用 已用% 挂载点
/dev/nvme0n1p5  489G  299G  186G   62% /home
```

观察：备份目标所在的 `/home` 文件系统尚有 186 GiB 可用空间，足以保存当前约 29 GiB eMMC 的完整原始副本并留出校验空间。该命令只读取主机文件系统统计信息；执行当时尚未读取、创建或校验任何 eMMC 备份。学习者当时表示已有厂商固件和烧录方式；后续说明厂商固件尚未下载，因此不能把它当作已验证恢复方案。

### 步骤 78：为完整 eMMC 备份核对源设备和目标目录

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
set backup_dir ~/Study/rk3588-backup
mkdir -p $backup_dir

ssh -o ConnectTimeout=5 root@10.42.0.192 \
  'lsblk -o NAME,SIZE,TYPE,MOUNTPOINTS /dev/mmcblk0'

df -h $backup_dir
```

实际输出（学习者提供）：

```text
NAME         SIZE TYPE MOUNTPOINTS
mmcblk0     28.8G disk
├─mmcblk0p1    4M part
├─mmcblk0p2    4M part
├─mmcblk0p3   64M part
├─mmcblk0p4  128M part
├─mmcblk0p5   32M part
├─mmcblk0p6   14G part /
├─mmcblk0p7  128M part /oem
└─mmcblk0p8 14.4G part /userdata

文件系统        大小  已用  可用 已用% 挂载点
/dev/nvme0n1p5  489G  299G  186G   62% /home
```

观察：源路径 `/dev/mmcblk0` 当前确为 R1 运行中的 28.8 GiB eMMC，且根分区为 `mmcblk0p6`；目标目录已创建在主机 `/home` 文件系统且空间充足。此步没有读取原始 eMMC 内容或向板端写入数据。下一步的完整读取应保持板子供电、网络和 SSH 连接稳定；由于根文件系统在线且可写，所得镜像是块级快照，不等同于离线一致性备份。

### 步骤 79：完成完整 eMMC 的在线只读导出

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
set image $backup_dir/r1-emmc-20260809.img

ssh -o ConnectTimeout=5 root@10.42.0.192 \
  'dd if=/dev/mmcblk0 bs=4M status=progress' \
  > $image

printf 'ssh/dd exit=%s\n' $status
```

实际输出（学习者提供）：

```text
ssh/dd exit=0
```

观察：SSH 命令的退出码为 0，表明远端 `dd` 已正常结束，主机重定向也未报告失败。此结果确认完整导出流程完成到主机 `~/Study/rk3588-backup/r1-emmc-20260809.img`；尚未核对远端与本地的精确字节数，也尚未计算 SHA-256，因此不能仅凭退出码断言备份内容完整或可恢复。镜像来自在线根文件系统，仍属于块级快照而非离线一致性保证。

### 步骤 80：核对 eMMC 与主机镜像的精确字节数

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
ssh -o ConnectTimeout=5 root@10.42.0.192 \
  'blockdev --getsize64 /dev/mmcblk0'

stat -c 'size=%s bytes' $image
```

实际输出（学习者提供）：

```text
30924603392
size=30924603392 bytes
```

观察：远端 eMMC 的精确容量与本地主机镜像长度均为 30924603392 字节，确认导出没有截断或少读。长度匹配不证明每个字节都未在传输或存储后损坏，仍需计算并保存镜像的 SHA-256；它也不消除在线根文件系统的一致性边界。

### 步骤 81：计算完整 eMMC 镜像的 SHA-256

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
sha256sum $image | tee $image.sha256
```

实际输出（学习者提供）：

```text
62b683c11cfbc5870ccfc98e9b0b334d5e2ab88a33d1d3f83f13a384a4a938d0  /home/loser/Study/rk3588-backup/r1-emmc-20260809.img
```

观察：已得到完整主机镜像在本次读取时的 SHA-256 指纹；该值可在日后检测镜像是否改变。命令设计为同时通过 `tee` 写入同目录的 `.sha256` 文件，但该文件的存在和可用性尚未独立用 `sha256sum -c` 回归核对。哈希仅标识该在线块级快照，不能证明它可离线一致恢复或厂商烧录流程可用。

### 步骤 82：回归核对镜像校验文件

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
sha256sum -c $image.sha256
```

实际输出（学习者提供）：

```text
/home/loser/Study/rk3588-backup/r1-emmc-20260809.img: 成功
```

观察：`sha256sum -c` 成功读取同目录校验文件，并重新计算当前镜像后匹配其中记录的 SHA-256；备份副文件可用于以后检测镜像是否发生变化。**边界**：这确认的是主机文件的当前完整性，不是将镜像写回板子后的恢复演练；镜像仍来自在线根文件系统。

### 步骤 83：读取 FIT 内 Linux 设备树的根节点与别名

执行端：Arch Linux 主机的 fish Shell；当前目录：`~`。

```fish
sed -n '1,45p' /tmp/r1-fit-fdt.dts
```

实际输出（学习者提供，节选；45 行范围未全部重复）：

```dts
/ {
	compatible = "rockchip,rk3588s-evb4-lp4x-v10", "rockchip,rk3588";
	interrupt-parent = <0x01>;
	#address-cells = <0x02>;
	#size-cells = <0x02>;
	model = "Rockchip RK3588S EVB4 LP4X V10 Board";

	aliases {
		ethernet1 = "/ethernet@fe1c0000";
		serial0 = "/serial@fd890000";
		...
	};
};
```

观察：根节点 `/` 是整棵硬件描述树的入口。`model` 供人阅读；两个 `compatible` 字符串从具体板级描述回退到通用 SoC；`#address-cells=<2>` 和 `#size-cells=<2>` 规定其子节点在 `reg` 等属性中分别使用两个 32 位单元表示地址与大小。`aliases` 节点将短名称映射到实际节点路径，例如 `ethernet1` 指向 `/ethernet@fe1c0000`；它不创建硬件、也不等同于驱动绑定。下一步应沿一个别名找到真实节点，观察节点名、`reg` 与 `compatible` 的角色。

### 步骤 84：沿 `ethernet1` 别名读取以太网硬件节点

执行端：Arch Linux 主机的 fish Shell；当前目录：`~/Study/rk3588`。

```fish
rg -n -A 18 '^[[:space:]]*ethernet@fe1c0000[[:space:]]*\{' /tmp/r1-fit-fdt.dts
```

实际输出（学习者提供，节选）：

```dts
ethernet@fe1c0000 {
	compatible = "rockchip,rk3588-gmac", "snps,dwmac-4.20a";
	reg = <0x00 0xfe1c0000 0x00 0x10000>;
	interrupts = <0x00 0xea 0x04 0x00 0xe9 0x04>;
	interrupt-names = "macirq", "eth_wake_irq";
	clock-names = "stmmaceth", "clk_mac_ref", "pclk_mac", "aclk_mac", "ptp_ref";
	status = "okay";
	phy-mode = "rgmii-rxid";
};
```

观察：`ethernet@fe1c0000` 中 `@fe1c0000` 是单元地址命名约定；`reg` 的四个单元由根节点的 2/2 规则解释为 64 位地址 `0xfe1c0000` 与大小 `0x10000`（64 KiB）。`compatible` 先声明 Rockchip GMAC 集成，再回退到 Synopsys DesignWare MAC 4.20a 通用实现，供内核选择匹配驱动。中断、时钟、复位和电源域属性描述驱动使用该控制器前需要取得的资源；当前不展开其 phandle 数值。`status = "okay"` 表示该 DT 节点启用，不证明网线有载波、DHCP 成功或驱动已绑定；`phy-mode` 描述 MAC 与外部 PHY 的接口模式。实际系统曾取得 `eth0` DHCP 地址，另有运行时网络证据支撑该接口可用，但不能仅由此 DTS 片段推导。

### 步骤 85：读取 `eth0` 的实际绑定驱动

执行端：Arch Linux 主机的 fish Shell，经 SSH 在 R1 目标 Linux 执行；远端当前目录：`~`。

```fish
ssh -o ConnectTimeout=5 root@10.42.0.192 \
  'ethtool -i eth0'
```

实际输出（学习者提供）：

```text
driver: st_gmac
version: Jan_2016
firmware-version:
expansion-rom-version:
bus-info:
supports-statistics: yes
supports-test: no
supports-eeprom-access: no
supports-register-dump: yes
supports-priv-flags: no
```

观察：运行中内核已为 `eth0` 绑定 `st_gmac` 驱动，形成“DT 中启用 GMAC 节点 → Linux 创建网络接口并报告驱动”的运行时证据链。`version: Jan_2016` 是该驱动向 ethtool 报告的版本字符串，不能当作板卡生产日期或当前内核构建日期；空的 `firmware-version`、`bus-info` 仅表示该驱动未在 ethtool 接口填充这些字段，不是加载失败。该输出尚未直接展示驱动模块的文件、模块别名或它与某一条 `compatible` 的精确匹配规则，需用 sysfs 或 `modinfo` 继续验证。

### 步骤 86：通过 sysfs 查看 `eth0` 的平台驱动绑定

执行端：R1 目标 Linux Root Shell；当前目录：`~`。

```sh
readlink -f /sys/class/net/eth0/device/driver
```

实际输出（学习者提供）：

```text
/sys/bus/platform/drivers/rk_gmac-dwmac
```

观察：sysfs 将 `eth0` 底层设备的 `driver` 符号链接解析到平台驱动目录 `rk_gmac-dwmac`，直接证明该平台驱动已绑定。它与 DTS 的 Rockchip GMAC / Synopsys DesignWare MAC 兼容项在语义上相符。`ethtool` 报告的 `st_gmac` 与 sysfs 的 `rk_gmac-dwmac` 是该厂商内核从不同接口呈现的驱动名称；二者名称不同不构成冲突。模块文件、别名和其精确匹配规则仍待检查，不能仅凭目录名断言具体源码文件。

### 步骤 87：通过 sysfs 定位 `eth0` 的平台设备

执行端：R1 目标 Linux Root Shell；当前目录：`~`。

```sh
readlink -f /sys/class/net/eth0/device
```

实际输出（学习者提供）：

```text
/sys/devices/platform/fe1c0000.ethernet
```

观察：Linux 用平台设备名 `fe1c0000.ethernet` 表示 `eth0` 的底层 GMAC；其中地址 `fe1c0000` 与 DTS 节点名 `ethernet@fe1c0000`、`reg` 的寄存器起始地址一致。至此已建立 `eth0 → fe1c0000.ethernet → rk_gmac-dwmac` 的运行时 sysfs 链条；下一步读取其 `of_node/compatible`，直接验证该设备引用的运行时设备树兼容字符串。

### 步骤 88：读取运行时 GMAC 节点的兼容项与状态

执行端：R1 目标 Linux Root Shell；当前目录：`~`。

```sh
tr '\0' '\n' < /sys/class/net/eth0/device/of_node/compatible
tr -d '\0' < /sys/class/net/eth0/device/of_node/status; printf '\n'
```

实际输出（学习者提供）：

```text
rockchip,rk3588-gmac
snps,dwmac-4.20a
okay
```

观察：运行时 OF 节点的 `compatible` 与已反编译的 GMAC DTS 节点一致，`status` 为 `okay`。前者将设备树匹配条件直接关联到已创建的平台设备，后者表明设备树启用该节点。`okay` 不是驱动绑定成功的替代证据；驱动目录和 `eth0` 网络接口已在前两步独立观察到。

### 步骤 89：观察 SPL、可信固件和 U-Boot 的实际启动日志

执行端：R1 Debug UART；学习者重启后观察串口，曾尝试以空格或回车中断启动，但未进入 U-Boot 提示符。重启命令和退出码未保留。

实际输出（学习者提供；仅保留启动链相关行，DDR 调频、GIC 和其他初始化诊断已省略）：

```text
U-Boot SPL 2017.09-gc060f28d70-220414 #zyf (Apr 18 2022 - 18:13:34)
Trying to boot from MMC2
MMC: no card present
spl: mmc init failed with error: -123
Trying to boot from MMC1
Trying fit image at 0x4000 sector
## Verified-boot: 0
## Checking atf-1 ... + OK
## Checking uboot ... + OK
## Checking fdt ... + OK
## Checking atf-2 ... + OK
## Checking atf-3 ... + OK
## Checking optee ... + OK
Jumping to U-Boot(0x00200000) via ARM Trusted Firmware(0x00040000)

NOTICE:  BL31: v2.3():v2.3-481-g17b41886e:derrick.huang
I/TC: OP-TEE version: 3.13.0-652-g4542e1efd ... aarch64
INFO:    Entry point address = 0x200000

U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1 (Sep 29 2024 - 11:10:07 +0800)
Model: Rockchip RK3588 Evaluation Board
DRAM:  4 GiB
Using default environment
Hotkey: ctrl+m
mmc@fe2c0000: 1, mmc@fe2e0000: 0
Bootdev(atags): mmc 0
boot mode: normal
```

观察：这首次直接确认当前正常启动路径已运行 SPL、一个含 ATF/U-Boot/FDT/OP-TEE 的早期 FIT、BL31（ARM Trusted Firmware 的 EL3 固件）、OP-TEE（BL32）和随后 U-Boot 本体。SPL 和 U-Boot 横幅的构建标识不同，不能把二者混为同一二进制。早期 FIT 的每个列出组件都报告 SHA-256 `OK`，这是本次启动实际执行的完整性检查证据；它与此前从 `p3` 读取到的 Linux FIT 及其签名元数据是不同层次的对象。

`Trying fit image at 0x4000 sector` 只说明 SPL 在其称为 `MMC1` 的设备上读取某个扇区位置；尚未将此 MMC 编号或扇区位置映射到 Linux 的 `/dev/mmcblk0p1`，也不能据此定位其分区。`MMC2` 无卡也不能单独说明其物理接口名称。`Hotkey: ctrl+m` 是 U-Boot 的实际输出，但现有证据不足以断言它是否、何时能中断自动启动；后续应先完整观察 `boot mode: normal` 之后的加载输出，而不是修改环境变量。

学习者随后提供了 `boot mode: normal` 之后至内核入口前的输出（显示、PMIC、时钟和内存转储的大部分诊断省略；这些原始输出未另存为文件）：

```text
FIT: no signed, no conf required
DTB: rk-kernel.dtb
HASH(c): OK
...
Net:   eth1: ethernet@fe1c0000
Hit key to stop autoboot('CTRL+C'):  0
ANDROID: reboot reason: "(none)"
Not AVB images, AVB skip
No valid android hdr
Android image load failed
Android boot failed, error -1.
## Booting FIT Image at 0xe95c7640 with size 0x02232400
...
## Loading kernel from FIT Image at e95c7640 ...
   Using 'conf' configuration
   Trying 'kernel' kernel subimage
     Data Size:    35707392 Bytes = 34.1 MiB
     Hash value:   5e8fc7f485e3a8ef71f73d0c69d6520bd753377a697f1ec983f8ec620449ceeb
   Verifying Hash Integrity ... sha256+ OK
## Loading fdt from FIT Image at e95c7640 ...
   Using 'conf' configuration
   Trying 'fdt' fdt subimage
     Data Size:    147826 Bytes = 144.4 KiB
     Hash value:   abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546
   Verifying Hash Integrity ... sha256+ OK
   Loading fdt from 0x08300000 to 0x08300000
   Booting using the fdt blob at 0x08300000
   Loading Kernel Image from 0xe95ec040 to 0x00400000 ... OK
   kernel loaded at 0x00400000, end = 0x0260da00
   Using Device Tree in place at 0000000008300000, end 0000000008327171
Total: 877.407 ms

Starting kernel ...
```

观察：这段日志回答了此前的两个关键问题。第一，第二阶段 U-Boot 的自动启动提示明确要求 `CTRL+C`，而非空格或回车；此前未停止不代表串口输入无效。第二，U-Boot 实际选择 FIT 配置 `conf`，加载并校验 `kernel` 与 `fdt`。它们的大小、SHA-256 和相对布局与此前从 `p3` 反编译的 Linux FIT 完全一致：FIT RAM 基址加 `0x800` 正好得到 FDT 数据地址，加 `0x24a00` 正好得到 kernel 数据地址。因此，当前运行的 Linux kernel/FDT 载荷与 `p3` FIT 中已验证的两个载荷是同一组字节；物理读取源虽未在该片段中明确打印为 `mmcblk0p3`，但 `p3` 是得到直接交叉支持的候选。

日志中的 FIT RAM 长度 `0x02232400` 恰好止于 `p3` FIT `resource` 载荷的起始偏移；此处未显示 `resource` 被作为 Linux 启动 FIT 的一部分装入。不能据此断言该资源从未被其他代码使用。U-Boot 先尝试 Android 头和 AVB 路径但失败，随后成功走 FIT Linux 路径；这解释了系统含 Android 兼容字段但当前实际启动 Ubuntu 的现象。`FIT: no signed, no conf required` 是该次配置签名要求的日志，不能单凭这一行给出完整安全启动策略结论。

### 步骤 90：中断自动启动并读取 U-Boot 版本

执行端：R1 Debug UART 的 U-Boot Shell。学习者在 `Hit key to stop autoboot('CTRL+C')` 阶段发送 `Ctrl+C`，出现 `=>` 提示符后执行只读命令：

```text
version
```

实际输出（学习者提供）：

```text
U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1 (Sep 29 2024 - 11:10:07 +0800)

aarch64-none-linux-gnu-gcc (GNU Toolchain for the A-profile Architecture 10.3-2021.07 (arm-10.29)) 10.3.1 20210621
GNU ld (GNU Toolchain for the A-profile Architecture 10.3-2021.07 (arm-10.29)) 2.36.1.20210621
```

观察：学习者已可稳定进入第二阶段 U-Boot 的交互提示符，且 `version` 能执行并返回与启动横幅相同的版本标识。该输出还说明该 U-Boot 使用 AArch64 GNU Toolchain 10.3.1 与 GNU ld 2.36.1 构建。`dirty` 是构建时源码树有未提交改动的标识，不表示当前板端有未保存修改。此次只执行了只读命令，未改变 U-Boot 环境或 eMMC。下一步只读查看 `bootcmd`，以关联实际自动启动脚本与刚才观察的 Android/FIT 分支。

### 步骤 91：读取 U-Boot 自动启动命令

执行端：R1 U-Boot `=>` Shell。

```text
printenv bootcmd
```

实际输出（学习者提供）：

```text
bootcmd=boot_android ${devtype} ${devnum};boot_fit;bootrkp;run distro_bootcmd;
```

观察：`bootcmd` 是 U-Boot 自动启动时执行的命令序列；分号表示按顺序执行，不是“成功才继续”的条件连接。它先将当前设备类型/编号传给 `boot_android`，这与日志中的 Android/AVB 检查和失败完全对应；随后执行 `boot_fit`，这与后续成功加载 Linux FIT `conf` 对应。Linux 内核一旦启动，控制不会返回 U-Boot，因此本次没有执行到 `bootrkp` 或 `run distro_bootcmd`。若 `boot_fit` 返回失败，后两项才会成为下一候选分支。`${devtype}` 与 `${devnum}` 是在运行脚本时展开的环境变量，其当前值及两个子命令的具体定义尚待只读读取。

### 步骤 92：区分 U-Boot 环境变量与内建命令

执行端：R1 U-Boot `=>` Shell。

```text
printenv devtype devnum boot_android boot_fit
```

实际输出（学习者提供）：

```text
devtype=mmc
devnum=0
## Error: "boot_android" not defined
## Error: "boot_fit" not defined
```

观察：当前脚本的设备变量为 `mmc` 和 `0`。两条“not defined”仅说明 `boot_android`、`boot_fit` 不在**环境变量**表中，不代表它们不能执行；启动日志已经证明二者作为命令依次运行。U-Boot 解析 `bootcmd` 时会先展开 `${...}` 变量，再查找命令；因此这两个名称是该厂商 U-Boot 编译进来的命令，而不是 `run` 所调用的环境变量脚本。下一步应用 `help` 读取命令帮助，不能通过 `printenv` 获取其实现。

### 步骤 93：读取 Android 与 FIT 启动命令帮助

执行端：R1 U-Boot `=>` Shell。

```text
help boot_android
help boot_fit
```

实际输出（学习者提供）：

```text
boot_android - Execute the Android Bootloader flow.

Usage:
boot_android <interface> <dev[:part|;part_name]> <slot> [<kernel_addr>]
    - Load the Boot Control Block (BCB) from the partition 'part' on
      device type 'interface' instance 'dev' to determine the boot
      mode, and load and execute the appropriate kernel.
    - In normal and recovery mode, the kernel will be loaded from
      the corresponding "boot" partition.
    - In bootloader mode, the command defined in the "fastbootcmd"
      variable will be executed.
    - On Android devices with multiple slots, the pass 'slot' is
      used to load the appropriate kernel. The standard slot names
      are 'a' and 'b'.

boot_fit - Boot FIT Image from memory or boot/recovery partition

Usage:
boot_fit boot_fit [addr]
```

观察：`boot_android` 是完整 Android Bootloader 流程：读取 BCB 以决定普通、恢复或 bootloader 模式；其帮助说明普通/恢复模式会使用相应的 `boot` 分区。`boot_fit` 则是从内存或 `boot`/`recovery` 分区启动 FIT 的内建命令。它们共同解释了当前厂商 U-Boot 为何先探测 Android、再回退到 FIT。

注意：帮助所示 `boot_android` 用法含更多形参，而当前 `bootcmd` 只显示 `boot_android ${devtype} ${devnum}`。实际启动没有报语法错误且确实进入 Android 路径，说明该厂商实现存在默认参数或与帮助文字不同的调用约定；具体默认值待通过源代码或更详细命令追查，不能从帮助文本直接假定。下一步只读列出 U-Boot 所见 `mmc 0` 的分区表，检验它是否含与 Linux 一致的 `boot`、`recovery`、`misc` 标签。

### 步骤 94：交叉核对 U-Boot `mmc 0` 与 Linux eMMC 分区表

执行端：R1 U-Boot `=>` Shell。

```text
part list mmc 0
```

实际输出（学习者提供；下表保留全部分区的 LBA 范围、标签和 Partition GUID；Type GUID 与全零 attributes 已省略）：

```text
Partition Map for MMC device 0  --   Partition Type: EFI

Part    Start LBA       End LBA         Name
  1     0x00004000      0x00005fff      "uboot"
        guid:   70190000-0000-412d-8000-5ae500003bdf
  2     0x00006000      0x00007fff      "misc"
        guid:   a4640000-0000-4a75-8000-420d00001b32
  3     0x00008000      0x00027fff      "boot"
        guid:   7a3f0000-0000-446a-8000-702f00006273
  4     0x00028000      0x00067fff      "recovery"
        guid:   a6450000-0000-4a07-8000-7cf200005735
  5     0x00068000      0x00077fff      "backup"
        guid:   c2610000-0000-4264-8000-7eff00001135
  6     0x00078000      0x01c77fff      "rootfs"
        guid:   614e0000-0000-4b53-8000-1d28000054a9
  7     0x01c78000      0x01cb7fff      "oem"
        guid:   fb190000-0000-4870-8000-1c1800005a79
  8     0x01cb8000      0x03999fde      "userdata"
        guid:   67330000-0000-4629-8000-61730000400d
```

观察：全部八个标签、顺序和 Partition GUID 均与 Linux `/dev/mmcblk0` 的已读分区表一致，因此 U-Boot 的 `mmc 0` 就是 Linux 的当前 eMMC `mmcblk0`。这使 `boot_fit` 的“boot/recovery partition”描述可直接关联到 `mmcblk0p3` 与 `mmcblk0p4`。

早期 SPL 日志的 `Trying fit image at 0x4000 sector` 与本表 `uboot` 分区起始 LBA `0x4000` 精确相同，强力支持 SPL 读取的是此 eMMC 的 `p1=uboot` 起点。SPL 与第二阶段 U-Boot 对同一存储控制器使用的编号不同（`MMC1` 与 `mmc 0`）；不应按数字名称直接混同，而应以 GPT 的起始 LBA 和 GUID 交叉验证。此前该起点的 2560 B FDT 又未呈现标准 FIT `images/configurations` 树，二者的具体厂商封装关系仍待解释。

### 步骤 95：读取 U-Boot 环境中的启动参数前缀

执行端：R1 U-Boot `=>` Shell。

```text
printenv bootargs
```

实际输出（学习者提供）：

```text
bootargs=storagemedia=emmc androidboot.storagemedia=emmc androidboot.mode=normal
```

观察：该环境变量直接保存 eMMC 与 Android 兼容字段，但没有 `root=PARTUUID=...`、`console=...` 或 `earlycon=...`。此前反编译的 p3 FIT 内 Linux DTB 的 `/chosen/bootargs` 含后面三类参数；运行时 FDT 的同一属性则同时包含本步骤的三个环境字段和 FIT DTB 的参数。

**已验证**的是三处字符串的内容与包含关系；**推测**是第二阶段 U-Boot 在将 FIT DTB 交给内核前把环境 `bootargs` 合并进 `/chosen/bootargs`。现有输出尚未直接显示执行此操作的代码路径，因此不能据此断言具体命令或函数。下一步宜读取 U-Boot 是否定义了用于 FDT/内核地址的环境变量，为观察其加载过程做准备。

### 步骤 96：读取 U-Boot 的标准装载地址变量

执行端：R1 U-Boot `=>` Shell。

```text
printenv loadaddr fdt_addr_r kernel_addr_r ramdisk_addr_r
```

实际输出（学习者提供）：

```text
## Error: "loadaddr" not defined
fdt_addr_r=0x08300000
kernel_addr_r=0x00400000
ramdisk_addr_r=0x0a200000
```

观察：`kernel_addr_r=0x00400000` 与先前启动日志中的 `Loading Kernel Image ... to 0x00400000` 一致；`fdt_addr_r=0x08300000` 也与 `Loading fdt ... to 0x08300000` 一致。因此这两个变量是本次 Linux FIT 启动的实际装载目的地址，而不只是未使用的默认值。`ramdisk_addr_r` 给出了 initrd/ramdisk 的候选地址，但当前日志只明确加载 kernel 和 FDT，不能据此断言 ramdisk 已存在或已使用。`loadaddr` 未定义表示该厂商环境没有设置这个通用变量，不是错误或启动故障。

下一步读取 `bdinfo`，将这些地址放入 U-Boot 所见的 DRAM 范围和自身重定位区域中理解；该命令只显示信息，不会改动内存或 eMMC。

### 步骤 97：将装载地址放入 U-Boot 的 DRAM 布局

执行端：R1 U-Boot `=>` Shell。

```text
bdinfo
```

实际输出（学习者提供；仅保留与内存布局、串口和 FDT 指针有关的字段）：

```text
DRAM bank   = 0x00000000
-> start    = 0x00200000
-> size     = 0x08200000
DRAM bank   = 0x00000001
-> start    = 0x09400000
-> size     = 0xE6C00000
baudrate    = 1500000 bps
TLB addr    = 0xEFFF0000
relocaddr   = 0xEDC3A000
reloc off   = 0xEDA3A000
irq_sp      = 0xEB9FAA40
sp start    = 0xEB9FAA40
Early malloc usage: 31a0 / 80000
fdt_blob = 0000000008300000
```

观察：第一段可用 DRAM 为 `0x00200000` 至 `0x083fffff`，第二段为 `0x09400000` 至 `0xefffffff`。其间 `0x08400000–0x093fffff` 不在可用 bank 中，与先前日志的 OP-TEE 保留范围相符；低于 `0x00200000` 的区域也未列为可用 RAM。`kernel_addr_r=0x00400000` 位于第一段，`fdt_addr_r/fdt_blob=0x08300000` 位于第一段末端，`ramdisk_addr_r=0x0a200000` 位于第二段，均未落在上述空洞中。U-Boot 的重定位地址和栈位于接近第二段顶端的 `0xeb...–0xed...` 区域，亦未与这些装载地址重叠。

`fdt_blob` 指向 `0x08300000`，与 `fdt_addr_r` 相同；这证明当前 U-Boot 的工作 FDT 指针使用该地址，但尚不能仅凭该字段判定其中是启动前的板级 FDT 还是即将交接给 Linux 的 FDT。下一步只读打印其 `/chosen` 节点来查看当前工作树所含的启动参数。

### 步骤 98：读取当前工作 FDT 的 `/chosen/bootargs`

执行端：R1 U-Boot `=>` Shell。

```text
fdt print /chosen
```

实际输出（学习者提供）：

```text
No FDT memory address configured. Default at 0x08300000
chosen {
        bootargs = "earlycon=uart8250,mmio32,0xfeb50000 console=ttyFIQ0 irqchip.gicv3_pseudo_nmi=0 root=PARTUUID=614e0000-0000 rw rootwait";
};
```

观察：首行只表示没有显式为 `fdt` 子命令设置地址，因此使用默认 `0x08300000`；它不是读取失败。这里的 `bootargs` 与此前反编译的 p3 FIT 内 Linux DTB 一致，且不含环境变量中的 Android/eMMC 前缀。运行时 FDT 则含两者。故已验证：当前提示符时，该地址的工作 FDT 仍保持 FIT 中的根分区/串口参数；Android 前缀尚未出现在其中。

由此可将参数合并的时间窗口收窄为“当前 `=>` 状态之后，到 U-Boot 输出 `Starting kernel ...` 之前”。**推测**：合并发生在 `boot_fit` 或它调用的更底层 Linux 启动流程中；当前证据不能区分具体函数。下一步检查当前树是否也缺少仅在运行时 FDT 中出现的 `/memory` 节点，以进一步验证它是 fixup 前的树。

### 步骤 99：验证当前工作 FDT 缺少运行时 `/memory` 节点

执行端：R1 U-Boot `=>` Shell。

```text
fdt print /memory
```

实际输出（学习者提供）：

```text
libfdt fdt_path_offset() returned FDT_ERR_NOTFOUND
```

观察：`FDT_ERR_NOTFOUND` 表示当前 FDT 中不存在路径 `/memory`，不是内存硬件检测失败。此前已反编译的 FIT 内 Linux DTB 同样没有该节点，而运行时 FDT 明确新增了它。结合步骤 98 的 `/chosen/bootargs` 结果，已验证 `0x08300000` 当前工作树保留 FIT 内 DTB 的关键内容，尚未包含运行时补充的内存节点和 Android/eMMC 启动参数。

结论：本轮只读勘察已把“FIT 内 DTB → U-Boot 当前工作 FDT → 运行时 FDT”的差异链闭合。运行时补充的具体函数/厂商补丁仍待通过匹配版本的 U-Boot 源码或更细的启动调试确认。下一步可安全重启回正常 Linux；`reset` 只复位板子，不写入 eMMC，但会中断当前 UART 交互并重新走一次启动链。

### 步骤 100：退出 U-Boot 并恢复正常 Linux

执行端：R1 U-Boot `=>` Shell。

```text
reset
```

实际结果（学习者提供）：板子随后回到 Linux。串口启动日志和 `root@R1` 提示符的完整本次输出未单独保存。

观察：这确认本轮 U-Boot 只读查询没有破坏已知启动路径。`reset` 是易失性复位操作，不应写入 eMMC 或环境变量；本次仍不把它当作恢复流程演练或镜像可写性的证据。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 以正确波特率读取可读文本 | 出现可读日志或提示符 | 出现内核日志和 `root@R1:~#` | 通过 |
| 进入 Linux 用户空间 | 获得 Shell | Root Shell 可交互 | 通过 |
| Shell 命令返回结果 | `ls` 返回目录项 | 返回 `snap` | 通过 |
| 内核与架构可识别 | `uname -a` 返回运行中内核信息 | Linux 5.10.110，aarch64 | 通过 |
| 根文件系统来源可识别 | 挂载源可查询 | `/dev/mmcblk0p6` | 通过 |
| 根文件系统物理介质可识别 | MMC 类型可查询 | `MMC`（eMMC） | 通过 |
| 用户空间发行版可识别 | 登录欢迎信息可读取 | Ubuntu 22.04 LTS | 通过 |
| 发行版标准身份可确认 | `/etc/os-release` 可读取 | Ubuntu 22.04 LTS，Jammy，`ID=ubuntu` | 通过 |
| 默认启动目标可识别 | systemd 默认目标可查询 | `graphical.target` | 通过 |
| 内核启动参数可识别 | `/proc/cmdline` 可读取 | Android 兼容字段、eMMC、UART 控制台和 PARTUUID 根分区 | 通过 |
| 运行时设备树模型可识别 | `model` 属性可读取 | `Rockchip RK3588S EVB4 LP4X V10 Board` | 通过 |
| 运行时设备树兼容列表可识别 | `compatible` 属性可读取 | `rockchip,rk3588s-evb4-lp4x-v10` → `rockchip,rk3588` | 通过 |
| 运行时 FDT 二进制可访问 | `/sys/firmware/fdt` 存在且可列出 | root 只读，151552 字节 | 通过 |
| 运行时 FDT 格式可验证 | `file` 可解析 FDT 头 | Device Tree Blob v17，大小与文件一致 | 通过 |
| 板端 `dtc` 可直接调用 | `command -v dtc` 输出路径 | 无输出，当前 `PATH` 中不可用 | 不通过 |
| 主机 `dtc` 可直接调用 | `command -v dtc` 输出路径 | 无输出，当前 `PATH` 中不可用 | 不通过 |
| 主机 `dtc` 可运行 | `dtc -v` 返回版本 | DTC v1.8.1 | 通过 |
| 主机 `dtc` 包元数据可读取 | `pacman -Qi dtc` 返回已安装包信息 | `dtc` 1:1.8.1-1，单独指定安装，数字签名验证 | 通过 |
| 板端可用网络链路 | 存在已连通的 IP 网络接口 | `eth0` 为 `NO-CARRIER`，无可用链路 | 不通过 |
| 根文件系统 `/boot` 含启动资产 | 存在内核、DTB 或启动配置 | 目录为空 | 不通过 |
| eMMC 分区布局可识别 | 列出分区、标签、文件系统与挂载点 | 8 分区；`p1=uboot`、`p3=boot`、`p6=rootfs` | 通过 |
| `boot` 分区起始格式可识别 | `file -s` 解析起始镜像头 | DTB v17，单个 blob 声明大小 1536 字节 | 通过 |
| `p3` 起始 FDT 的用途可识别 | 字符串提供格式语义线索 | U-Boot FIT 元数据树，列出 `fdt`、`kernel`、`resource` | 通过 |
| 板端 `dumpimage` 可直接调用 | 命令返回帮助或镜像信息 | Bash 报告 `command not found` | 不通过 |
| 板端 Base64 工具可直接调用 | 输出可执行路径 | `/usr/bin/base64` | 通过 |
| FIT 元数据主机解码长度 | 解码结果为 1536 字节 | 1536 字节普通文件 | 通过（仅长度） |
| 解码后 FDT 头可识别 | `file` 识别为 1536 字节 Device Tree Blob | 学习者报告与预期相同 | 通过（格式） |
| FIT 元数据可反编译 | `dtc` 成功生成 DTS | 无错误输出，`.dts` 文件存在 | 通过 |
| FIT 载荷布局可读取 | DTS 包含位置、大小与配置引用 | `fdt`、`kernel`、`resource` 的范围和 `conf` 已读取 | 通过 |
| FIT `fdt` 载荷哈希 | 实际 SHA-256 匹配 FIT 声明 | 哈希完全一致 | 通过 |
| FIT `fdt` 的根板级标识 | 含有可识别的 `model` 与 `compatible` 文本 | 与运行时根标识文本一致 | 通过（文本匹配） |
| ZMODEM 发送端可用性 | `sz` 可执行 | Bash 报 `command not found` | 不通过 |
| 以太网物理链路 | `eth0` 具有载波 | 标志含 `UP,LOWER_UP`，不再有 `NO-CARRIER` | 通过（仅物理链路） |
| 以太网 IP 地址 | `eth0` 列出 IPv4 或 IPv6 地址 | 仅显示 `eth0 UP`，无地址 | 不通过 |
| 网络管理服务 | 可识别正在运行的管理服务 | `NetworkManager.service` 为 `active (running)` | 通过 |
| NetworkManager 以太网状态 | 查询 `eth0` 状态与连接名称 | 正在获取 IP，连接为 `Wired connection 1` | 进行中 |
| DHCP/IP 配置结果 | 等待后状态变为已连接或报告失败 | `eth0` 变为 `disconnected`，无活动连接 | 未通过 |
| 主机直连以太网接口 | 找到有物理载波的主机侧接口 | `enp108s0` 为已连接且 `UP,LOWER_UP` | 通过 |
| 主机直连口 IPv4 地址 | 直连接口列出 IPv4 地址 | `192.168.0.1/24` | 通过 |
| 主机连接 IPv4 方法 | 确认 DHCP/共享或静态配置方式 | `manual`，地址为 `192.168.0.1/24` | 通过 |
| 临时静态 IPv4 直连通信 | 板端能 ping 主机直连地址 | 2/2 回复，0% 丢包 | 通过 |
| 板端有线 IPv4 方法 | 读取 DHCP 或静态配置方式 | `auto` | 通过 |
| 共享网段地址分配 | 主机与 R1 获得同一共享子网地址 | 主机为 `10.42.0.1/24`，R1 为 `169.254.80.143/16` | 未通过 |
| 主机共享 DHCP/DNS 服务 | DHCP 67 与 DNS 53 有监听端口 | `dnsmasq` 正在监听 | 通过 |
| R1 共享服务启动后的连接状态 | `eth0` 自动重连或显示当前状态 | `disconnected`，无活动连接 | 未通过 |
| R1 主动重连后的状态 | 重新激活并请求 DHCP | `connecting (getting IP configuration)` | 进行中 |
| R1 DHCP 尝试最终状态 | 获得租约或给出失败状态 | `disconnected`，未获得租约 | 未通过 |
| DHCP 报文到达主机 | 主机抓到 R1 Discover | 多个 Discover 到达；未见 Offer | 部分通过 |
| dnsmasq 共享地址与地址池 | 读取监听地址和 DHCP 范围 | `10.42.0.1`、`10.42.0.10–254`、3600 秒 | 通过 |
| 防火墙是否允许 DHCP Discover | UDP 67 可到达 dnsmasq | UFW 命中后丢弃 191 个 UDP 67 包 | 不通过（根因确认） |
| UFW 放行后的 DHCP 回归 | R1 自动取得共享网段地址并可 ping 主机 | `10.42.0.192/24`，观察到 3 次回复 | 通过 |
| 共享 NAT 外部 IPv4 | R1 经主机访问外部 IP | 3 个请求均超时 | 不通过 |
| UFW 共享 NAT 转发 | R1 外网测试流量被转发规则允许 | 91 个包进入默认 drop 的 FORWARD 链 | 不通过（阻断点确认） |
| 最小 UFW 转发规则添加 | UFW 接受 `enp108s0` → `wlo1` 共享网段规则 | `Rule added` | 通过（配置已写入） |
| 最小 UFW 转发规则命中 | R1 外网流量计入新规则 | 0 包、0 字节 | 不通过（接口条件未匹配） |
| 板端 SSH 服务 | `ssh` systemd 单元处于活动状态 | `active` | 通过 |
| root 密码状态 | 账户是否有已设置/锁定/无密码状态 | `P`，已设置密码 | 通过 |
| sshd 有效认证策略 | root、密码和公钥策略可读取 | root/密码/公钥均允许；键盘交互关闭 | 通过 |
| 主机 SSH 公钥 | 存在可复用的 Ed25519 公钥 | `id_ed25519.pub` 存在 | 通过 |
| R1 root 现有授权密钥 | `.ssh` 与 `authorized_keys` 可读取 | 两者不存在 | 通过 |
| R1 root SSH 公钥授权 | 授权文件存在且权限安全 | `.ssh=700`，`authorized_keys=600`，均为 root | 通过（认证待测） |
| SSH 仅公钥认证 | 主机私钥可被 R1 接受 | `Permission denied (publickey,password)` | 不通过 |
| 主机与 R1 授权公钥是否对应 | 两处公钥指纹完全相同 | 均为 `SHA256:3MXA9RlxfRuO7mouBBDWxc3qh777QKVbH+6CnO1OTN0`（Ed25519） | 通过 |
| 显式指定匹配私钥的 SSH 认证 | R1 接受 `id_ed25519` | 仍为 `Permission denied (publickey,password)` | 不通过（客户端选错密钥已排除） |
| sshd 有效授权公钥路径 | 与 `/root/.ssh/authorized_keys` 一致 | `.ssh/authorized_keys .ssh/authorized_keys2` | 通过 |
| 授权路径权限与所有者 | 每一级符合 sshd 严格检查 | `/root` 为 700 但属 `youyeetoo:youyeetoo` | 不通过（高度可疑） |
| 最近 SSH 服务日志 | 含本次认证拒绝诊断或明确缺失 | 仅含服务启停和监听信息 | 未确认 |
| sshd 严格模式 | 严格路径检查开关可读取 | `strictmodes yes` | 通过（根因确认） |
| `/root` 属主修复 | 仅目录自身归 `root:root`，模式保持 700 | `drwx------ ... root root ... /root` | 通过（登录回归待测） |
| SSH 公钥登录回归 | 输出远端用户和主机名 | `root`、`R1` | 通过 |
| 完整运行时 FDT 传输与核对 | 长度、格式与哈希可确认 | 151552 字节、FDT v17、SHA-256 `51cb…d068c` | 通过 |
| FIT `fdt` 主机传输与核对 | 长度与 FIT SHA-256 可确认 | 147826 字节、SHA-256 `abd1…7546` | 通过 |
| FIT `fdt` 与运行时 FDT 字节比较 | 识别是否相同及最早差异 | `cmp exit=1`；最早差异位于 FDT 头部 | 通过（内容不同） |
| FIT 与运行时 DTS 语义比较 | 识别运行时新增/改变的类别 | 内存、显示、MAC、启动参数、logo 缓冲区等差异 | 通过（运行时树被补充） |
| 完整 DTS 差异范围 | 全部差异块可枚举 | 73 行、5 个差异块 | 通过（范围已收敛） |
| `p1=uboot` 起始格式与版本线索 | 识别 FDT 头与 U-Boot 相关文本 | 起始为 FDT v17 2560 B；全分区含 FIT 组件文字和 U-Boot 2017.09 vendor build | 通过（关联尚未定位） |
| `p1` 起始 FDT 主机提取 | 仅复制 2560 字节并核对长度 | `/tmp/r1-p1-fit.dtb` 为 2560 字节普通文件（文件名为先前假设遗留） | 通过 |
| `p1` 起始 FDT 的语义 | 判断是 FIT 索引还是硬件描述 | 为 RK3588S EVB4 平台树，无 `images`/`configurations` | 通过（FIT 假设已否定） |
| 主机 eMMC 完整备份可用空间 | 可用空间大于当前 eMMC 原始容量 | `/home` 可用 186 GiB，当前 eMMC 为约 29 GiB | 通过（后续已完成备份） |
| 完整 eMMC 备份源/目标预检 | 源为当前 eMMC，目标目录存在且空间充足 | `/dev/mmcblk0` 为 28.8 GiB；`~/Study/rk3588-backup` 位于可用 186 GiB 的 `/home` | 通过（尚未读取镜像） |
| 完整 eMMC 在线导出 | SSH 与远端 `dd` 正常完成 | `ssh/dd exit=0`，主机镜像已创建 | 通过（长度与哈希待核对） |
| eMMC 与镜像精确长度 | 两端字节数完全相同 | 均为 30924603392 字节 | 通过（哈希待核对） |
| 主机镜像 SHA-256 | 能生成可记录的内容指纹 | `62b683c11cfbc5870ccfc98e9b0b334d5e2ab88a33d1d3f83f13a384a4a938d0` | 通过（校验文件待回归） |
| 镜像校验文件回归 | 校验文件可验证当前镜像 | `sha256sum -c` 报告 `成功` | 通过 |
| FIT 内 Linux DTB 根节点 | 可读取根属性和一个别名映射 | 读取 `model`、`compatible`、地址/大小单元数及 `ethernet1` 路径 | 通过 |
| 以太网节点的 DT 描述 | 可从别名定位节点并读取匹配/寄存器/状态属性 | `ethernet@fe1c0000`、GMAC/DW MAC、`reg`、`status=okay`、`phy-mode` | 通过 |
| `eth0` 运行时驱动绑定 | ethtool 能报告网络接口驱动 | `driver: st_gmac` | 通过 |
| `eth0` 平台驱动绑定 | sysfs 驱动链接可解析 | `/sys/bus/platform/drivers/rk_gmac-dwmac` | 通过 |
| `eth0` 平台设备路径 | sysfs 设备链接可解析 | `/sys/devices/platform/fe1c0000.ethernet` | 通过 |
| `eth0` 运行时设备树兼容列表 | `of_node/compatible` 与 GMAC 节点兼容项一致 | `rockchip,rk3588-gmac`、`snps,dwmac-4.20a` | 通过 |
| `eth0` 运行时设备树状态 | `of_node/status` 可读取节点启用状态 | `okay` | 通过 |
| 早期启动链日志 | 可观察 SPL、可信固件和 U-Boot 横幅 | SPL → 早期 FIT 校验 → BL31/OP-TEE → U-Boot 已出现 | 通过 |
| U-Boot 加载 Linux FIT | 可观察配置、kernel/FDT 和哈希校验 | 选择 `conf`；kernel、fdt 大小/哈希均匹配 p3 FIT 并校验通过 | 通过（物理源分区仍未由日志直印） |
| U-Boot 交互与版本 | 能中断自动启动并执行只读命令 | 出现 `=>`；`version` 输出 U-Boot 版本和构建工具链 | 通过 |
| U-Boot 自动启动入口 | 可读取 `bootcmd` 并与日志对应 | Android → FIT → RKP → distro 的候选顺序已读取 | 通过 |
| 启动设备变量与子命令类型 | 可区分环境变量和 U-Boot 命令 | `devtype=mmc`、`devnum=0`；Android/FIT 不在环境变量表 | 通过 |
| Android/FIT 内建命令接口 | `help` 可显示命令用途 | BCB/Android 分支及 FIT 分区启动用途已读取 | 通过 |
| U-Boot MMC 与 Linux eMMC 对应 | 分区标签和 GUID 可逐项对应 | `mmc 0` 与 `/dev/mmcblk0` 的 8 个分区一致 | 通过 |

## 日志解释与边界

- **已验证**：R1 当前能够启动 Linux，且 Debug UART 工作正常。
- `rkcif-mipi-lvds*` 的 `failed -19` 出现在相机 CSI 子系统；Linux 已继续启动，因此它不是当前的启动阻塞项。仅在需要使用对应摄像头接口时再将其作为问题排查。
- **已验证**：当前根文件系统为 eMMC 上的 `/dev/mmcblk0p6`。
- **已验证**：登录欢迎信息和 `/etc/os-release` 均表明用户空间为 Ubuntu 22.04 LTS；标准文件进一步记录代号为 Jammy Jellyfish、`ID=ubuntu`、`ID_LIKE=debian`。
- **已验证**：systemd 默认启动目标为 `graphical.target`，该系统配置为默认启动图形会话。
- **已验证**：内核启动参数表明根分区以 PARTUUID 标识、早期控制台位于 UART，并含厂商保留的 Android 兼容字段。
- **已验证**：运行时设备树 `model` 为 `Rockchip RK3588S EVB4 LP4X V10 Board`，未体现 R1 商品名。
- **已验证**：运行时设备树 `compatible` 从具体板级字符串 `rockchip,rk3588s-evb4-lp4x-v10` 回退到通用 SoC 字符串 `rockchip,rk3588`；详见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：内核通过 root 只读的 `/sys/firmware/fdt` 导出运行时 FDT 二进制，大小为 151552 字节；详见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`file` 将该运行时文件识别为 Device Tree Blob version 17；其总大小、启动 CPU ID、字符串区和结构区大小均可读取；详见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：当前板端 Shell 的 `PATH` 中没有可直接调用的 `dtc`；后续 DTB 反编译优先在 Arch 主机准备工具。
- **已验证**：Arch 主机已安装 `dtc` 包 `1:1.8.1-1`，DTC v1.8.1 可执行；详细元数据见[工具记录](../tool/dtc.md)。
- **已验证**：当前 `eth0` 已具有物理载波（`UP,LOWER_UP`），说明网线及对端链路已建立；`ip -br addr` 未列出 IPv4 或 IPv6 地址，因此 IP、路由与主机传输能力尚不可用。`lo` 和 `can0` 不能作为当前主机传输通道。
- **已验证**：当前已挂载根文件系统的 `/boot` 为空；启动组件不位于该目录。
- **已验证**：当前 eMMC 有 8 个分区；`p1` 标记为 `uboot`，`p3` 标记为 `boot`，`p6` 为当前挂载的 ext4 `rootfs`。`p3` 开头是 1536 字节的 U-Boot FIT 元数据树，列出了 `fdt`、`kernel` 与 `resource` 载荷；详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)与[FIT 笔记](../note/uboot-fit-image.md)。
- **已验证**：板端当前无法直接调用 `dumpimage`；不为解析 FIT 改动板端软件环境。
- **已验证**：板端可直接调用 `/usr/bin/base64`，可用于经串口传输少量 FIT 元数据。
- **已验证**：通过 Base64 手工传输到主机的 FIT 元数据临时文件长度为 1536 字节；内容格式和完整性待主机侧 `file` 检查。
- **已验证**：主机 `file` 已重新识别该临时文件为预期的 1536 字节 Device Tree Blob；传输元数据未做独立密码学完整性校验。
- **已验证**：Arch 主机 DTC v1.8.1 已成功将该 FIT 元数据反编译为 `/tmp/r1-p3-fit.dts`。
- **已验证**：FIT 默认配置 `conf` 选择 `fdt`、`kernel` 与 `resource`；三个载荷的偏移、大小和 SHA-256 声明已读取。FIT 中 `fdt` 为 147826 字节，与 151552 字节运行时 FDT 不同；详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。
- **已验证**：`p3` 中实际 `fdt` 范围的 SHA-256 已匹配 FIT 声明；这验证了 `fdt` 的位置、长度和内容，但不验证 U-Boot 执行该检查或签名策略。
- **已验证**：FIT `fdt` 载荷包含与运行时 FDT 相同的根 `model` 和根 `compatible` 文本；但两者大小不同，尚不能断言是同一未修改 DTB。
- **已验证**：当前 Shell 不能直接调用 `sz`，因此不能假定已有 ZMODEM 大文件传输能力。
- **已验证**：`NetworkManager.service` 正在运行，是当前网络状态与 DHCP 连接配置的观察入口。
- **已验证**：NetworkManager 正在为 `eth0` 的 `Wired connection 1` 获取 IP 配置；尚未获得成功或失败结果。
- **已验证**：等待后 `eth0` 变为 `disconnected`，没有活动连接或 IP 地址；网线对端是 Arch Linux 主机，不是路由器。
- **已验证**：Arch 主机侧 `enp108s0` 为已连接且 `UP,LOWER_UP` 的以太网接口；其 IP 与共享/DHCP 角色尚待检查。
- **已验证**：Arch 主机 `enp108s0` 配置 IPv4 `192.168.0.1/24`；该地址不单独证明 DHCP 或互联网共享已启用。
- **已验证**：Arch 主机 `enp108s0` 使用 NetworkManager 的 `manual` IPv4 方法，不是 `shared` 模式；不能预期它自动分配 DHCP 地址给 R1。
- **已验证**：R1 临时配置 `192.168.0.2/24` 后可 ping 主机 `192.168.0.1`（2/2 回复）；直连 IPv4 通信已建立。
- **已验证**：R1 的 `Wired connection 1` 为 `ipv4.method=auto`，即 DHCP 客户端模式。
- **已验证**：共享配置后主机 `enp108s0` 当前为 `10.42.0.1/24`，R1 当前为 IPv4 链路本地地址 `169.254.80.143/16`；R1 尚未取得共享网段 DHCP 租约。
- **已验证**：主机共享侧 `dnsmasq` 正在监听 DHCP UDP 67 和 DNS `10.42.0.1:53`；R1 未获租约不能归因于主机 DHCP 服务未启动。
- **已验证**：主机共享 DHCP 已启动后，R1 `eth0` 仍为 `disconnected`，没有自动重新发起 DHCP 请求。
- **已验证**：主动重连后 R1 `eth0` 进入 `connecting (getting IP configuration)`；前台 `nmcli` 被 Ctrl-C 中断，但后台 DHCP/IP 流程仍在进行。
- **已验证**：当前 DHCP 尝试最终回到 `disconnected`，R1 未获得共享 DHCP 租约；详见[ISSUE-20260809-003](../issue/issue-20260809-003-r1-dhcp-lease-missing.md)。
- **已验证**：R1 DHCP Discover 已到达主机；已保存抓包未见主机 DHCPOFFER，问题聚焦于主机 DHCP 服务处理或防火墙路径。
- **已验证**：NetworkManager 启动的 dnsmasq 已配置共享监听地址和 DHCP 地址池；未发 Offer 的根因仍待查。
- **已确认根因**：主机 UFW 的默认 INPUT 丢弃策略与 UDP 67 规则丢弃了 R1 DHCP Discover；未向 dnsmasq 发出 Offer。
- **已解决**：仅放行 `enp108s0` 入站 UDP 67 后，R1 自动获得 `10.42.0.192/24` 并可 ping 主机；见[ISSUE-20260809-003](../issue/issue-20260809-003-r1-dhcp-lease-missing.md)。
- **已验证**：R1 ping `1.1.1.1` 为 3/3 超时，主机自身可 ping 外部百度 IP；共享 NAT 的外部 IPv4 连通性尚未建立。
- **已确认阻断点**：UFW `FORWARD` 默认 drop 已命中 R1 外网测试流量；详见[ISSUE-20260809-004](../issue/issue-20260809-004-ufw-blocks-shared-nat-forward.md)。
- **已验证**：最小 UFW 路由规则已添加，但 R1 外网 ping 尚未回归；规则命中和回程路径待查。
- **已验证**：该规则在 R1 外网测试后仍为 0 包、0 字节，`wlo1` 不是已证实的转发出口条件。
- **已验证**：R1 `ssh` 服务为 `active`，主机 SSH 公钥登录已回归通过；完整运行时 FDT 与 FIT `fdt` 均已在主机保存可验证副本，其长度和 SHA-256 已分别核对。完整 DTS diff 为 73 行、5 个差异块，确认运行时树含内存、显示、MAC、启动参数与 logo 缓冲区等有限补充信息。
- **已验证**：R1 root 账户状态为 `P`（已设置密码）；密码内容未知，串口自动登录不构成 SSH 认证证据。
- **已验证**：sshd 允许 root、密码与公钥认证；因密码未知，后续优先使用 SSH 公钥认证。
- **已验证**：Arch 主机已有 `~/.ssh/id_ed25519.pub`，可作为向 R1 授权的公钥；私钥不离开主机。
- **已验证**：R1 root 当前没有 `.ssh` 或 `authorized_keys`，可新建授权文件而不覆盖现有 root 公钥。
- **已验证**：主机公钥已写入 R1 root 授权文件，目录/文件权限为 700/600；SSH 公钥认证尚待主机侧验证。
- **已解决**：sshd 的 `StrictModes=yes` 会检查授权路径；`/root` 错误地由 `youyeetoo:youyeetoo` 所有，导致 root 的 `authorized_keys` 即使内容和自身权限正确仍被拒绝。仅恢复 `/root` 的属主为 `root:root` 后，主机 SSH 公钥登录回归输出 `root`、`R1`。详见[ISSUE-20260809-005](../issue/issue-20260809-005-r1-ssh-public-key-rejected.md)。
- **已验证**：`p1=uboot` 起始为 2560 字节 FDT v17；主机反编译确认它是 RK3588S EVB4 平台设备树，未见 FIT 的 `images` 或 `configurations`。全分区的可打印文本另含 FIT 组件文字和 U-Boot `2017.09-g33a7c066a8-dirty #youyeetoo1` 构建标识，但其与起始 FDT 的关系尚未定位。
- **已验证**：通过 SSH 只读提取的主机临时文件 `/tmp/r1-p1-fit.dtb` 为 2560 字节，与 `p1` 起始 FDT 大小一致，并已完成 DTC 反编译；文件名是先前 FIT 假设留下的命名，不代表其实际语义。
- **已验证**：主机 `/home` 文件系统有 186 GiB 可用空间，足以保存当前约 29 GiB eMMC 原始备份；后续已完成镜像导出和校验，厂商固件仍未下载。
- **已验证**：完整备份的源路径已复核为 R1 的 `/dev/mmcblk0`（28.8 GiB），目标目录为主机 `~/Study/rk3588-backup`；在线读取完成后长度和 SHA-256 校验均通过。在线读取根文件系统会得到块级快照，不能等同于离线文件系统一致性备份。
- **已验证**：完整 eMMC 在线只读导出命令以 `ssh/dd exit=0` 结束，主机镜像位于 `~/Study/rk3588-backup/r1-emmc-20260809.img`；长度和 SHA-256 尚待核对，不能仅凭退出码视为完整或可恢复。
- **已验证**：远端 `/dev/mmcblk0` 与主机镜像长度均为 30924603392 字节，完整导出未截断；主机 SHA-256 尚待计算和保存。在线根文件系统的一致性边界不变。
- **已验证**：主机完整镜像 SHA-256 为 `62b683c11cfbc5870ccfc98e9b0b334d5e2ab88a33d1d3f83f13a384a4a938d0`；由 `tee` 创建的 `.sha256` 校验文件尚待 `sha256sum -c` 独立核对。该值不证明在线快照具备离线一致性或恢复流程已验证。
- **已验证**：`sha256sum -c` 已成功验证当前镜像与其 `.sha256` 副文件一致；这是一份可重复校验的主机侧在线块级备份，不是已演练的恢复流程。
- **已验证**：已从 FIT 内 Linux DTB 的根节点读取 `model`、`compatible`、`#address-cells`、`#size-cells` 与 `aliases`；`ethernet1` 是指向 `/ethernet@fe1c0000` 的路径别名。它们用于描述和引用硬件节点，不直接证明某个驱动已成功绑定。
- **已验证**：FIT 内 Linux DTB 的 `ethernet1` 别名指向 `ethernet@fe1c0000`；该节点声明 GMAC/DW MAC 兼容项、64 KiB 寄存器范围、两个命名中断、时钟、`status="okay"` 和 `rgmii-rxid` PHY 接口模式。DT 启用状态不单独证明运行时网络可用。
- **已验证**：运行时 `eth0` 的 ethtool 驱动为 `st_gmac`；该事实将已启用的 GMAC DT 节点与实际网络接口驱动连接起来，但模块别名和精确匹配规则仍待检查。
- **已验证**：`/sys/class/net/eth0/device/driver` 解析为 `/sys/bus/platform/drivers/rk_gmac-dwmac`，确认底层设备已绑定该平台驱动；它与 ethtool 报告的 `st_gmac` 名称来自不同内核接口，模块细节待确认。
- **已验证**：`/sys/class/net/eth0/device` 解析为 `/sys/devices/platform/fe1c0000.ethernet`；其地址与 DTS 的 `ethernet@fe1c0000` 和 `reg` 起始地址一致，形成网络接口到设备树节点的地址关联。
- **已验证**：`/sys/class/net/eth0/device/of_node/compatible` 输出 `rockchip,rk3588-gmac` 与 `snps,dwmac-4.20a`，和已读取的 GMAC 节点兼容列表一致。由此完成“运行时设备树 compatible → 平台设备 → 已绑定平台驱动 → eth0”的本板证据链；这仍不单独定位驱动源码中的精确匹配表。
- **已验证**：`tr -d '\0' < /sys/class/net/eth0/device/of_node/status` 输出 `okay`。这确认运行时设备树将该 GMAC 节点标为启用；它与已观察到的驱动绑定是不同层次的事实，不能单独推出驱动 probe、物理链路或 DHCP 已成功。
- **待确认**：完整启动链所在介质、图形界面是否实际出画、FIT 中 `fdt` 与运行时 FDT 的关系、签名验证策略，以及 U-Boot 版本。
- 这次成功启动与先前 MaskROM 观察的关系未知，见 [ISSUE-20260807-001](../issue/issue-20260807-001-maskrom-and-linux-boot.md)。

## 唯一下一步

在 R1 目标 Linux 上读取 `/sys/class/net/eth0/device/of_node/compatible`，确认 `eth0` 平台设备引用的运行时设备树兼容字符串。目标是完成“设备树 compatible → 平台设备 → 驱动 → eth0”的证据链；命令只读。
