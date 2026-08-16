---
title: "EXP-20260810-001 识别运行中 Linux 的 PID 1"
type: experiment
status: verified
created: 2026-08-10
updated: 2026-08-10
tags: [rk3588, linux, systemd, userspace]
related:
  - "[[status/current]]"
  - "[[roadmap/learning-roadmap]]"
  - "[[issue/issue-20260810-001-systemd-degraded-failed-units]]"
---

# EXP-20260810-001 识别运行中 Linux 的 PID 1

## 目标

确认 R1 内核进入用户空间后，当前 PID 1 的进程名和启动参数。

## 环境与前置条件

- 执行端：R1 目标 Linux 的 root Shell。
- 硬件及版本：youyeetoo R1 V2；RK3588S；Ubuntu 22.04 / Linux 5.10.110。
- 连接方式：Debug UART。
- 操作前状态：系统已启动并可交互。

## 风险与恢复

- 影响范围：仅列出一个进程的信息，不修改进程、服务或文件。
- 备份：不需要。
- 恢复方法：不需要。

## 步骤与证据

### 步骤 1：读取 PID 1

目的：区分内核与第一个用户空间管理进程。预期可看到 PID 为 1 的 init 实现；具体实现以输出为准。

```sh
# R1 目标 Linux Shell
ps -p 1 -o pid,comm,args
```

实际输出；退出码未记录：

```text
    PID COMMAND         COMMAND
      1 systemd         /sbin/init
```

观察：`comm` 报告进程名为 `systemd`，`args` 报告其启动参数为 `/sbin/init`。这确认本系统的 PID 1 由 systemd 进程担任；`/sbin/init` 与 systemd 可执行文件的文件系统链接关系尚未单独读取。

### 步骤 2：解析 PID 1 的实际可执行文件

目的：读取内核为 PID 1 提供的 `/proc/1/exe` 链接，并解析其中的符号链接。预期它指向 systemd 的实际可执行文件；具体路径以输出为准。

```sh
# R1 目标 Linux Shell
readlink -f /proc/1/exe
```

实际输出；退出码未记录：

```text
/usr/lib/systemd/systemd
```

观察：`-f` 解析了链接链，确认正在运行的 PID 1 实际可执行文件是 `/usr/lib/systemd/systemd`。

### 步骤 3：读取 init 入口链接

目的：验证 `/sbin/init` 是否是独立可执行程序，还是指向 systemd 实现的符号链接。预期保留三个路径的实际类型；不根据文件大小推断目录链接关系。

```sh
# R1 目标 Linux Shell
ls -l /sbin/init /lib/systemd/systemd /usr/lib/systemd/systemd
```

实际输出；退出码未记录：

```text
-rwxr-xr-x 1 root root 1785544 Nov 22 04:57 /lib/systemd/systemd
lrwxrwxrwx 1 root root      20 Nov 22 04:57 /sbin/init -> /lib/systemd/systemd
-rwxr-xr-x 1 root root 1785544 Nov 22 04:57 /usr/lib/systemd/systemd
```

观察：`/sbin/init` 是指向 `/lib/systemd/systemd` 的符号链接，不是独立的 init 实现。两个 systemd 路径均显示为常规可执行文件且元数据相同；`/lib` 是否是到 `/usr/lib` 的目录链接或两者是否为硬链接，尚未验证。

### 步骤 4：读取库目录的兼容链接

目的：确认 `/lib` 与 `/usr/lib` 的目录关系，从而闭合 init 的实际路径。预期保留两者的文件类型和链接目标。

```sh
# R1 目标 Linux Shell
ls -ld /lib /usr/lib
```

实际输出；退出码未记录：

```text
lrwxrwxrwx  1 youyeetoo youyeetoo    7 Apr 19  2022 /lib -> usr/lib
drwxr-xr-x 99 youyeetoo youyeetoo 4096 Aug 23  2024 /usr/lib
```

观察：`/lib` 是相对链接 `usr/lib`，从根目录解析后即 `/usr/lib`。因此 `/sbin/init → /lib/systemd/systemd → /usr/lib/systemd/systemd`，与 `/proc/1/exe` 的运行时路径一致。输出中的目录属主为 `youyeetoo`，仅记录为厂商系统当前状态；尚未检查包管理归属或将其判为异常。

### 步骤 5：读取 systemd 的整体运行状态

目的：读取 PID 1 对系统单元状态的汇总判断，不以可获得 Shell 代替服务健康状态。预期记录实际状态；不同状态的原因须由后续失败单元列表验证。

```sh
# R1 目标 Linux Shell
systemctl is-system-running
```

实际输出；退出码未记录：

```text
degraded
```

观察：systemd 当前报告 `degraded`。这表示系统已运行但存在一个或多个失败单元；具体单元、影响范围和根因尚未读取，不能仅凭该汇总状态判断系统不可用。

### 步骤 6：列出失败单元

目的：将 `degraded` 汇总状态拆成具体对象。预期列出失败单元及其加载、激活和子状态；不对单元执行任何操作。

```sh
# R1 目标 Linux Shell
systemctl --failed --no-pager
```

实际输出；退出码未记录：

```text
  UNIT                      LOAD   ACTIVE SUB    DESCRIPTION
● apport-autoreport.service loaded failed failed Process error reports when aut…
● rockchip.service          loaded failed failed Setup rockchip platform enviro…

LOAD   = Reflects whether the unit definition was properly loaded.
ACTIVE = The high-level unit activation state, i.e. generalization of SUB.
SUB    = The low-level unit activation state, values depend on unit type.
2 loaded units listed.
```

观察：两个单元均已加载，但 `ACTIVE` 和 `SUB` 都是 `failed`。根因、两者是否相关及实际功能影响未知，已建立[ISSUE-20260810-001](../issue/issue-20260810-001-systemd-degraded-failed-units.md)跟踪。

### 步骤 7：读取板级失败服务状态

目的：在不修改服务的前提下，确认 `rockchip.service` 的执行程序、退出状态与最近日志。选择该单元是因为它的名称和描述直接涉及 Rockchip 平台环境。

```sh
# R1 目标 Linux Shell
systemctl status rockchip.service --no-pager
```

实际输出；退出码未记录：

```text
× rockchip.service - Setup rockchip platform environment
     Loaded: loaded (/lib/systemd/system/rockchip.service; enabled; vendor preset: enabled)
     Active: failed (Result: exit-code) since Wed 2023-11-22 04:57:21 CST; 1h 4min ago
    Process: 807 ExecStart=/etc/init.d/rockchip.sh (code=exited, status=2)
   Main PID: 807 (code=exited, status=2)
        CPU: 17ms

Nov 22 04:57:21 R1 systemd[1]: Started Setup rockchip platform environment.
Nov 22 04:57:21 R1 rockchip.sh[807]: /etc/init.d/rockchip.sh: line 75: warn…nput
Nov 22 04:57:21 R1 rockchip.sh[807]: It's the first time booting.
Nov 22 04:57:21 R1 rockchip.sh[807]: The rootfs will be configured.
Nov 22 04:57:21 R1 rockchip.sh[837]: tar: /rknpu2-rk3588-*.tar: Cannot open…tory
Nov 22 04:57:21 R1 rockchip.sh[837]: tar: Error is not recoverable: exiting now
Nov 22 04:57:21 R1 systemd[1]: rockchip.service: Main process exited, code…UMENT
Nov 22 04:57:21 R1 systemd[1]: rockchip.service: Failed with result 'exit-code'.
Hint: Some lines were ellipsized, use -l to show in full.
```

观察：服务脚本 `/etc/init.d/rockchip.sh` 以状态 2 退出。可见日志将直接失败点定位到 `tar` 无法打开 `/rknpu2-rk3588-*.tar`；被截断文本、该文件缺失或未匹配的底层原因以及对 NPU 功能的实际影响尚未验证。学习者要求回到 U-Boot 主线，因此不修改此服务，问题归档等待后续 NPU 阶段。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| PID 1 可读取 | 显示一个用户空间 init 进程 | `systemd`，参数为 `/sbin/init` | 通过 |
| PID 1 实际可执行文件可读取 | 解析为 systemd 的实际路径 | `/usr/lib/systemd/systemd` | 通过 |
| `/sbin/init` 入口类型可读取 | 明确是独立文件或符号链接 | 指向 `/lib/systemd/systemd` 的符号链接 | 通过 |
| `/lib` 与 `/usr/lib` 的关系可读取 | 明确是否存在目录兼容链接 | `/lib -> usr/lib` | 通过 |
| systemd 整体状态可读取 | 返回一个状态值 | `degraded` | 通过（失败单元待定位） |
| systemd 失败单元可列出 | 显示失败单元或无失败单元 | `apport-autoreport.service`、`rockchip.service` | 通过（根因待定位） |
| `rockchip.service` 直接失败点可读取 | 显示执行程序、退出状态和日志线索 | `rockchip.sh` 状态 2；`tar` 无法打开预期 tar 路径 | 通过（底层原因待验证） |

## 结论

**已验证**：本次启动的 R1 Linux 中，PID 1 的进程名是 `systemd`，其显示的启动参数为 `/sbin/init`，实际可执行文件为 `/usr/lib/systemd/systemd`；`/sbin/init` 指向 `/lib/systemd/systemd`，而 `/lib` 指向 `/usr/lib`。systemd 的整体状态为 `degraded`，当前已列出失败的 `apport-autoreport.service` 和 `rockchip.service`。后者的直接失败点是 `rockchip.sh` 中 `tar` 无法打开预期的 `/rknpu2-rk3588-*.tar`；底层原因与功能影响未验证，已归档延后。

## 关联知识与问题

- 支持或修正的知识点：Linux 内核完成早期初始化后，用户空间由 PID 1 接管。
- 关联问题：无。

## 后续行动

- [ ] 回到主机 U-Boot 源码，阅读 `evb-rk3588_defconfig`，不构建或烧录。
