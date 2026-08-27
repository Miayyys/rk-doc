---
title: "EXP-20260821-003 构建 R1 PSCI CPU_ON 心跳候选"
type: experiment
status: active
created: 2026-08-21
updated: 2026-08-22
tags: [rk3588, amp, psci, zephyr, kernel]
related:
  - "[[experiment/exp-20260821-001-build-zephyr-shared-gic-heartbeat]]"
  - "[[experiment/exp-20260821-002-build-r1-psci-affinity-preflight]]"
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[status/current]]"
---

# EXP-20260821-003 构建 R1 PSCI CPU_ON 心跳候选

## 目标

验证在 Linux 已运行、A55 core3 已由 Linux 排除且被 PSCI 报告为 `OFF` 的前提下，R1 能否经一次标准 `PSCI_CPU_ON` 启动该核至 Zephyr 心跳入口，并由 Linux 读取共享状态页确认执行。

## 环境与前置条件

- 执行端：Arch Linux 主机构建；R1 仅通过 U-Boot RAM 启动候选。
- Linux：已验证的 Rockchip 5.10.252 / RKNPU 0.9.8 headless 候选；resource-DTB 排除 `cpu@300` 并为 Zephyr 保留 `0x50000000`–`0x50100000`。
- 固件：Zephyr 心跳二进制的入口为 `0x5000100c`，大小 32,800 B；状态页为 `0x500ff000`，见[EXP-20260821-001](exp-20260821-001-build-zephyr-shared-gic-heartbeat.md)。
- 前置查询：板端 `AFFINITY_INFO(0x300, 0)` 已返回 `OFF (1)`，见[EXP-20260821-002](exp-20260821-002-build-r1-psci-affinity-preflight.md)。

## 风险与恢复

- 影响范围：候选 Linux 将显式映射已 `no-map` 的 Zephyr carveout，用于加载心跳映像、读取状态页，并在学习者写入确认命令后执行一次 `CPU_ON`。
- 不做：不写 eMMC 分区、不保存 U-Boot 环境、不调用 `RK_SIP_AMP_CFG`，不使用 `rockchip_amp` 的 `amp-cpus` 路径。
- 恢复方法：U-Boot RAM 候选失效或重启后，板卡按原 eMMC 启动；`/userdata` 中的中转文件可保留或人工删除。

## 步骤与证据

### 步骤 1：实现手动门控的加载与启动接口

目的与预期结果：创建一个内建 platform driver，提供二进制 `image` 属性和文本 `start`/`status` 属性。加载操作不得触发 CPU 启动；仅写入精确文本 `start` 时，驱动才在再次确认 `OFF` 后调用一次标准 `psci_ops.cpu_on()`。

实现新增 Kconfig `CONFIG_ROCKCHIP_AMP_PSCI_CPU_ON_HEARTBEAT` 与内建 platform driver `r1_amp_psci_cpu_on_heartbeat.c`。它从 DTS 读取固定 MPIDR、入口、映像长度、状态偏移和 `memory-region`，通过 `devm_memremap(..., MEMREMAP_WB)` 显式映射仅属于 Zephyr 的 `no-map` carveout。

驱动导出：根用户可写的二进制 `image`（仅允许顺序写到精确长度）、只读 `status`，以及仅接受文本 `start` 的 root-only `start`。`image` 完整写入后执行数据与指令 cache flush；`start` 在发起前再次查询 `AFFINITY_INFO`，仅在 `OFF` 时清空状态页并调用一次 `psci_ops.cpu_on()`。首次调用后，无论返回成功或错误，都拒绝再次启动，直到重启候选内核。

主机静态审计的调用边界为：唯一 `psci_ops.cpu_on()` 位于 `start_store()`；没有 `RK_SIP`、`sip_smc` 或 `amp-cpus` 匹配。直接 GCC / `-j1` 下对象编译、完整 `vmlinux` 重链和 DTB 构建均成功。最终 DTB 的属性为：

```text
target-mpidr=0 300
zephyr-entry=0 5000100c
zephyr-image-size=8020
status-offset=ff000
gpu-status=disabled
```

`System.map` 含启动器 probe、driver init 与 driver 数据符号。Zephyr 输入 `zephyr.bin` 为 32,800 B，SHA-256 为 `040a4218e18f8db88517e2350f377767ef06131f5a94d9b85fdcf44142e666eb`，与 DTS 的 `0x8020` 精确一致。

### 步骤 2：重建 resource-DTB RAM-only FIT

目的与预期结果：让厂商 U-Boot 仍从 resource 的 `rk-kernel.dtb` 获得本实验 DTS，同时不写任何 eMMC 分区。

以厂商 resource 解包的三个原始条目重建新 resource；新 `rk-kernel.dtb` 的回读 SHA-256 与最终 DTB 同为 `b5ec5f01e3c9015a74c5014a95b0e7f0444277b06bf038abde4a0dad59c8f9b4`，两个 logo 的 `cmp` 均通过。重建 resource 为 724,992 B，SHA-256 `5684b50d5e99d5d39416ac84924641fedd06b792f4111ef4e5a0876be382429a`。

`mkimage -E -p 0x800 -B 0x200` 与 `dumpimage -l` 已核验 FIT 的 DTB、内核、resource 三个哈希。RAM-only FIT 路径为 `build/local/r1-psci-cpu-on-heartbeat/r1-boot-fit-psci-cpu-on-heartbeat.img`，大小 38,636,544 B，SHA-256：

```text
7beb12c40e540c39afd58e06a683e000d947e4a48557de5cc493a0a7f3339ba1
```

学习者已将 FIT 与 Zephyr 二进制传至 R1 `/userdata` 根目录，板端 SHA-256 分别为 `7beb12c40e540c39afd58e06a683e000d947e4a48557de5cc493a0a7f3339ba1` 和 `040a4218e18f8db88517e2350f377767ef06131f5a94d9b85fdcf44142e666eb`，与主机一致。此步骤仅创建 userdata 文件，未写 eMMC boot 分区。FIT 不携带 Zephyr 二进制；这要求 Linux 启动后由学习者明确写入 `image` 属性，避免候选内核一启动就具备可执行的次级负载。

### 步骤 3：板端接口回归与 Zephyr 映像装载

目的与预期结果：先证明资源 DTB、启动器和权限均实际生效，再只装载固件，保持 CPU3 为 OFF。

学习者从 U-Boot RAM 启动候选后报告 `nproc` 为 `7`。启动器节点下的权限实际为：`image` 为 root-only 写入、`status` 为只读、`start` 为 root-only 写入。首次读取状态为：

```text
image=0/32800 affinity=not-queried (0) cpu_on_attempted=0 cpu_on_ret=-11 magic=0xff3d3d3dff3e3e3e current_el=18391923789735476541
```

其中状态页的 `magic` / `current_el` 尚未初始化，不能视为 Zephyr 已执行；关键边界是 `cpu_on_attempted=0`。

随后学习者执行 `cat /userdata/zephyr-heartbeat.bin > .../image`，回读为：

```text
image=32800/32800 affinity=not-queried (0) cpu_on_attempted=0 cpu_on_ret=-11 magic=0xff3d3d3dff3e3e3e current_el=18391923789735476541
```

因此 32,800 B 映像已精确写入已保留区域，同时尚未查询/启动 CPU3；未写入 eMMC、未保存 U-Boot 环境、未调用 Rockchip SiP。

### 步骤 4：一次标准 PSCI CPU_ON 调用

目的与预期结果：在已装载映像且驱动重新确认 CPU3 为 OFF 后，仅发起一次标准 PSCI 启动请求，区分固件交接与 Zephyr 应用执行。

学习者写入 `start` 后，串口出现：

```text
Secondary CPU 3 initializing
I/TC: Secondary CPU 3 switching to normal world boot
```

随后启动器状态为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x0 current_el=0
```

已验证：标准 `PSCI_CPU_ON` 被固件接受（返回 `0`），并且 TF-A/OP-TEE 已将 CPU3 交接给 normal world。`affinity=off (1)` 是驱动在发起调用**之前**的复查结果，不能用于判断调用后的 CPU 状态。

未验证：Zephyr 心跳未写入 `AMP1` / CurrentEL；状态页为零是启动器在 `CPU_ON` 前清零后的读回。因此当前阻塞位于“normal world 入口后至 Zephyr 心跳写入”这一段，可能涉及入口可见性、Zephyr 次级核启动路径或早期运行环境；尚不能将它归因为任一具体原因。

### 步骤 5：构建无运行时依赖的入口探针

目的与预期结果：去掉 Zephyr 的启动代码、GIC、UART、栈和调度器，只验证 CPU3 是否执行固定 PSCI 入口及能否使共享状态页对 Linux 可见。

新增 `src/amp-entry-probe/entry.S` 和 `linker.ld`。该探针的镜像基址仍为 `0x50000000`，入口固定为 `0x5000100c`；入口立即向 `0x500ff000` 写入 `0x414d5031`（`AMP1`）和原始 `CurrentEL`，执行 cache clean / `dsb sy` 后永久 `wfe`。它不使用私有 SiP、GIC、UART 或任何 Zephyr API。

主机构建的 `build/local/amp-entry-probe/amp-entry-probe.bin` 精确为 32,800 B，从而可复用现有 DTS/启动器的固定装载长度。其 SHA-256 为：

```text
9d34294d66c5129a057f57ca4b90e1ca505f2f2e026d5c2854e95968035bb54c
```

ELF 入口为 `0x5000100c`，唯一 `PT_LOAD` 范围从 `0x50000000` 开始、长度 `0x8020`；对应二进制偏移 `0x100c` 的首条指令已核验。该探针尚未传入板端或启动 CPU3。

学习者已将该探针传至 `/userdata/amp-entry-probe.bin`，板端 SHA-256 同为 `9d34294d66c5129a057f57ca4b90e1ca505f2f2e026d5c2854e95968035bb54c`。在重新 RAM 启动同一候选后的新启动器实例中，已完成映像装载，状态为：

```text
image=32800/32800 affinity=not-queried (0) cpu_on_attempted=0 cpu_on_ret=-11 magic=0x4000000041 current_el=1125899906842640
```

末两项是 CPU_ON 前未初始化状态页的残留，不能解释为探针执行；`start` 会在调用前清零它们。当前新实例尚未调用 CPU_ON。

### 步骤 6：裸入口探针的 PSCI CPU_ON 回归

目的与预期结果：验证 CPU3 是否执行固定入口 `0x5000100c`，并确认共享状态页的跨核可见性与进入异常级别。

学习者仅写入一次 `start` 后，TF-A/OP-TEE 再次报告 CPU3 初始化并切至 normal world。启动器立即回读为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x414d5031 current_el=8
```

`magic=0x414d5031` 是裸探针写入的 `AMP1`，不是旧状态页残留；`current_el=8` 是 AArch64 `CurrentEL` 的原始值，表示入口从 EL2 执行。由此已验证：CPU3 从 PSCI `OFF` 状态被标准 `CPU_ON` 拉起、执行到预期物理入口，并将状态同步回 Linux 映射的保留内存。该结果不表示 Zephyr 已在 CPU3 上成功运行；它把此前 Zephyr 心跳缺失收敛为 Zephyr 的 EL2 早期启动路径问题。

### 步骤 7：构建 Zephyr EL2 启动检查点候选

目的与预期结果：不改变 Zephyr 的启动逻辑，仅在最早 EL2 hook、最高异常级别 hook、EL2 init 后、EL1 init 后和 `main()` 分别写不同状态标记，以一次 CPU_ON 定位停止点。

主机只读审计显示原心跳应用的入口确为 `0x5000100c`，其路径为 `__reset_prep_c()` → `z_arm64_el_highest_init()` → `z_arm64_el2_init()` → `eret` 至 EL1 → `z_arm64_el1_init()` → `z_prep_c()`（BSS、MMU、中断）→ `z_cstart()` → `main()`。新增独立应用 `src/zephyr-amp-checkpoint/`：汇编强定义的 `z_arm64_el2_plat_prep_c()` 在尚无 C 栈时写 `EL2P`；三个 C hook 分别写 `ELHI`、`EL2I`、`EL1I`，`main()` 写 `AMP1`。所有标记均同时写入原始 `CurrentEL` 并清理缓存。

首次把该汇编 hook 放在精确 `.text` 节时，静态检查发现 ELF 入口意外移动到 `0x5000103c`，与启动器固定的 `0x5000100c` 不匹配；该候选未传板。将它移入 `.text.z_early_checkpoint` 后重建，复位向量恢复为：

```text
ELF entry: 0x5000100c
__reset / __start: 0x5000100c
z_arm64_el2_plat_prep_c: 0x500011ac
```

新 `zephyr.bin` 仍精确为 32,800 B，SHA-256 为：

```text
e7f1020fa3f81fcbf6b620e79924efb1cdda50eb6282b225190644e6a010ba52
```

构建时首次因新目录未继承 Zephyr toolchain 参数而错误查找 Zephyr SDK，随后显式复用既有 `cross-compile` / `/usr/bin/aarch64-linux-gnu-` 参数；第二次被主机只读 `ccache` 阻断，使用 `CCACHE_DISABLE=1` 后完整构建成功。两项均为主机环境问题，不是 Zephyr/板端结论。

学习者已将检查点候选传至 R1 `/userdata/zephyr-checkpoint.bin`；板端 SHA-256 为 `e7f1020fa3f81fcbf6b620e79924efb1cdda50eb6282b225190644e6a010ba52`，与主机构建产物一致。它尚未写入启动器的 `image` 属性，也未触发新的 CPU_ON；本次仅创建 `/userdata` 文件，未写 eMMC 或 U-Boot 环境。

在新启动器实例中装载后，学习者确认 `image=32800/32800`、`cpu_on_attempted=0`，再明确写入一次 `start`。启动器重新查询到 `OFF (1)`，标准 PSCI `CPU_ON` 返回 `0`，最终状态为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x454c3149 current_el=4
```

`0x454c3149` 是 `EL1I`。这证明 CPU3 已通过 Zephyr 的 EL2 初始化与 `eret`，并到达 `z_arm64_el1_init()` 尾部的应用 hook；`CurrentEL=4` 也直接证明该 hook 已在 EL1 执行。尚未观察到的是 hook 返回后的少量 reset 汇编（`SPSel`、解除 SError mask、跳转 `z_prep_c()`）以及 `z_prep_c()` 内的 SoC hook、BSS/data、MMU、中断和 `z_cstart()`。因此不能将本结果归因为某个 `z_prep_c()` 调用；下一轮应只在这个未观测区间插入更细检查点。

### 步骤 8：构建 EL1 reset 与 `z_prep_c()` 细分检查点

目的与预期结果：每次仍只运行一个 CPU_ON 实例，但把 `EL1I` 之后的未观测区间拆分为七个状态。这样下一次状态页的最后标记即可给出第一个未通过阶段，而不是猜测原因。

应用源目录新增 `host-zephyr-instrumentation.patch`，以保存对主机本地、可删除 Zephyr v4.4.0 副本的最小补丁。补丁不改 R1 内核、FIT、DTS 或板端文件：reset 汇编在 `SPSel` 和 SError 设置后写 `RST1`；`z_prep_c()` 分别在入口、SoC hook 后、TPIDR 后、BSS/data 后、MMU 后和中断后写 `P000`–`P005`。应用提供的写状态函数仍只使用保留区 `0x500ff000`，并以 `dc cvac`/`dsb sy` 向 Linux 可见。

主机从 Zephyr workspace 构建后，静态核验结果为：

```text
ELF entry: 0x5000100c
__reset: 0x5000100c
r1_amp_checkpoint_post_el1_reset: 0x500011e4
z_prep_c: 0x50002a90
zephyr.bin size: 32800 bytes
SHA-256: c7c0d6151d0421f87d06cb28460656e4653676ed1919c7c417a9e86fc1c19224
```

这证明固定启动器入口和映像大小保持不变；候选尚未传板、未装载，也未触发 CPU_ON。

学习者随后将候选传至 R1 `/userdata/zephyr-prep-checkpoint.bin`，板端 SHA-256 同为 `c7c0d6151d0421f87d06cb28460656e4653676ed1919c7c417a9e86fc1c19224`。仅创建 userdata 文件；尚未装载或启动 CPU3，未写 eMMC 或 U-Boot 环境。

在新启动器实例中，学习者已完成映像装载预检（`image=32800/32800`、`cpu_on_attempted=0`）并执行一次 `start`。启动器重新确认 `OFF (1)`，`CPU_ON` 返回 `0`，TF-A/OP-TEE 再次输出 CPU3 normal-world 交接；但状态页为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x0 current_el=0
```

这次连最早的 `EL2P` 都没有出现。它与上一轮同样入口的五阶段候选得到 `EL1I` / `CurrentEL=4` 不一致，故当前不能把零值归因为 reset 或 `z_prep_c()` 的某一步。**待验证假设**：细分插桩改变了候选映像的最早执行行为，或本轮装载/启动环境未复现先前已验证基线。下一步应以未修改的、板端仍保留的 `zephyr-checkpoint.bin` 在新实例中作单变量回归；不能在本实例重复 `start`。

对照实例装载原始五阶段候选后，启动前读到的 `P003` / `CurrentEL=4` 是保留页的旧值，不能作为该原始候选的执行证据。一次新的标准 CPU_ON 后，原始候选再次得到：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x454c3149 current_el=4
```

这复现了原始候选的 `EL1I` 基线，故没有证据表明 PSCI、固件交接或原始 Zephyr 候选回退。与此同时，`P003` 只能由细分候选写入，而它是在细分候选启动后立即读取零、下一次重启前才被看到；**推测**上一次读取发生在 CPU3 到达该标记之前。该推测尚未在同一实例延迟读取中验证，下一轮只增加一次短等待后再读取状态。

延迟读取在新的细分候选实例中得到 `P003` / `CurrentEL=4`，故上述读取竞态推测已获支持：CPU3 已执行到 `z_prep_c()` 的 BSS/data 完成点。其后的第一个调用是 `z_arm64_mm_init(true)`；因此仅在主机本地 Zephyr 副本中于该函数的入口、断言后、页表分配后、页表建立后及 MMU 启用返回后加入 `M000`–`M004`。这不改变 R1 内核、DTS、FIT 或板端文件。

### 步骤 9：构建 MMU 初始化细分候选

目的与预期结果：确认 CPU3 在 `z_arm64_mm_init(true)` 的哪个边界停止，避免直接猜测页表、内存属性或 MMU 寄存器问题。

主机重建成功，静态核验：

```text
ELF entry: 0x5000100c
__reset: 0x5000100c
z_prep_c: 0x50002a90
z_arm64_mm_init: 0x5000344c
zephyr.bin size: 32800 bytes
SHA-256: 290998fb17550429a24ed912a6724998634981d8c5c3fdb3e45bab2f2ef4fd77
```

候选尚未传板、未装载或启动 CPU3。

学习者已将该候选传至 `/userdata/zephyr-mmu-checkpoint.bin`；板端 SHA-256 同为 `290998fb17550429a24ed912a6724998634981d8c5c3fdb3e45bab2f2ef4fd77`。仅创建 userdata 文件，尚未装载或启动 CPU3，未写 eMMC 或 U-Boot 环境。

在新的启动器实例中装载该候选并执行唯一一次 `start` 后，串口再次显示 CPU3 初始化并交接至 normal world。等待后，启动器状态为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x4d303033 current_el=4
```

`0x4d303033` 是 `M003`。因此 CPU3 已在 EL1 依次完成 MMU 初始化入口、断言、一级页表分配及 `setup_page_tables()`。最初把它解释为“尚未返回 `enable_mmu_el1()`”并不充分：主机 ELF 符号显示 Zephyr 自动映射的代码/数据仅覆盖 `0x50000000`–`0x50016000`，而检查点状态页为 `0x500ff000`。`M004` 位于 `enable_mmu_el1()` 返回后，且它首先访问这页；因此**待验证的更强假设**是 MMU 实际已启用，但 `M004` 因访问未映射的状态页而异常。不能仅凭 `M003` 判定任一系统寄存器写失败或判定页表内容错误。下一轮只显式映射该 4 KiB 状态页，再读取 `M004`；读取仍须在 `start` 后等待，避免重现先前的过早读零值。

### 步骤 10：为 MMU 后检查点映射状态页

目的与预期结果：只验证“MMU 后访问状态页未映射”这一假设，不改变 CPU 启动器、Linux、DTS 或 PSCI 路径。

主机本地 Zephyr 副本的 RK3588 `mmu_regions.c` 新增单一平坦页映射：物理与虚拟地址均为 `0x500ff000`、长度 4 KiB、属性为 `MT_NORMAL | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE`。该页位于 Linux 已保留的 `0x50000000`–`0x50100000` carveout 内；应用写入后仍执行 `dc cvac` / `dsb sy`。该本地改动已同步记录在 `src/zephyr-amp-checkpoint/host-zephyr-instrumentation.patch`，不进入 R1 内核或板端存储。

主机完整重建成功。静态核验如下：

```text
ELF entry: 0x5000100c
binary size: 32800 bytes
SHA-256: 54a8a7cfdb705e16b67604227b5291879eaa185c279dbc30eb469dc813a5da7b
mapping source: R1 AMP checkpoint, 0x500ff000, 0x1000
```

候选尚未传板、尚未装载或启动 CPU3。若下一次在新的启动器实例中得到 `M004`，就证明 `enable_mmu_el1()` 已返回且原阻塞为未映射的观测页；若仍为 `M003`，才有必要继续细分该函数的系统寄存器操作。

学习者已将候选传至 `/userdata/zephyr-mmu-status-mapped.bin`；板端 SHA-256 为 `54a8a7cfdb705e16b67604227b5291879eaa185c279dbc30eb469dc813a5da7b`，与主机构建产物一致。仅创建 userdata 文件，尚未写入启动器 `image`、尚未调用 CPU_ON，未写 eMMC 或 U-Boot 环境。

在新的启动器实例中，学习者完成映像装载预检（`image=32800/32800`、`cpu_on_attempted=0`），再执行唯一一次 `start` 并等待。串口显示 CPU3 初始化并交接至 normal world；最终状态为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x414d5031 current_el=4
```

`0x414d5031` 为应用 `main()` 写入的 `AMP1`，`CurrentEL=4` 表示 EL1。因此 CPU3 已通过 Zephyr 的 MMU 初始化、中断初始化与内核启动，并到达应用 `main()`。这直接验证了前一轮 `M003` 的阻塞是 MMU 后检查点页未映射，而不是标准 PSCI CPU_ON、固件 normal-world 交接、页表建立或 `enable_mmu_el1()` 本身已被证明失败。该应用在写入 `AMP1` 后仅执行 `wfe`，故本结果是一次性到达 `main()` 的验证，尚不是实时任务、稳定性、IPC 或多核生命周期管理的验证。

### 步骤 11：构建共享页递增心跳候选

目的与预期结果：在不加入 UART、外设或 IPC 驱动的前提下，让已到达 `main()` 的 CPU3 周期性更新共享状态页；Linux 两次读取不同的计数即可证明持续并行运行与跨核可见性。

应用保留进入 `main()` 时的一次 `AMP1` 标记，随后循环写入 `0x48420000 | sequence`（`HB` 前缀和低 16 位递增序号），每次写入后保留已有 `dc cvac` / `dsb sy`。两次写入之间仅有本地 volatile NOP 延迟循环；它不依赖 Zephyr 定时器、中断、UART、外设、Linux 驱动或新 PSCI 调用。

主机重建成功，静态核验：

```text
ELF entry: 0x5000100c
binary size: 32800 bytes
SHA-256: 0636e858d13392264e7904c1c88ceeab62d439b415be26a3bc0a8909851a726b
```

候选尚未传板、未写入启动器或启动 CPU3。

学习者已将该候选传至 `/userdata/zephyr-heartbeat.bin`；板端 SHA-256 为 `0636e858d13392264e7904c1c88ceeab62d439b415be26a3bc0a8909851a726b`，与主机构建产物一致。仅创建 userdata 文件，尚未写入启动器或启动 CPU3，未写 eMMC 或 U-Boot 环境。

在新的启动器实例中，学习者确认 `image=32800/32800`、`cpu_on_attempted=0` 后执行唯一一次 `start`。固件再次输出 CPU3 初始化及 normal-world 交接；等待后连续两次读取为：

```text
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x48420001 current_el=4
image=32800/32800 affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x48420002 current_el=4
```

两次 `magic` 均有 `0x4842`（`HB`）前缀，低 16 位从 `1` 递增到 `2`；`CurrentEL=4` 保持为 EL1。因为读取发生在 Linux 上、CPU3 已被 Linux DTS 静态排除且启动器不重复 CPU_ON，这验证 Linux 与 CPU3 上 Zephyr 的持续并行执行，以及 Zephyr→Linux 的共享页 cache 可见性。心跳的循环延迟不是实时周期指标，尚不能据此评估时延、抖动、IPC 吞吐、CPU 关闭/重启或长期稳定性。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 启动器源码边界 | `CPU_ON` 只在手动门控路径中出现 | 唯一调用位于 `start_store()`；未见 SiP/`amp-cpus` | 通过（静态） |
| Linux 侧映像加载 | 仅向已保留 carveout 写入 Zephyr 二进制 | 板端已写入精确 32,800 B，且 `cpu_on_attempted=0` | 通过（运行时） |
| PSCI CPU_ON | 仅由显式 sysfs 写入触发一次 | 一次调用返回 `0`，TF-A/OP-TEE 输出 CPU3 normal-world 交接日志 | 通过（固件交接） |
| 次级 CPU 执行 | 状态页显示 `AMP1` 和 CurrentEL | 早期候选曾为 `magic=0x0`；后续映射状态页候选到达 `AMP1` / EL1，递增候选得到 `HB|1→HB|2` | 后续回归通过（早期失败保留在步骤4） |
| Linux 存活 | 启动后仍可通过 UART/SSH 读取状态 | 启动后可持续读取状态页，并得到 `HB|1→HB|2` | 通过（运行时） |
| 裸入口探针 | 不依赖 Zephyr 早期初始化地写状态页 | `AMP1` 已写回，原始 `CurrentEL=8` | 通过（运行时） |
| Zephyr 检查点候选 | 五个 EL2→EL1 阶段可由状态页区分 | 一次 CPU_ON 后为 `EL1I` / `CurrentEL=4`；EL1 hook 之前通过，之后未观测区间仍待细分 | 部分通过（运行时） |
| EL1 reset / `z_prep_c()` 细分候选 | `RST1`、`P000`–`P005` 可区分未观测路径 | 同一实例延迟读取为 `P003` / EL1，即 BSS/data 后、MMU 调用前 | 通过（运行时） |
| MMU 初始化细分候选 | `M000`–`M004` 可区分 MMU 初始化边界 | 一次 CPU_ON 后为 `M003` / EL1；页表已建立，但 `M004` 状态页不在当前自动映射范围 | 部分通过（运行时；映射假设待验证） |
| MMU 后状态页映射候选 | 仅映射 `0x500ff000` 后可观察 `M004` | 一次 `CPU_ON` 后得到 `AMP1` / EL1，证明映射后的 Zephyr 路径可运行 | 通过（运行时） |
| MMU 后状态页映射候选运行时 | 状态页映射后继续通过 MMU 和 Zephyr 内核启动 | 一次 CPU_ON 后为 `AMP1` / EL1 | 通过（运行时） |
| 共享页递增心跳候选 | `main()` 后写可观察的递增计数 | 已传输并运行；Linux 读到 `HB|1→HB|2` | 通过（运行时） |
| 共享页递增心跳运行时 | Linux 两次读到不同的 `HB | sequence` | `HB|1`、`HB|2`，均为 EL1 | 通过（运行时） |
| LLM 与 Zephyr 最小共存 | 模型生成期间心跳继续推进 | `llm_demo-amp` 生成 `Alright,`；心跳 `HB|0x323 → HB|0x32b`，均为 EL1 | 通过（运行时） |

## 结论

已完成候选 Linux 的运行时接口回归、Zephyr 映像受控装载及标准 `CPU_ON` 调用。早期 Zephyr 心跳未更新，后续 `magic=0`、`M003` 等检查点也只是定位过程证据；这些原始输出保留在步骤中，最终结论以随后映射状态页和递增心跳候选的成功回归为准。替换为无运行时依赖的探针后，CPU3 在相同入口写回 `AMP1` / `CurrentEL=8`。状态页映射后，标准 Zephyr 路径已从 EL2 切换至 EL1、完成 BSS/data、页表和中断初始化并到达 `main()`。PSCI 启动、CPU 静态隔离、入口地址、共享状态可见性及 Zephyr 应用到达 `main()` 均已验证。仍未验证实时任务、持续运行、次级核关闭/重启、IPC 或 Linux+NPU 与 Zephyr 的长期并行稳定性；不得把当前结果误写成完整 AMP 系统成功。

递增心跳已由 Linux 连续观测为 `HB|1`、`HB|2`，故“CPU3 Zephyr 与 Linux 持续并行运行、共享页单向可见”也已验证。仍未验证双向消息、实时性、生命周期管理、长时稳定性，或 NPU LLM 与 Zephyr 同时负载下的共存。

在保持该心跳实例运行时，学习者首次直接运行 `llm_demo-r1`。心跳从 `HB|0x80` 增至 `HB|0xa8`，均为 EL1，故 Zephyr 在 Linux 侧 RKLLM 初始化尝试期间持续执行。RKLLM 运行时却打印 `Enabled cpus: [0, 1, 2, 3, 4, 5, 6]`、`Enabled cpus num: 4`，随后以 CPU mask/count mismatch 在 `rkllm_init()` 失败；未进入模型生成或 NPU workload。

同一心跳实例随后以 `taskset -c 3-6` 启动相同的板端 `llm_demo-r1`。启动前后的心跳从 `HB|0x10c` 增至 `HB|0x11e`，均为 EL1；RKLLM 输出仍为 `[0, 1, 2, 3, 4, 5, 6]` 与 count `4`，并以相同错误失败。因此 `taskset` 只约束该进程可调度的 CPU，**不能改变 Runtime 对系统 online CPU 拓扑的枚举**。主机源码中的 `llm_demo.cpp` 已明确设置 `enabled_cpus_num = 4` 和 `enabled_cpus_mask = CPU3 | CPU4 | CPU5 | CPU6`，而此板端 `llm_demo-r1` 没有体现这一配置。当前可证实的阻塞是板端运行了旧/未带显式掩码的二进制；不是 RKNPU 0.9.8、PSCI、Zephyr 心跳或 CPU3 让出回退。

主机将该源码传为板端新文件 `llm_demo-amp.cpp`，以 R1 原生 `g++ 11.4.0` 编译为 `llm_demo-amp`，没有覆盖旧 `llm_demo-r1`。新二进制 SHA-256 为 `14dfe7f4f3bd5aa1c62601866452a77a74630db9b37301e89a1bc3eca196a6a0`，源码 SHA-256 为 `baab9f46c73ebfc94c2342108ad6b556cefaa6444acb0f410abb6452a2b82385`，RUNPATH 为 `$ORIGIN/lib`。在同一仍运行的心跳实例中，以短 prompt `ok` 运行新二进制：Runtime 明确枚举 `[3, 4, 5, 6]`、初始化成功并生成 `Alright,`；心跳从 `HB|0x323` 递增至 `HB|0x32b`，且保持 EL1。由此验证当前 RAM 候选中 Linux 侧 RKNPU 实际 LLM 与 CPU3 上 Zephyr 心跳的最小共存。该证据不覆盖长时稳定性、实时抖动、双向通信或外设负载。

## 关联知识与问题

- 支持：标准 PSCI、Linux reserved-memory、sysfs 二进制属性、AArch64 cache 可见性。
- 限制：共享 GIC、次级核异常级别和 Linux+Zephyr 长时间稳定性仍待板端实证。

## 后续行动

- [ ] 结束当前一次性心跳实例后，设计并先静态审计不引入 RPMsg 的最小共享页命令/应答协议，明确缓存一致性与边界；不重复 `start`、不写 eMMC、不调用 `RK_SIP_AMP_CFG`。
