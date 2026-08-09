---
title: "EXP-20260807-001 建立 R1 Debug UART 主机端连接"
type: experiment
status: verified
created: 2026-08-07
updated: 2026-08-07
tags: [rk3588, r1, uart, serial]
related:
  - "[[status/current]]"
  - "[[environment/hardware]]"
---

# EXP-20260807-001 建立 R1 Debug UART 主机端连接

## 目标

确认 Arch Linux 识别 USB 转串口模块，并在不向板子发送数据的前提下建立 Debug UART 监听条件。

## 环境与前置条件

- **用户提供**：R1 Debug UART 已接线到 USB 转串口模块。
- **用户提供**：主机 `ls /dev` 中出现了一个 `ttyUSB*` 节点；模块芯片与当前用户访问权限尚待确认。
- 曾观察到板卡通过 Type-C 处于 Rockchip MaskROM，见 [EXP-20260805-001](exp-20260805-001-identify-rockusb-device.md)。之后的上电操作已启动 Linux，见 [EXP-20260807-002](exp-20260807-002-boot-linux-via-debug-uart.md)。

## 步骤与证据

### 步骤 1：确认设备节点与权限

执行端：Arch Linux 主机；当前目录：任意。

```bash
ls -l /dev/ttyUSB*
```

目的：确定实际设备名（通常如 `/dev/ttyUSB0`）以及当前用户是否具备打开串口的权限。

实际输出：

```text
crw-rw---- 188,0 root 7 8月  19:37 󰡯 /dev/ttyUSB0
```

观察：设备节点为 `/dev/ttyUSB0`；首字符 `c` 表明它是字符设备。该输出的列顺序未清楚显示所属组，因此不能据此判断当前用户权限。

### 步骤 2：明确读取设备类型和归属

执行端：Arch Linux 主机；当前目录：任意。

```bash
stat -c 'path=%n; type=%F; mode=%A (%a); owner=%U; group=%G; major=%t; minor=%T' /dev/ttyUSB0
```

目的：绕开 `ls` 的别名或显示格式，明确读取设备类型、所有者、所属组、权限和主/次设备号。

实际输出：

```text
path=/dev/ttyUSB0; type=字符特殊文件; mode=crw-rw---- (660); owner=root; group=uucp; major=bc; minor=0
```

观察：`type=字符特殊文件` 确认它是字符设备。权限 `660` 表示 `root` 与 `uucp` 组成员可读写；`bc` 是十六进制主设备号（即十进制 188），次设备号为 0。

### 步骤 3：确认当前用户的串口权限

执行端：Arch Linux 主机；当前目录：`~`。

```bash
id -nG
```

实际输出：

```text
loser uucp input wheel
```

观察：当前用户 `loser` 属于 `uucp` 组，因此可直接以普通用户打开 `/dev/ttyUSB0`，无需 `sudo`。

### 步骤 4：确认串口终端程序

执行端：Arch Linux 主机；当前目录：任意。

```bash
picocom /dev/ttyUSB0 -b 150000
```

目的：打开主机串口终端并检查实际配置。

实际输出（节选）：

```text
picocom v3.1
port is        : /dev/ttyUSB0
flowcontrol    : none
baudrate is    : 150000
parity is      : none
databits are   : 8
stopbits are   : 1
Terminal ready
```

观察：`picocom v3.1` 已安装且能打开 `/dev/ttyUSB0`。但实际波特率为 `150000`，少了一个 `0`；R1 Debug UART 的资料参数是 `1500000`，因此本次监听配置无效。终端未收到输出不构成接线故障证据，因为板卡当前也处于 MaskROM。

### 步骤 5：以正确参数打开监听

执行端：Arch Linux 主机；当前目录：任意。

先在当前 `picocom` 会话按 `Ctrl-A`，再按 `Ctrl-X` 退出。然后执行：

```bash
picocom -b 1500000 /dev/ttyUSB0
```

目的：以 R1 Debug UART 的官方 1500000 baud、8N1、无流控参数打开被动监听。

实际输出：待补充。

后续结果：以 `1500000` 重新打开 `picocom` 后，学习者重新上电并收到可读启动日志，且获得 `root@R1:~#` 交互 Shell。可读日志和交互提示符同时验证了波特率、接线和串口收发可用。

## 预期与边界

- **已确认**：存在字符设备节点 `/dev/ttyUSB0`。
- **已确认**：`/dev/ttyUSB0` 的所有者为 `root`，所属组为 `uucp`，且该组具有读写权限。
- **已确认**：当前用户 `loser` 属于 `uucp`，具备普通用户串口访问权限。
- **已确认**：`picocom v3.1` 可打开 `/dev/ttyUSB0`，且以 1500000 baud 成功接收 R1 启动日志并支持交互。
- 本步骤只读取主机 `/dev` 目录，不会对开发板发送数据或写入 eMMC。
- 即使节点存在，也还不能证明 RX/TX/GND 接线、波特率或板端启动输出正确。

## 唯一下一步

串口监听已建立；后续在目标 Linux 中执行只读系统识别命令，建立软件环境基线。

## 参考资料

- youyeetoo [R1 Serial port debugging](https://wiki.youyeetoo.com/en/r1/uartdebug)，访问于 2026-08-07。该文指定 R1 背面 UART DEBUG 的参数为 1500000 baud，并说明 TX/RX 交叉且必须连接 GND。
