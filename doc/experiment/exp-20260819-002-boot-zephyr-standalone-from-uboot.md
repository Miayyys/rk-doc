---
title: "EXP-20260819-002 从 R1 U-Boot 独立启动 Zephyr"
type: experiment
status: verified
created: 2026-08-19
updated: 2026-08-19
tags: [rk3588, zephyr, u-boot, amp]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites]]"
  - "[[status/current]]"
---

# EXP-20260819-002 从 R1 U-Boot 独立启动 Zephyr

## 目标

验证 R1 的 Cortex-A55 是否能够运行 Zephyr 官方 RK3588 板级支持，并确定当前厂商 U-Boot 可用的最小 RAM 启动方法。

本实验只验证 U-Boot 将当前 CPU 的控制权交给 Zephyr；不验证 Linux 与 Zephyr 并行运行、CPU 隔离、共享内存或 RPMsg。

## 环境与前置条件

- 执行端：Arch Linux 主机 fish、R1 U-Boot 串口。
- 硬件：youyeetoo R1 V2 / RK3588S，Debug UART 速率 1,500,000 baud。
- Zephyr：v4.4.0，Git commit `684c9e8f32e4373a21098559f748f06915f950c9`，源码位于 Git 忽略的 `build/local/zephyrproject/zephyr/`。
- 构建目标：`roc_rk3588_pc/rk3588` 的 `samples/hello_world`。
- 工具链：主机 `/usr/bin/aarch64-linux-gnu-` cross-compile 工具链。
- 中转分区：U-Boot `mmc 0:8`，对应 Linux `/dev/mmcblk0p8` 和 `/userdata`。
- 操作前状态：eMMC 启动分区和 U-Boot 环境保持不变；Zephyr 固件只作为 `/userdata` 普通文件保存。

## 风险与恢复

- 影响范围：`booti`、`bootm` 或 `go` 会终止当前 U-Boot 会话；失败可能触发同步异常、复位或无输出。
- 持久化影响：只在 `/userdata/zephyr-test/` 新增普通文件，没有写入 `uboot`、`boot`、GPT 或 U-Boot 环境。
- 恢复方法：硬件复位或重新上电后仍从原 eMMC 启动链启动。

## 步骤与证据

### 步骤 1：构建官方 RK3588 Zephyr hello_world

目的与预期结果：先证明 Zephyr v4.4.0 可为官方 RK3588 A55 板级目标生成 AArch64 裸机映像。

首次构建成功后的关键输出（学习者提供）：

```text
Memory region         Used Size  Region Size  %age Used
           FLASH:           0 B          0 B
             RAM:         92 KB         1 MB      8.98%
        IDT_LIST:           0 B        32 KB      0.00%
Generating files from .../zephyr.elf for board: roc_rk3588_pc/rk3588
```

首次固件静态身份：

```text
zephyr.bin: Linux kernel ARM64 boot executable Image, little-endian, 4K pages
zephyr.elf: ELF 64-bit LSB executable, ARM aarch64, statically linked
size=36960 bytes
Entry point address: 0x5000100c
SHA-256: a9d3ed69d938d73f1282643dc307468f1227cf05e3b89984a4299f5fe5ffd598
```

观察：Zephyr 板级内存基址为 `0x50000000`；二进制首字 `0x14000403` 是跳到偏移 `0x100c` 的 AArch64 分支，对应 ELF 入口 `0x5000100c`。

### 步骤 2：排除不适用的 U-Boot OS 启动路径

目的与预期结果：判断厂商 U-Boot 的 `booti`/`bootm` 是否能把 Zephyr 当作 Linux Image 或 legacy standalone image 启动。

学习者先将原始 `zephyr.bin` 加载到 `0x50000000`；`booti 0x50000000` 在进入 Zephyr 前触发 U-Boot 同步异常。随后把相同 payload 封装成 legacy standalone uImage：

```text
Image Type:   AArch64 U-Boot Standalone Program (uncompressed)
Data Size:    36960 Bytes
Load Address: 50000000
Entry Point:  50000000
SHA-256: 634eda81e7500d4db268714be84638cab64f749de14dd9bad7c65472ad861cf6
```

`bootm 0x52000000` 能校验并复制 payload，随后仍在 U-Boot 内触发同步异常：

```text
Loading Standalone Program from 0x52000040 to 0x50000000 ... OK
kernel loaded at 0x50000000, end = 0x50009060
"Synchronous Abort" handler, esr 0x96000010
PC = 000000000029f428
```

分阶段执行时，`bootm start 0x52000000` 能解析映像，但 `bootm loados` 再次在 U-Boot 内异常：

```text
"Synchronous Abort" handler, esr 0x96000010
PC = 00000000002261b0
```

观察：两处 PC 都属于 U-Boot，串口没有 Zephyr banner；因此这些失败不能归因于 Zephyr 应用代码。完整寄存器与栈输出未另存附件，本记录仅保留可定位阶段和 PC 的原始摘录。

### 步骤 3：让 Zephyr 接管 EL2 的 D-cache/MMU 状态

目的与预期结果：厂商 U-Boot 未提供 `dcache`、`icache` 或 `bootelf` 命令，因此检查 Zephyr 自身是否能在从 EL2 启动时清理继承状态。

Zephyr `arch/arm64/core/reset.c` 的 `z_arm64_el2_init()` 在 `CONFIG_ARM64_BOOT_DISABLE_DCACHE` 下会 clean/invalidate D-cache，并关闭 EL2 的 D-cache/MMU；该选项依赖 `CONFIG_ARM64_DCACHE_ALL_OPS`。首次固件没有启用这两项，因此以额外配置重新构建：

```text
CONFIG_ARM64_DCACHE_ALL_OPS=y
CONFIG_ARM64_BOOT_DISABLE_DCACHE=y
```

实际 `.config` 验证：

```text
85:CONFIG_ARM64_DCACHE_ALL_OPS=y
86:CONFIG_ARM64_BOOT_DISABLE_DCACHE=y
```

新固件静态身份：

```text
size=36960
Entry point address: 0x5000100c
SHA-256: 782af16b0c0c7e6a702518d787d0abe29ae9157694136022a807e3e93acd4ad5
```

板端 `/userdata/zephyr-test/zephyr-hello-rk3588-el2.bin` 的 SHA-256 与主机一致。

观察：这里只证明成功固件启用了启动清理；因为没有用原始固件执行同样的 `go` 对照实验，不能断言这两个选项是成功的唯一原因。

### 步骤 4：通过 `go` 直接交接控制权

目的与预期结果：避开会在厂商 U-Boot 内异常的 OS/legacy image 处理代码，只把原始映像载入其链接地址并跳转。

```text
=> ext4load mmc 0:8 0x50000000 /zephyr-test/zephyr-hello-rk3588-el2.bin
=> md.l 0x50000000 1
=> go 0x50000000
```

实际启动输出（学习者提供）：

```text
## Starting application at 0x50000000 ...
*** Booting Zephyr OS build v4.4.0 ***
Hello World! roc_rk3588_pc/rk3588
```

观察：U-Boot 成功跳到 `0x50000000`，Zephyr 执行首条分支进入 `0x5000100c`，并通过板级 UART 输出 banner 与 hello_world。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| Zephyr AArch64 构建 | 生成 RK3588 裸机映像 | 36,960 B，入口 `0x5000100c` | 通过 |
| 板端文件完整性 | 主机与板端哈希一致 | 均为 `782af16b...acd4ad5` | 通过 |
| 厂商 U-Boot `booti`/`bootm` | 若适用则进入 Zephyr | 在 U-Boot 内同步异常 | 不适用 |
| U-Boot `go` | 从链接地址直接执行 | 显示 Zephyr v4.4.0 与 Hello World | 通过 |
| Linux+Zephyr 并行运行 | 本实验不验证 | `go` 已把当前 CPU 控制权交给 Zephyr，Linux 未启动 | 未验证 |

## 结论

R1 的 Cortex-A55、官方 Zephyr v4.4.0 RK3588 板级支持、`0x50000000` 链接布局和 UART 输出链已经完成一次可复现的独立启动验证。当前厂商 U-Boot 的 `booti`/`bootm` 路径会在进入 Zephyr前异常；经 `ext4load` 加载原始映像后使用 `go 0x50000000` 可成功启动。

这只是 AMP 的远端固件可执行性前置证明。它没有从 Linux 移除 CPU，没有为 Zephyr 保留 Linux 物理内存，也没有让另一 CPU 同时启动 Linux，更没有验证 mailbox/RPMsg。

## 关联知识与问题

- 支持的技术方向：[Linux+Zephyr AMP 长期方向](../decision/dec-20260810-002-linux-zephyr-amp-long-term-direction.md)。
- 前置资源盘点：[EXP-20260817-001](exp-20260817-001-inventory-r1-amp-runtime-prerequisites.md)。

## 后续行动

- [ ] 基于已验证的 Linux 5.10.252 候选，形成仅用于静态检查的 AMP DTS 草案：从 Linux CPU 拓扑移除 `cpu_l3`，同步调整 ARM PMU affinity，并为 Zephyr 的 `0x50000000`–`0x50100000` 预留 1 MiB `no-map` 内存；只编译和反编译核对，不启动、不写 eMMC。
