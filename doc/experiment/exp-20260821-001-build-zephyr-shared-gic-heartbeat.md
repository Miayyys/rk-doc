---
title: "EXP-20260821-001 构建并静态审计 Zephyr 共享 GIC 心跳候选"
type: experiment
status: verified
created: 2026-08-21
updated: 2026-08-22
tags: [rk3588, zephyr, amp, gic, psci]
related:
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[experiment/exp-20260820-002-inspect-r1-bl31-sip-entry]]"
  - "[[experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot]]"
  - "[[experiment/exp-20260821-003-build-r1-psci-cpu-on-heartbeat]]"
  - "[[status/current]]"
---

# EXP-20260821-001 构建并静态审计 Zephyr 共享 GIC 心跳候选

## 目标

为已静态划给 Zephyr 的 A55 core3 / `0x50000000`–`0x50100000` carveout 构建一个最小候选固件，并确认它不占用 Debug UART，且在共享 GIC 已由另一 OS 启用时不会重置 GIC Distributor。

本实验只验证主机侧构建、Kconfig、最终设备树和 ELF 布局；不启动次级 CPU，不加载该镜像到 R1，不调用 PSCI 或 Rockchip SiP，也不写 eMMC。

## 环境与前置条件

- 执行端：Arch Linux 主机；本次命令由 Bash 执行环境运行。
- Zephyr：v4.4.0，工作区 `build/local/zephyrproject/`；已安装的交叉工具链为 `/usr/bin/aarch64-linux-gnu-`。
- 应用源码：`src/zephyr-amp-heartbeat/`，只写 `0x500ff000` 状态页的 `AMP1` 标记和 `CurrentEL`，随后执行 `wfe` 循环。
- 输出目录：Git 忽略的 `build/local/zephyr-amp-heartbeat/`。
- R1 运行前置（来自先前实验）：候选 Linux 的 resource-DTB 已把 A55 `cpu@300` 排除，并保留 `zephyr@50000000` 的首 1 MiB；本实验不改变该状态。

## 风险与恢复

- 影响范围：只在主机生成 Zephyr 构建缓存和产物。
- 板端影响：无；未连接、加载或启动板端候选镜像。
- 恢复方法：`build/local/zephyr-amp-heartbeat/` 可完全重新生成；不包含需要提交或备份的二进制。

## 步骤与证据

### 步骤 1：审查共享 GIC 的 Zephyr 实现边界

目的与预期结果：确认 `CONFIG_GIC_SAFE_CONFIG` 的保护范围，避免把“共享 GIC”误解成“Zephyr 完全不触碰 GIC”。

在 Zephyr v4.4.0 的 `drivers/interrupt_controller/intc_gicv3.c` 中，`gicv3_dist_init()` 有如下条件：

```c
#ifdef CONFIG_GIC_SAFE_CONFIG
if (sys_read32(GICD_CTLR) &
    (BIT(GICD_CTLR_ENABLE_G0) | BIT(GICD_CTLR_ENABLE_G1NS))) {
        return;
}
#endif
```

因此，当 Distributor 已被另一 OS 启用时，Zephyr 会跳过随后会执行的全局 Distributor 禁用、SPI 清理、优先级设置和重新启用。`arm_gic_init()` 在这之后仍会调用 `__arm_gic_init()`；后者定位**当前 CPU**的 Redistributor，并调用 `gicv3_rdist_enable()` 和 `gicv3_cpuif_init()`。

观察：安全配置保护共享 Distributor，不等于完全不访问 GIC；当前 CPU 的 Redistributor/CPU interface 仍会配置。这与“core3 专属 Zephyr、Linux 先完成 GIC 初始化”的后续启动模型相容，但尚未在 R1 上验证。

### 步骤 2：配置最小心跳应用

目的与预期结果：不使用共享 Debug UART；保留 RK3588S SoC 所需的 timer/GIC 依赖，并打开 Distributor 安全配置。

`src/zephyr-amp-heartbeat/prj.conf` 的有效关键项为：

```text
CONFIG_CONSOLE=n
CONFIG_SERIAL=n
CONFIG_UART_INTERRUPT_DRIVEN=n
CONFIG_GIC_SAFE_CONFIG=y
CONFIG_MULTITHREADING=y
```

`boards/roc_rk3588_pc.overlay` 将 `uart2`（`0xfeb50000`，R1 Debug UART）设为 `status = "disabled"`。初次尝试禁用系统时钟、架构 timer 和 GIC，Kconfig 因 RK3588S timer/GIC 依赖而失败；这是有效否定证据，故最终配置保留 `CONFIG_ARM_ARCH_TIMER=y`、`CONFIG_GIC=y` 与 `CONFIG_GIC_V3=y`。

另一次构建起初因主机 `ccache` 目录只读停止；设置 `CCACHE_DISABLE=1` 后重建成功。这是主机缓存限制，不是 Zephyr 或目标代码错误。

### 步骤 3：构建候选镜像

目的与预期结果：生成 AArch64 Zephyr ELF/二进制，随后读取最终配置与布局。

实际执行（Bash）：

```bash
source build/local/zephyr-venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
export CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
export CCACHE_DISABLE=1
cd build/local/zephyrproject
west build -p always -b roc_rk3588_pc \
  /home/loser/Study/rk3588/src/zephyr-amp-heartbeat \
  -d /home/loser/Study/rk3588/build/local/zephyr-amp-heartbeat
```

实际输出（保留关键行）：

```text
-- Board: roc_rk3588_pc, qualifiers: rk3588
-- Found toolchain: cross-compile (/usr/bin/aarch64-linux-gnu-)
-- west build: building application
Memory region         Used Size  Region Size  %age Used
             RAM:         88 KB         1 MB      8.59%
Generating files from .../zephyr/zephyr.elf for board: roc_rk3588_pc/rk3588
```

构建退出码为 0。

### 步骤 4：核查最终配置、设备树与 ELF 覆盖范围

目的与预期结果：以生成结果而非应用配置文本证明 UART 已禁用、共享 GIC 的安全选项生效，且状态页不和程序映像重叠。

```bash
build_dir=build/local/zephyr-amp-heartbeat/zephyr
rg -n '^(# )?CONFIG_(SERIAL|CONSOLE|GIC_SAFE_CONFIG|GIC|GIC_V3|ARM_ARCH_TIMER) ' \
  "$build_dir/.config"
rg -n -C 7 'serial@feb50000' "$build_dir/zephyr.dts"
readelf -h "$build_dir/zephyr.elf" | rg 'Entry point'
readelf -lW "$build_dir/zephyr.elf" | rg 'LOAD'
stat -c 'path=%n size=%s' "$build_dir/zephyr.bin" "$build_dir/zephyr.elf"
sha256sum "$build_dir/zephyr.bin"
```

实际输出（关键行）：

```text
# CONFIG_SERIAL is not set
# CONFIG_CONSOLE is not set
CONFIG_ARM_ARCH_TIMER=y
CONFIG_GIC_SAFE_CONFIG=y
CONFIG_GIC=y
CONFIG_GIC_V3=y

uart2: serial@feb50000 {
        compatible = "rockchip,rk3588-uart", "ns16550";
        ...
        status = "disabled";
};

Entry point address:               0x5000100c
LOAD  0x010000 0x0000000050000000 ... 0x008020 0x016000 RWE 0x10000
path=.../zephyr.bin size=32800
path=.../zephyr.elf size=537344
040a4218e18f8db88517e2350f377767ef06131f5a94d9b85fdcf44142e666eb  zephyr.bin
```

观察：唯一可加载段的内存范围为 `0x50000000`–`0x50016000`，状态页地址 `0x500ff000` 在其外；入口仍为 `0x5000100c`。此前已审阅的 Zephyr reset 路径会从 EL2 初始化后转入 EL1，或直接从 EL1 初始化。最终 DT 禁用 UART2 且最终 Kconfig 禁用 serial/console，因此该候选不会注册该 UART 驱动或 console。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 候选镜像构建 | AArch64 RK3588S Zephyr 构建成功 | 退出码 0，RAM 88 KiB / 1 MiB | 通过 |
| Debug UART 所有权 | 不启用 serial/console/UART2 | `SERIAL`、`CONSOLE` 未设置，最终 UART2 disabled | 通过（静态） |
| 共享 GIC 防护 | 不重置已启用的 Distributor | `GIC_SAFE_CONFIG=y`；源码在 GICD 已启用时提前返回 | 通过（静态） |
| 状态页布局 | 不覆盖 `0x500ff000` | LOAD 至 `0x50016000` | 通过（静态） |
| Linux+Zephyr 并行 | 次级 CPU 成功运行且 Linux 持续运行 | 本实验未执行 PSCI/CPU 启动；后续运行时验证见 [EXP-20260821-003](exp-20260821-003-build-r1-psci-cpu-on-heartbeat.md) | 本实验边界：未验证 |

## 结论

已构建并静态审计一个可用于后续 AMP 启动验证的 Zephyr 心跳候选：它不占用 R1 Debug UART，映像不与状态页重叠，并在 GIC Distributor 已由 Linux 启用时避免全局重置。

该结论的关键条件仍待验证：`GIC_SAFE_CONFIG` 只有读取到已启用的 `GICD_CTLR` 时才跳过全局配置。因此该镜像不应由 U-Boot 在 Linux **之前**随意启动。后续 [EXP-20260821-003](exp-20260821-003-build-r1-psci-cpu-on-heartbeat.md) 已在运行时验证 Linux 完成初始化后由标准 PSCI 路径拉起 MPIDR `0x300`，并回读状态页；本实验自身仍只保留静态边界。全流程不使用 `RK_SIP_AMP_CFG`，不写 eMMC。

## 关联知识与问题

- 支持：R1 静态 CPU/内存隔离、Zephyr 次级 CPU 启动前的最小外设所有权边界。
- 限制：尚未得到 R1 实际 Linux 侧 PSCI 启动实现；也未验证共享缓存、GIC 或异常级别的板端行为。

## 后续行动

- [x] 设计并静态审计 Linux 已启动后调用标准 `PSCI_CPU_ON` 的最小启动器；运行时回归见 [EXP-20260821-003](exp-20260821-003-build-r1-psci-cpu-on-heartbeat.md)。
