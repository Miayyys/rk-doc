---
title: "EXP-20260821-002 构建 R1 PSCI 次级核状态预检驱动"
type: experiment
status: verified
created: 2026-08-21
updated: 2026-08-21
tags: [rk3588, amp, psci, kernel, device-tree]
related:
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[experiment/exp-20260820-002-inspect-r1-bl31-sip-entry]]"
  - "[[experiment/exp-20260821-001-build-zephyr-shared-gic-heartbeat]]"
  - "[[status/current]]"
---

# EXP-20260821-002 构建 R1 PSCI 次级核状态预检驱动

## 目标

为 Linux 已让出的 R1 A55 core3（MPIDR `0x300`）构建一个最小、内建的平台驱动。它只调用标准 PSCI `AFFINITY_INFO` 查询该核状态，为之后是否可以安全设计 `PSCI_CPU_ON` 原型收集板端证据。

本实验验证主机侧源码、配置、DTB、内核链接和 resource-DTB RAM FIT 封装。没有传输候选、启动 R1、调用 PSCI/SiP 或写 eMMC。

## 环境与前置条件

- 执行端：Arch Linux 主机；命令在 Bash 环境执行。
- 内核树：`src/rockchip-linux-kernel-r1-dts-port` 的 `study/r1-dts-port` 工作树；基础为 Rockchip `develop-5.10` 的 RKNPU 0.9.8 候选。
- 输出目录：Git 忽略的 `build/kernel-r1-dts-port/`。
- 已验证的 RAM 候选 resource-DTB 会让 Linux 排除 `cpu@300`，并保留 `zephyr@50000000`；本实验不改变这一已验证的资源划分。
- 已知限制：不能使用 `rockchip_amp` 的 `amp-cpus` 路径，因为它会调用尚未取得 R1 参数契约证据的 `RK_SIP_AMP_CFG (0x82000022)`。

## 风险与恢复

- 影响范围：只修改本机候选内核源码和 Git 忽略的构建产物。
- 板端影响：无；未连接、加载或执行新内核。
- 恢复方法：本机源码改动可由 Git 检查和回退；`build/kernel-r1-dts-port/` 可重新生成。没有写入 p1、p3、rootfs 或 U-Boot 环境。

## 步骤与证据

### 步骤 1：选择内核内建而非外置模块的集成点

目的与预期结果：确认可复用内核已有 PSCI 接口，且不为了实验导出或调用 Rockchip 私有 SiP。

ARM64 正常次级 CPU 路径使用 `psci_ops.cpu_on()`；`psci_ops` 没有可供外置 `.ko` 使用的导出符号。因此新增内建 platform driver `drivers/soc/rockchip/r1_amp_psci_probe.c`，通过 DTS 节点绑定后读取 `target-mpidr`。

驱动安全边界的源码扫描实际输出为：

```text
39: if (of_property_read_u64(dev->of_node, "target-mpidr", &mpidr))
47: if (!psci_ops.affinity_info)
51: state = psci_ops.affinity_info(mpidr, 0);
```

没有 `cpu_on`、`sip_smc`、`ioremap` 或 `memremap` 匹配项。驱动拒绝不是 `0x300` 的 MPIDR，`level=0` 查询目标 CPU 本身状态。

### 步骤 2：加入可复现的 Kconfig 与 R1 AMP DTB 节点

目的与预期结果：让 probe 是显式受控的内建配置，而不是修改默认 AMP 驱动行为。

新增 Kconfig `CONFIG_ROCKCHIP_AMP_PSCI_PROBE`，依赖 `ROCKCHIP_AMP && ARM64 && ARM_PSCI_FW`；`rk3588_linux.config` 启用它。`rk3588s-yyt-amp.dts` 新增：

```dts
amp-psci-probe {
        compatible = "youyeetoo,r1-amp-psci-probe";
        target-mpidr = /bits/ 64 <0x300>;
        status = "okay";
};
```

同时将 fragment 中的 `CONFIG_MALI_CSF_SUPPORT` 明确关闭。原因是该源码快照缺少 `drivers/gpu/arm/bifrost/mali_csffw.bin`，而当前候选 DTS 本就不使用 Mali；关闭 CSF 使该构建输入可复现，不改变 PSCI probe 的行为。

### 步骤 3：以直接 GCC、单线程完成内核链接

目的与预期结果：在主机 GCC 16 下生成含新 probe 的 `vmlinux`、裸 `Image` 与 AMP DTB。

最初的并行构建在多个互不相关目录报 `fixdep: error opening file ... .o.d`。这发生在旧 Rockchip 5.10 的 `scripts/gcc-wrapper.py` 与当前工具链组合中；直接 `aarch64-linux-gnu-gcc` 的单目标验证通过，单线程完整构建也通过。因此本次实际使用直接 GCC 与 `-j1`，不把并行问题误判为 PSCI 源码错误。

实际构建的关键命令：

```bash
make -C src/rockchip-linux-kernel-r1-dts-port \
  O=build/kernel-r1-dts-port ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- CC=aarch64-linux-gnu-gcc -j1 \
  vmlinux arch/arm64/boot/Image rockchip/rk3588s-yyt-amp.dtb
```

完整构建在 `LD vmlinux`、`SORTTAB vmlinux`、`SYSMAP System.map` 后以退出码 0 结束。由于该树同次多目标的 `Image` 依赖判断没有感知刚重链的 `vmlinux`，随后按其 `.Image.cmd` 的同一命令从新 `vmlinux` 重新导出 Image：

```bash
aarch64-linux-gnu-objcopy -O binary -R .note -R .note.gnu.build-id \
  -R .comment -S build/kernel-r1-dts-port/vmlinux \
  build/kernel-r1-dts-port/arch/arm64/boot/Image
```

### 步骤 4：核验内建符号、DTB 与产物身份

目的与预期结果：用最终链接结果证明驱动被纳入内核，且 DTS 传入正确 MPIDR。

实际输出（关键行）：

```text
ffffffc00872b9a0 t r1_amp_psci_probe
ffffffc0093ac260 d r1_amp_psci_probe_of_match
ffffffc009a26a10 t r1_amp_psci_probe_driver_init
ffffffc00a279ea8 d r1_amp_psci_probe_driver

youyeetoo,r1-amp-psci-probe
0 300
okay

CONFIG_ARM_PSCI_FW=y
CONFIG_ROCKCHIP_AMP=y
CONFIG_ROCKCHIP_AMP_PSCI_PROBE=y
# CONFIG_MALI_CSF_SUPPORT is not set

7cd692d9a63874f47e74d4e30e9f69d6b063f8a6bcbe50c701181f72f9880881  Image
e8c231153cc897a583a7ca55dc7021ad2c657519f8603ebca69407ac5206f44b  rk3588s-yyt-amp.dtb
```

最终 Image 是 37,675,520 B 的 ARM64 boot Image；AMP DTB 是 233,426 B 的 FDT v17。`fdtget -tx` 的 `0 300` 是单个 64-bit 值 `0x300` 的两个 32-bit 单元显示。

### 步骤 5：重建 resource 并封装 RAM-only FIT

目的与预期结果：沿用已验证的 resource 覆盖路径，仅替换 `rk-kernel.dtb`，保留两个 logo，再把新内核、DTB 与 resource 封入可由 R1 U-Boot 从 RAM 加载的 FIT。

原 resource 用厂商 `resource_tool` 解包到独立目录；把新 AMP DTB 安装为 `rk-kernel.dtb` 后，以原有条目顺序重新打包。回读核验结果：

```text
r1-resource-psci-preflight.img  724480 bytes
188b3a4a0dd71b970dbf15e62880e99a87f95293b39715618351e97d5053855f  resource
e8c231153cc897a583a7ca55dc7021ad2c657519f8603ebca69407ac5206f44b  verify/rk-kernel.dtb
e8c231153cc897a583a7ca55dc7021ad2c657519f8603ebca69407ac5206f44b  source AMP DTB
logo.bmp: identical
logo_kernel.bmp: identical
```

重建 resource 的条目偏移为 DTB `0x4`、`logo.bmp` `0x460`、`logo_kernel.bmp` `0x881`；DTB 变大导致 resource 从原来的 638,976 B 增至 724,480 B，属于预期变化。

随后以 `mkimage -E -p 0x800 -B 0x200` 封装 `r1-boot-fit-psci-preflight.img`。`dumpimage -l` 列出的三个 SHA-256 分别匹配本实验的 AMP DTB、Image 和重建 resource。最终 FIT 为 38,635,520 B，SHA-256 为：

```text
6878569e404a955ceacc05dbe5397f83461c916bdfb073dc692c62159a742678
```

外置数据布局为：FDT `0x800` / `0x38fd2`，kernel `0x39800` / `0x23ee200`，resource `0x2427a00` / `0xb0e00`。它小于 64 MiB boot 分区，若仅作 RAM 启动则不会写该分区，容量余量为 28,473,344 B。

### 步骤 6：首次 RAM 启动的阻塞与无 Mali 替代候选

目的与预期结果：执行仅 `AFFINITY_INFO` 的预检候选并取得 core3 状态；若候选在无关硬件 probe 阶段阻塞，则先按调用栈缩小原因，不能把缺失 PSCI 日志误判成 PSCI 不可用。

学习者已将首个候选 FIT（SHA-256 `6878569e…9a742678`）传入 `/userdata/r1-ram-boot-test/` 并完成板端哈希核验。U-Boot RAM 启动后未出现 PSCI 日志，而是出现：

```text
rcu: INFO: rcu_sched self-detected stall on CPU
Task dump for CPU 2:
task:kworker/u14:1 ... Workqueue: events_unbound async_run_entry_fn
...
kbase_hwaccess_pm_powerup
kbase_backend_late_init
kbase_device_init
kbase_platform_device_probe
```

这表明阻塞的工作线程正在探测 Mali（`kbase`）GPU；RCU stall 是该线程长期运行的后果。PCIe link failure 同时出现，但不在该调用栈中。此启动没有到达 PSCI preflight，不能据此推断 `AFFINITY_INFO` 的返回值。

检查候选 DTS 后确认此前只禁用了 display DRM/HDMI，未覆盖基础 DTS 的 `gpu@fb000000` 节点。因此在 `rk3588s-yyt-amp.dts` 新增：

```dts
&gpu {
        status = "disabled";
};
```

重新 DTC 后，最终 DTB 的 `/gpu@fb000000/status` 为 `disabled`，PSCI 节点仍为 `youyeetoo,r1-amp-psci-probe` / MPIDR `0x300`。新 DTB SHA-256 为 `4bba0a0f…e26c7373`。以它重建的 resource SHA-256 为 `ead48276…04bf92d2`，两个 logo 回读不变；新的无 Mali RAM FIT SHA-256 为：

```text
ed28974559c028e67f8d7b18feef44b1bcd314859410b8cbfe0abd61e768f6f7
```

该 FIT 仍为 38,635,520 B，三项由 `dumpimage` 静态核验。

学习者随后实际从 RAM 启动了这个无 Mali 候选。板端显示 `uname -r` 为 `5.10.252`、`nproc` 为 `7`；运行时设备树同时存在 `zephyr@50000000` 和 `amp-psci-probe`，且 platform device 已绑定至 `/sys/bus/platform/drivers/r1-amp-psci-probe`。这证明无 Mali resource-DTB、预检 DT 节点和内建驱动都已进入运行中的 Linux。此前用 `dmesg | grep -E 'PSCI affinity|RKNPU.*0\\.9\\.8|kbase'` 未取得预检的 `dev_info` 文本；仅凭绑定不能判定其返回值，因为驱动把 PSCI 错误码也作为已记录状态后返回成功。

### 步骤 7：把单次查询结果导出为只读 sysfs 属性

目的与预期结果：不依赖易被启动日志淹没的 `dev_info`，让学习者从运行中 Linux 直接读取那一次 `AFFINITY_INFO` 的原始返回值。

`r1_amp_psci_probe.c` 新增 `affinity_state` 属性：probe 仍只执行一次 `psci_ops.affinity_info(mpidr, 0)`，缓存 MPIDR 与整数返回值；属性读取只格式化缓存，不会再次调用 PSCI。没有新增 `CPU_ON`、SiP、内存映射或 Zephyr 启动代码。该驱动对象已以直接 GCC / `-j1` 重新编译，完整 `vmlinux` 重链成功；新的 Image SHA-256 为 `f9a34b3a87a2610a4ab25cd6d778da19bff5901e5f2753203bed5dae6b5ee20c`，DTB 保持 `4bba0a0f…e26c7373`。

resource 回读的 `rk-kernel.dtb` 与 DTB SHA-256 一致，两个 logo 的 `cmp` 通过。新的 RAM-only FIT 仍为 38,635,520 B，`dumpimage` 的三个输入哈希均匹配，整体 SHA-256 为：

```text
72bfee619dccf8a9a427411c68db63112c6381cdeee832610fd396910dad695b
```

学习者将该新 FIT 作为 RAM-only 候选启动后，读取结果为：

```text
mpidr=0x300 level=0 state=off (1)
```

这直接证明当前 R1 BL31 对标准 PSCI `AFFINITY_INFO(MPIDR=0x300, level=0)` 返回 `OFF`。它是设计下一次 `PSCI_CPU_ON` 最小实验的前置条件，不等价于 CPU_ON 已成功，也不证明 Zephyr 已在次级核运行。该次运行未写 eMMC。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| PSCI 调用边界 | 仅查询 `AFFINITY_INFO` | 仅见 `psci_ops.affinity_info(mpidr, 0)` | 通过（静态） |
| 私有 AMP SiP | 不调用 `0x82000022` | 未见 `sip_smc` 或 `amp-cpus` 变更 | 通过（静态） |
| 目标 CPU | 只允许 A55 core3 / `0x300` | DTB 为 64-bit `0x300`，驱动拒绝其他值 | 通过（静态） |
| 驱动集成 | 内建到候选内核 | `vmlinux` 有 probe/OF/initcall 符号 | 通过 |
| 候选二进制 | 新 Image 与 AMP DTB 可识别 | Image/DTB 及 SHA-256 如上 | 通过（主机） |
| resource 回归 | 新 DTB 到 resource 的 `rk-kernel.dtb`，logo 不变 | DTB 哈希一致，两个 logo 的 `cmp` 通过 | 通过（主机） |
| RAM FIT | 三个输入可由 FIT 正确引用且小于 boot 容量 | `dumpimage` 哈希匹配，38,635,520 B / 64 MiB | 通过（主机） |
| 首个 RAM 启动 | 执行 PSCI 状态查询 | Mali `kbase` probe 导致 RCU stall，未到达 PSCI | 未通过；原因已定位 |
| GPU 排除修复 | 最终 AMP DTB 不绑定 Mali | `/gpu@fb000000/status = disabled` | 通过（静态） |
| 无 Mali 候选的板端进入 | 运行时 DTS/driver 生效 | 5.10.252、7 CPU、两个 DT 节点存在且 driver 已绑定 | 通过 |
| R1 PSCI 返回值 | 获取 core3 的实际状态 | `mpidr=0x300 level=0 state=off (1)` | 通过（板端） |

## 结论

已完成 Linux-after-boot PSCI 启动路径的**预检阶段**：候选内核含一个固定目标、只做标准 `PSCI_AFFINITY_INFO` 的 platform driver；它没有 CPU 启动、共享内存映射或 Rockchip 私有 SiP 调用。

已确认 R1 的 PSCI 对 Linux 已让出的 core3 返回 `OFF (1)`。这不证明 `PSCI_CPU_ON` 可行，也不证明 Zephyr 可在次级核运行。首个 RAM 候选曾被无关 Mali probe 阻塞；无 Mali候选和 sysfs 可观测性修复后已获得状态值。下一阶段应另开实验：Linux 只对同一 MPIDR 发出一次标准 `CPU_ON`，入口固定为已静态审计的 Zephyr 心跳映像，并从 Linux 读取其共享状态页；再次审计后才可上板。不得加入 Rockchip 私有 SiP。

## 关联知识与问题

- 支持：Linux 已启动后通过标准 PSCI 查询 A55 core3 状态的最小实现边界。
- 关联：R1 resource-DTB 会覆盖 FIT 内 DTB 的启动行为；AMP BL31 的私有 SiP 参数契约仍未证实。
- 构建限制：旧 BSP 在本机 GCC 16 下的并行 build 有 `.o.d` / `fixdep` 不稳定问题，后续应独立排查，不能据此推定板端问题。

## 后续行动

- [x] 从 sysfs 版无 Mali RAM FIT 读取 core3 状态：`OFF (1)`。
- [ ] 新开仅一次 `PSCI_CPU_ON` 的最小心跳实验；先完成入口、共享状态页与 Linux 侧返回值读取的静态审计，再进行 RAM 启动。
