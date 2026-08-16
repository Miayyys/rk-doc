---
title: "EXP-20260815-001 检查 R1 Ubuntu Camera 候选镜像"
type: experiment
status: active
created: 2026-08-15
updated: 2026-08-15
tags: [rk3588, r1, ubuntu, firmware, recovery]
related:
  - "[[status/current]]"
  - "[[resource/r1-ubuntu-camera-image-v2-v3]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
---

# EXP-20260815-001 检查 R1 Ubuntu Camera 候选镜像

## 目标

在不写入 R1 eMMC 的前提下，固定候选官方 Ubuntu camera 镜像的本机身份，并判断下一步应检查其容器格式还是分区表。

## 环境与前置条件

- 执行端：Arch Linux 主机。
- 硬件及版本：候选文件名 `R1_UbuntuCamera_ImageV2V3.img`；学习者说明其为 R1 官方镜像。
- 本地位置：`/home/loser/Study/rk3588-backup/`。
- 操作前状态：当前 R1 eMMC 已有独立的在线块级备份；候选镜像尚未烧录。

## 风险与恢复

- 影响范围：仅读取主机镜像文件元数据和内容头，不访问开发板或块设备。
- 备份：现有 eMMC 备份不受影响。
- 恢复方法：不需要。

## 步骤与证据

### 步骤 1：固定文件大小、通用格式与哈希

目的：排除空文件或不完整下载，并得到以后可复验的内容指纹。预期得到普通文件、非零大小和 SHA-256；通用 `file` 不能识别格式也属于有效结果。

```sh
# Arch Linux 主机
stat -c 'path=%n; size=%s bytes; type=%F; modified=%y' \
  /home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img
file -s /home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img
sha256sum /home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img
```

实际输出；退出码均为 0：

```text
path=/home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img; size=9639807562 bytes; type=regular file; modified=2026-08-15 10:09:01.000000000 +0800
/home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img: data
55cd40508c70f48d05f411f9103f9cbb004456a05574e6f473007df78e2c758f  /home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img
```

观察：文件存在且非零，SHA-256 已固定。`file` 未识别格式，因此不能据此判断它是 raw eMMC 镜像、Rockchip 升级包或其他容器；也不能仅凭 `.img` 后缀和文件名认定可适配 R1 V2/V3。

### 步骤 2：读取容器魔数

目的：在不解析或写入镜像的情况下，识别文件头所属容器类别。预期若为 Rockchip 升级容器，起始字节应为 ASCII `RKFW`；raw eMMC 镜像通常不会以该魔数开头。

```fish
# Arch Linux 主机的 fish Shell
od -An -tx1 -N 64 /home/loser/Study/rk3588-backup/R1_UbuntuCamera_ImageV2V3.img
```

实际输出；退出码未记录：

```text
 52 4b 46 57 66 00 00 00 00 01 00 00 00 02 e8 07
 09 1d 0b 0e 02 38 38 35 33 66 00 00 00 c0 f1 06
 00 26 f2 06 00 04 d8 8c 3e 00 00 00 00 01 00 00
 00 00 00 00 00 00 00 48 49 02 00 00 00 00 00 00
```

观察：前四个字节 `52 4b 46 57` 对应 ASCII `RKFW`。**已验证**该文件是 Rockchip 固件容器，而不是可直接按分区表读取或以 `dd` 写入的 raw eMMC 镜像。该魔数不验证容器内各载荷的完整性、板型兼容性或可用 NPU 组件。

### 步骤 3：盘点本机 Rockchip 解析工具

目的：区分连接/烧录工具与能解析 `RKFW` 容器的工具，不安装新软件。预期 `rkdeveloptool` 可存在；`afptool` 和 `rkImageMaker` 是否存在以实际输出为准。

```fish
# Arch Linux 主机的 fish Shell
for tool in rkdeveloptool afptool rkImageMaker
    printf '%s: ' $tool
    command -v $tool; or echo missing
end
```

实际输出；退出码未记录：

```text
rkdeveloptool: /usr/bin/rkdeveloptool
afptool: missing
rkImageMaker: missing
```

观察：主机具备 `rkdeveloptool`，但它是已知的 Rockusb 设备通信工具，不能由“命令存在”推导出它可列出本地 `RKFW` 容器内容。两个常见封包/解包工具当前不在 `PATH`。随后只读搜索已下载的 `src/rkbin/`，未找到这两个名称，但找到 `src/rkbin/tools/firmwareMerger`；该文件的功能、架构和是否支持只读查看均待确认。

### 步骤 4：识别本地 `firmwareMerger` 候选

目的：判断 `rkbin` 中的同类厂商工具能否直接在当前 Arch 主机运行；不执行该程序。预期显示 ELF 架构和动态加载器。

```fish
# Arch Linux 主机的 fish Shell
file /home/loser/Study/rk3588/src/rkbin/tools/firmwareMerger
```

实际输出；退出码未记录：

```text
/home/loser/Study/rk3588/src/rkbin/tools/firmwareMerger: ELF 32-bit LSB executable, Intel i386, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux.so.2, for GNU/Linux 2.6.8, with debug_info, not stripped
```

观察：该文件是 i386 32 位动态 ELF，指定 32 位加载器 `/lib/ld-linux.so.2`。当前 Arch 主机为 x86_64，是否安装并配置 32 位动态运行环境尚未检查；文件名也不证明它支持解包。本项目不为这个未确认工具安装兼容环境，改为在烧录前读取现有 `rkdeveloptool` 的本机帮助，确认统一固件命令语法。

### 步骤 5：核对已安装 `rkdeveloptool` 的命令范围

目的：确认当前安装的设备通信工具是否提供直接处理 `RKFW` 统一固件容器的命令；只显示本机帮助，不连接或写入开发板。

```fish
# Arch Linux 主机的 fish Shell
rkdeveloptool
```

实际输出；退出码未记录：

```text
Tool Usage
ListDevice: ld
DownloadBoot: db <Loader>
UpgradeLoader: ul <Loader>
ReadLBA: rl <BeginSec> <SectorLen> <File>
WriteLBA: wl <BeginSec> <File>
WriteLBA: wlx <PartitionName> <File>
WriteGPT: gpt <gpt partition table>
WriteParameter: prm <parameter>
EraseFlash: ef
...
```

观察：帮助中有 Loader 下载/升级、LBA/分区写入、GPT 写入和擦除等低层命令，但**没有**接受完整 `RKFW` 固件容器的统一固件升级子命令（常见写法为 `uf <update.img>`）。`ul` 的参数明确是 `<Loader>`，不能据此把 `R1_UbuntuCamera_ImageV2V3.img` 当作 Loader 使用。`wl`/`wlx` 只接收一个已定位的原始载荷；`gpt` 与 `ef` 会改变 eMMC 布局或内容。因而在尚未解包且尚未有分区映射的前提下，所有这些写入命令都不适用于该完整 `RKFW` 文件。

资料交叉核对：2026-08-15 查阅的 youyeetoo [R1 eMMC 烧录页](https://wiki.youyeetoo.com/en/r1/burnemmc) 对 Ubuntu 的步骤是 GUI `RKDevTool` 中选择 “Upgrade Firmware → Firmware → Upgrade”，与本机工具集不同。页面的 macOS 附录虽出现 `rkdeveloptool wl 0`，但示例对象为 raw Armbian 镜像，不是 `RKFW` 容器。因此官方页面不能为本机 `wl` 写入提供依据。

### 步骤 6：烧录完成报告，等待板端验证

目的：将学习者报告的状态变化与可复现的板端启动验证分开记录，避免把烧录客户端的完成提示直接当作系统可用结论。

实际信息：学习者报告“固件更新好了”，并说明使用的是刚下载的 `R1_UbuntuCamera_ImageV2V3.img`。烧录所用工具、命令/GUI 操作、设备模式、开始和结束时间、客户端输出尚未保存。

观察：这是**用户提供**的“已烧录”状态，而非已验证的板端启动结果。下一步从 Debug UART 读取新系统的 `os-release`、内核、根文件系统与 systemd 状态；这些只读检查可证实 eMMC 是否成功启动，但若新旧镜像版本字符串相同，不能单独证明写入文件的内容来源。

### 步骤 7：验证更新后的 eMMC 启动

目的：确认板端能够启动、根文件系统来自 eMMC，且记录新系统的基本身份与 systemd 健康状态。

```sh
# R1 Debug UART 的 root Shell
printf '%s\n' '== system =='
cat /etc/os-release
printf '\n== kernel ==\n'
uname -a
printf '\n== root ==\n'
findmnt -no SOURCE /
printf '\n== systemd ==\n'
systemctl is-system-running
```

实际输出；退出码未记录：

```text
== system ==
PRETTY_NAME="Ubuntu 22.04 LTS"
NAME="Ubuntu"
VERSION_ID="22.04"
VERSION="22.04 (Jammy Jellyfish)"
VERSION_CODENAME=jammy
ID=ubuntu
ID_LIKE=debian
...

== kernel ==
Linux R1 5.10.110 #4 SMP Sun Sep 29 10:38:13 CST 2024 aarch64 aarch64 aarch64 GNU/Linux

== root ==
/dev/mmcblk0p6

== systemd ==
degraded
```

观察：**已验证**板端目前可从 eMMC 分区 `mmcblk0p6` 启动 Ubuntu 22.04，且 PID 1 所在的 systemd 已完成系统启动。`degraded` 表示至少有一个 systemd 单元失败，不等同于无法启动。`os-release` 和 `uname` 与更新前记录相同，故本步骤不能单独证明 eMMC 内容与候选文件不同或相同；但它验证了学习者报告更新后的系统可启动。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 候选文件存在且非零 | 普通文件及大小 | 9,639,807,562 字节常规文件 | 通过 |
| 内容指纹可记录 | 生成 SHA-256 | `55cd4050…e2c758f` | 通过 |
| 文件头容器类别可识别 | 显示 Rockchip 魔数、分区表或未知字节 | 起始 ASCII 为 `RKFW` | 通过：Rockchip 固件容器 |
| 常见容器解析工具可直接调用 | `afptool` 或 `rkImageMaker` 存在 | 两者均缺失；另发现未验证的 `firmwareMerger` | 不通过，继续识别厂商工具 |
| `firmwareMerger` 可直接作为当前主机工具 | 架构与主机可直接匹配 | i386 32 位动态 ELF；运行环境/功能均未确认 | 不采用 |
| 已装 `rkdeveloptool` 可直接升级 `RKFW` | 帮助中存在统一固件子命令 | 未出现 `uf` 或等价的容器升级命令 | 不适用该镜像 |
| 候选镜像已写入 eMMC | 板端从新系统正常启动 | 学习者报告已完成烧录；来源/内容无法由版本串单独验证 | 部分通过 |
| 更新后的系统可用性 | 板端从 eMMC 成功启动并可读出身份 | Ubuntu 22.04、Linux 5.10.110、`/dev/mmcblk0p6`、systemd `degraded` | 通过，失败服务待查 |

## 结论

**已验证**：候选镜像是一个可定位的非零主机文件，且当前内容哈希已记录。

**已验证**：当前安装的 `rkdeveloptool` 不能以其已显示的命令直接升级这个完整 `RKFW` 容器。`wl`、`wlx`、`gpt` 和 `ef` 不是此处的替代方案，不能尝试。

**用户提供**：候选镜像已完成更新；实际烧录路径及板端启动结果尚未保存。

**已验证**：在更新报告后，R1 从 `/dev/mmcblk0p6` 启动到 Ubuntu 22.04 / Linux 5.10.110；systemd 仍为 `degraded`。

**待验证**：官方直接来源/厂商大小或校验值、下载是否完整、容器内载荷、R1 V2 适配性、候选文件内容与当前 eMMC 的精确对应关系、失败 systemd 单元、NPU LLM 所需组件，以及是否存在可验证来源、可在 Arch 上运行且支持该容器的厂商烧录客户端。任何后续写入均须另行核对和授权。

## 关联知识与问题

- 支持或修正的知识点：镜像文件名和扩展名不是板型兼容性或完整性的充分证据；哈希只标识当前本机文件。
- 关联资源：[R1 Ubuntu Camera 候选镜像](../resource/r1-ubuntu-camera-image-v2-v3.md)。

## 后续行动

- [ ] 列出当前失败的 systemd 单元，判断 `degraded` 是否仍由先前的厂商初始化问题造成。
