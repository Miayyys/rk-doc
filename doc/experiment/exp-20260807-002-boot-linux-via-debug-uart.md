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
  - "[[tool/dtc]]"
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
- **已验证**：当前 `eth0` 无物理载波（`NO-CARRIER`），`lo` 和 `can0` 均不能作为当前主机传输通道；暂不通过网络传输 FDT。
- **已验证**：当前已挂载根文件系统的 `/boot` 为空；启动组件不位于该目录。
- **已验证**：当前 eMMC 有 8 个分区；`p1` 标记为 `uboot`，`p3` 标记为 `boot`，`p6` 为当前挂载的 ext4 `rootfs`。`p3` 的开头是声明大小为 1536 字节的 DTB v17，且不等同于大小为 151552 字节的运行时 FDT；该起始 DTB 的用途、`p3` 的其余内容和运行时 FDT 的来源尚未确认。详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。
- **待确认**：完整启动链所在介质、图形界面是否实际出画、`p3` 开头小 DTB 的用途、`p3` 其余内容、eMMC 中实际运行时 DTB 的来源，以及 U-Boot 版本。
- 这次成功启动与先前 MaskROM 观察的关系未知，见 [ISSUE-20260807-001](../issue/issue-20260807-001-maskrom-and-linux-boot.md)。

## 唯一下一步

在 R1 Root Shell 执行 `dd if=/dev/mmcblk0p3 bs=1536 count=1 status=none | strings -n 3`，只读取刚由 FDT 头声明的前 1536 字节并查看其中的可打印字符串。预期会出现设备树属性名，也可能出现 `model` 或 `compatible`；无论结果如何都不修改分区。
