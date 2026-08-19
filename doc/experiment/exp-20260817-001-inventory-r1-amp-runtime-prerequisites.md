---
title: "EXP-20260817-001 盘点 R1 AMP 运行时前置条件"
type: experiment
status: verified
created: 2026-08-17
updated: 2026-08-18
tags: [rk3588, linux, zephyr, amp, remoteproc, mailbox]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[status/current]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
---

# EXP-20260817-001 盘点 R1 AMP 运行时前置条件

## 目标

在不改变 CPU、内存或启动链状态的前提下，确认当前 R1 Linux 是否已有可直接用于 Zephyr AMP 的 CPU、remoteproc/RPMsg 与 mailbox 运行时基础。

## 环境与前置条件

- 执行端：R1 目标 Linux；学习者在正常启动系统中执行。
- 硬件：youyeetoo R1 V2 / RK3588S。
- 操作前状态：原厂 eMMC 启动链保持不变；未离线 CPU、未启动 Zephyr、未写 eMMC。
- 命令来源：本次查询由协作步骤提供；学习者回传完整可见输出，退出码未单独记录。

## 风险与恢复

- 影响范围：仅读取 sysfs、`lscpu` 与 `/proc/config.gz`。
- 备份：不适用；没有修改持久化或运行时状态。
- 恢复方法：不适用。

## 步骤与证据

### 步骤 1：读取 CPU、远程处理器与 IPC 基础

目的与预期结果：确认 Linux 当前管理的 CPU 集合；查找已有 remoteproc/RPMsg sysfs 实例；读取 AMP/IPC 相关 Kconfig 符号。

```bash
# R1 目标 Linux；只读查询
printf '%s\n' '== CPU 集合 =='
cat /sys/devices/system/cpu/possible
cat /sys/devices/system/cpu/present
cat /sys/devices/system/cpu/online

printf '%s\n' '== CPU 拓扑 =='
lscpu -e=CPU,CORE,SOCKET,NODE,MAXMHZ,MINMHZ

printf '%s\n' '== remoteproc / rpmsg / mailbox sysfs =='
ls -la /sys/class/remoteproc /sys/bus/rpmsg 2>&1

printf '%s\n' '== 内核 AMP/IPC 配置 =='
zcat /proc/config.gz |
  grep -E '^(CONFIG_(REMOTEPROC|RPMSG|MAILBOX|HWSPINLOCK|CPU_IDLE|HOTPLUG_CPU|IOMMU_SUPPORT|ROCKCHIP_IOMMU))='
```

实际输出（学习者提供；`ls` 的报错表示相应 sysfs 目录不存在）：

```text
== CPU 集合 ==
0-7
0-7
0-7
== CPU 拓扑 ==
CPU CORE SOCKET NODE    MAXMHZ   MINMHZ
  0    0      0    - 1800.0000 408.0000
  1    1      0    - 1800.0000 408.0000
  2    2      0    - 1800.0000 408.0000
  3    3      0    - 1800.0000 408.0000
  4    0      0    - 2256.0000 408.0000
  5    1      0    - 2256.0000 408.0000
  6    2      1    - 2256.0000 408.0000
  7    3      1    - 2256.0000 408.0000
== remoteproc / rpmsg / mailbox sysfs ==
ls: cannot access '/sys/class/remoteproc': No such file or directory
ls: cannot access '/sys/bus/rpmsg': No such file or directory
== 内核 AMP/IPC 配置 ==
CONFIG_HOTPLUG_CPU=y
CONFIG_CPU_IDLE=y
CONFIG_MAILBOX=y
CONFIG_IOMMU_SUPPORT=y
CONFIG_ROCKCHIP_IOMMU=y
```

观察：

- `possible`、`present`、`online` 都是 `0-7`：8 个 CPU 均存在且当前均由 Linux 在线管理。
- 两组频率上限不同（CPU 0–3 为 1800 MHz，CPU 4–7 为 2256 MHz），表明当前拓扑至少呈现两类性能簇；`lscpu` 的 socket/core 展示不能单独作为资源所有权或隔离结论。
- 未发现 remoteproc 或 RPMsg sysfs 类；本次 Kconfig 输出也未显示 `CONFIG_REMOTEPROC` 或 `CONFIG_RPMSG` 为 `y`。这说明没有发现一个已由当前 Linux 暴露的现成 Zephyr/协处理器装载入口，不能据此断言内核源码完全没有对应选项。
- `CONFIG_MAILBOX=y` 只证明通用 mailbox 框架已启用；是否存在可用于本板 AMP 的 mailbox 控制器、通道和 DTS 节点仍待验证。
- `CONFIG_HOTPLUG_CPU=y` 允许后续研究 CPU offline/online 机制，但这不是 AMP 隔离或 Zephyr 启动证据，当前没有执行该操作。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| Linux 可见 CPU 集合 | 确认是否存在可研究的次级核 | 0–7 均 online | 通过 |
| 已有 remoteproc/RPMsg 路径 | 查找现成装载/通信入口 | 相关 sysfs 目录不存在 | 未发现 |
| mailbox 框架 | 确认是否启用通用基础 | `CONFIG_MAILBOX=y` | 通过（仅框架） |

### 步骤 3：检索 BSP 中的 AMP 参考入口

目的与预期结果：在已固定的 Rockchip `develop-5.10` R1 DTS 移植工作树中，确认是否存在 RK3588 AMP 参考 DTS 和对应 Linux IPC 配置。此步骤只列出匹配路径/行，未修改源码或构建。

实际输出摘录（学习者提供；省略与 RK3588 无关的其他 SoC 文件）：

```text
arch/arm64/configs/rockchip_linux_defconfig
513:CONFIG_MAILBOX=y
519:CONFIG_RPMSG_ROCKCHIP_MBOX=y
520:CONFIG_RPMSG_VIRTIO=y

drivers/rpmsg/Kconfig
61:config RPMSG_ROCKCHIP_MBOX
72:config RPMSG_ROCKCHIP_SOFTIRQ
87:config RPMSG_VIRTIO

drivers/remoteproc/Kconfig
4:config REMOTEPROC
17:config REMOTEPROC_CDEV

arch/arm64/boot/dts/rockchip/rk3588-evb1-lp4-v10-linux-amp.dts
arch/arm64/boot/dts/rockchip/rk3588-amp.dtsi
```

观察：

- 与运行中原厂 R1 镜像不同，待移植的 Rockchip BSP 默认 Linux 配置启用 `CONFIG_RPMSG_ROCKCHIP_MBOX=y` 和 `CONFIG_RPMSG_VIRTIO=y`，源码也提供 remoteproc 的通用 Kconfig 项。
- BSP 提供明确名为 `rk3588-amp` 的 DTS 参考。它是下一轮阅读的优先入口。
- 这些参考文件面向 `rk3588` EVB；R1 为 RK3588S 且使用厂商专有 DTS 组合。仅凭文件名不能证明可直接用于 R1，必须先审计其 CPU、内存和外设所有权。

### 步骤 4：读取 RK3588 AMP 参考的内存与通信资源划分

目的与预期结果：从 `rk3588-amp.dtsi` 提取远端固件、Linux RPMsg 与 DMA 区域，以及 mailbox 连接关系；不将参考文件合入 R1。

实际输出摘录（学习者提供；前置 GIC 路由节点的完整定义尚未读取）：

```text
30-            GIC_AMP_IRQ_CFG_ROUTE(368, 0xd0, CPU_GET_AFFINITY(3, 0))
31-            /* MAILBOX */
32-            GIC_AMP_IRQ_CFG_ROUTE(100, 0xd0, CPU_GET_AFFINITY(3, 0))>;
34:        status = "okay";

37:    reserved-memory {
42-        /* mcu address */
43-        mcu_reserved: mcu@7a00000 {
44:            reg = <0x0 0x7a00000 0x0 0x200000>;
45-            no-map;
48:        rpmsg_reserved: rpmsg@7c00000 {
49:            reg = <0x0 0x07c00000 0x0 0x400000>;
50-            no-map;
53:        rpmsg_dma_reserved: rpmsg-dma@8000000 {
54-            compatible = "shared-dma-pool";
55:            reg = <0x0 0x08000000 0x0 0x200000>;
56-            no-map;
60:    rpmsg: rpmsg@7c00000 {
61:        compatible = "rockchip,rpmsg";
62:        mbox-names = "rpmsg-rx", "rpmsg-tx";
63:        mboxes = <&mailbox0 0 &mailbox0 3>;
64-        rockchip,vdev-nums = <1>;
65-        rockchip,link-id = <0x03>;
66:        reg = <0x0 0x7c00000 0x0 0x20000>;
67:        memory-region = <&rpmsg_dma_reserved>;
69:        status = "okay";
73:&mailbox0 {
74-    rockchip,txpoll-period-ms = <1>;
75:    status = "okay";
```

资源图（由 `reg` 数值直接计算）：

```text
0x07a0_0000 ─ 0x07c0_0000  mcu_reserved       2 MiB
0x07c0_0000 ─ 0x0800_0000  rpmsg_reserved     4 MiB
0x0800_0000 ─ 0x0820_0000  rpmsg_dma_reserved 2 MiB
```

观察：

- 三段内存连续，共 8 MiB，均带 `no-map`，因此 Linux 不应把它们作为普通线性映射内存使用。
- `mcu_reserved` 的注释为 `mcu address`，是参考中留给远端 MCU/固件的 2 MiB 区域；远端固件的实际格式、装载者和入口地址仍待读顶层 DTS/启动代码确认。
- `rpmsg` 节点以 `rockchip,rpmsg` 声明控制区 `0x07c00000`、大小 `0x20000`（128 KiB），并引用 mailbox0 的通道 0、3，按 `mbox-names` 顺序分别命名为 `rpmsg-rx`、`rpmsg-tx`。
- `rpmsg_dma_reserved` 是 `shared-dma-pool`，被 `memory-region` 引用。该节点为何需 2 MiB、谁可访问以及缓存一致性策略要读驱动后才能结论。
- GIC 路由宏出现 `CPU_GET_AFFINITY(3, 0)` 和 mailbox IRQ 100；在未读取宏定义和所属节点前，不能据此断言它等于某个 Linux 逻辑 CPU 或该核已运行 Zephyr。

### 步骤 5：读取 AMP 顶层 DTS 的 CPU 与内存所有权改动

目的与预期结果：确认 AMP 参考是否仅声明通信资源，还是在顶层 DTS 明确从 Linux 移除 CPU/内存资源。

实际输出摘录（学习者提供）：

```dts
#include "rk3588-amp.dtsi"

cpus {
    cpu-map {
        cluster0 {
            /delete-node/ core3;
        };
    };
};

reserved-memory {
    amp_reserved: amp@800000 {
        reg = <0x0 0x00800000 0x0 0x01800000>;
        no-map;
    };
};

&arm_pmu {
    interrupt-affinity = <&cpu_l0>, <&cpu_l1>, <&cpu_l2>,
                        <&cpu_b0>, <&cpu_b1>, <&cpu_b2>, <&cpu_b3>;
};

/delete-node/ &cpu_l3;
```

同一公共 DTS 开头还定义：

```dts
rockchip_amp: rockchip-amp {
    compatible = "rockchip,amp";
    clocks = <&cru HCLK_PMU_CM0_ROOT>, <&cru FCLK_PMU_CM0_CORE>, ...;
    pinctrl-0 = <&uart5m0_xfer>;
    amp-irqs = /bits/ 64 < ... UART5 ... MAILBOX ... >;
    status = "okay";
};
```

资源图（由 `amp_reserved.reg` 直接计算）：

```text
0x0080_0000 ─ 0x0200_0000  amp_reserved  24 MiB  no-map
```

观察：

- 该 EVB AMP 参考明确删除 `cluster0/core3` 的 CPU map 项和 `&cpu_l3` 节点，并同步从 ARM PMU 的中断亲和性列表去除 `cpu_l3`。这比 Linux CPU offline 更强：Linux 从启动时就不应发现该 CPU 节点。
- 顶层 DTS 额外预留 24 MiB `amp_reserved`，与公共 DTS 中 `mcu_reserved`/RPMsg 保留区是不同地址段；是否分别服务不同固件阶段或不同远端实体尚待驱动/文档证实。
- `rockchip,amp` 节点配置的时钟名指向 PMU CM0 域，并占用 UART5、GPIO 外部中断、mailbox 中断路由。这表明参考方案包含 SoC 特定的 AMP 管理硬件，不能简单视作通用 AArch64 Zephyr 启动模板。
- `CPU_GET_AFFINITY(3, 0)` 宏虽定义为 `((cpu) << 8)`，但 `GIC_AMP_IRQ_CFG_ROUTE` 如何编码 GIC 目标、它和被删除的 `cpu_l3`/PMU CM0 的精确关系仍待读取绑定头与 `rockchip,amp` 驱动。

### 步骤 6：定位 `rockchip,amp` 的内核驱动与 IRQ 解析

目的与预期结果：确定 DTS 中的 `rockchip,amp` 节点是否由 Linux 驱动匹配，并区分“配置中断路由”与“加载/启动远端固件”两类职责。

实际输出摘录（学习者提供）：

```c
/* include/dt-bindings/soc/rockchip-amp.h */
#define GIC_AMP_IRQ_CFG_ROUTE(_irq, _prio, _aff) (_irq) (_prio) (_aff)

/* drivers/soc/rockchip/rockchip_amp.c */
prop = of_find_property(np, "amp-irqs", NULL);
count = of_property_count_u64_elems(np, "amp-irqs");
if (count % 3)
    return;

for (i = 0; i < count / 3; i++) {
    of_property_read_u64_index(np, "amp-irqs", 3 * i, &val);
    irq = (u32)val;
    of_property_read_u64_index(np, "amp-irqs", 3 * i + 1, &prio);
    of_property_read_u64_index(np, "amp-irqs", 3 * i + 2, &aff);
    amp_ctrl->irqs_cfg[irq].prio = (u32)prio;
    amp_ctrl->irqs_cfg[irq].aff = aff;
}

static const struct of_device_id rockchip_amp_match[] = {
    { .compatible = "rockchip,amp" },
    { .compatible = "rockchip,mcu-amp" },
    { .compatible = "rockchip,rk3568-amp" },
    { /* sentinel */ },
};
```

观察：

- `GIC_AMP_IRQ_CFG_ROUTE` 没有自行编码位字段，只展开为三个 64 位单元；`amp-irqs` 的语义由驱动按每 3 个值解释为 IRQ 编号、优先级和目标 affinity。
- `rockchip_amp` 的设备树 match table 确认 `compatible = "rockchip,amp"` 会绑定到 `drivers/soc/rockchip/rockchip_amp.c`。
- 当前已读函数只把 DTS 三元组写入驱动的 IRQ 配置数组。尚未看到固件映像、PSCI `CPU_ON`、remoteproc 或 Zephyr 入口处理，因此不能声称该驱动负责装载或启动远端固件。

### 步骤 7：读取 `rockchip_amp` 的 probe 路径

目的与预期结果：确认驱动是否有远端 CPU 生命周期控制，而不只配置 GIC。

实际输出摘录（学习者提供）：

```c
static int rockchip_amp_probe(struct platform_device *pdev)
{
    ...
    rkamp_dev->num_clks = devm_clk_bulk_get_all(&pdev->dev, &rkamp_dev->clks);
    ret = clk_bulk_prepare_enable(rkamp_dev->num_clks, rkamp_dev->clks);
    ...
    cpus_node = of_get_child_by_name(pdev->dev.of_node, "amp-cpus");
    if (cpus_node) {
        for_each_available_child_of_node(cpus_node, cpu_node) {
            if (!rockchip_amp_boot_cpus(&pdev->dev, cpu_node, idx))
                idx++;
        }
    }
    ...
    rk_amp_kobj = kobject_create_and_add("rk_amp", NULL);
    ...
    sysfs_create_file(rk_amp_kobj, &rk_amp_attrs[i].attr);
}

static struct platform_driver rockchip_amp_driver = {
    .probe = rockchip_amp_probe,
    .remove = rockchip_amp_remove,
    .driver = {
        .name = "rockchip-amp",
        .of_match_table = rockchip_amp_match,
    },
};
```

同一文件的符号定位显示：

```text
boot_cpu_store(): echo on/off/status [cpu id] > /sys/rk_amp/boot_cpu
rockchip_amp_boot_cpus()
```

观察：

- `probe` 会启用 `rockchip,amp` 节点列出的时钟和可选电源域，再寻找其 `amp-cpus` 子节点。
- 对每个可用的 `amp-cpus` 子节点，驱动调用 `rockchip_amp_boot_cpus()`；这证明驱动参与远端 CPU 的启动配置，而不仅是 GIC IRQ 路由。
- probe 完成后建立全局 `/sys/rk_amp/boot_cpu` 接口，代码提示其支持 `on`、`off`、`status` 加 CPU ID 的控制语义。该接口当前不在原厂 R1 运行时 DTS/内核中，不能在板端假定存在。
- 仍待确认：`amp-cpus` 节点实际指向哪类处理器、`entry`/`mode` 的 DTS 语义，以及 `rockchip_amp_boot_cpus()` 最终调用的 SoC/固件机制。此前“尚未看到启动责任”的结论已被本步骤修正。

### 步骤 8：读取远端 CPU 启动调用

目的与预期结果：确认 `rockchip_amp_boot_cpus()` 是否仅保存状态，还是实际请求固件启动远端 CPU。

实际输出（学习者提供）：

```c
if (of_property_read_u64_array(cpu_node, "id", &cpu_id, 1))
    return -1;
if (of_property_read_u64_array(cpu_node, "entry", &cpu_entry, 1))
    return -1;
if (!cpu_entry)
    return -1;
if (of_property_read_u32_array(cpu_node, "mode", &cpu_mode, 1))
    return -1;
if (of_property_read_u32_array(cpu_node, "boot-on", &boot_on, 1))
    boot_on = 1;

ret = sip_smc_amp_config(RK_AMP_SUB_FUNC_CFG_MODE, cpu_id, cpu_mode, 0);
...
if (boot_on)
    ret = sip_smc_amp_config(RK_AMP_SUB_FUNC_CPU_ON,
                             cpu_id, cpu_entry, 0);
```

观察：

- 每个 `amp-cpus` 子节点必须提供硬件 `id`、非零 `entry` 和 `mode`；省略 `boot-on` 时默认自动启动。
- 驱动先经 `sip_smc_amp_config(...CFG_MODE...)` 设置远端 CPU mode，再经 `sip_smc_amp_config(...CPU_ON...)` 将该 CPU 拉起到指定 `entry`。
- `sip_smc` 表明 Linux 通过 Rockchip 的 SiP（Silicon Provider）SMC 接口请求安全固件执行特权操作；它不是普通 Linux 进程跳转，也不是本次已观察到的标准 PSCI `CPU_ON` 调用。
- 这为**通用 `rockchip_amp` 驱动能力**中的远端 CPU 启动责任提供了直接代码证据。`cpu_id` 的编码、`cpu_mode` 可选值、`entry` 地址来源及 R1 的 TF-A/OP-TEE 是否实现同一 SiP 服务仍待验证；当前禁止对 R1 调用该接口。

### 步骤 9：检查 RK3588 AMP DTS 是否启用 `amp-cpus`

目的与预期结果：验证 RK3588 AMP 参考是否实际提供 `id`、`entry`、`mode`，从而启用步骤 8 的 SiP SMC CPU 启动路径。

```fish
# Arch 主机 fish；只读检索
rg -l 'amp-cpus' $kernel_src/arch/arm64/boot/dts/rockchip/rk3588*.dts*
```

实际输出（学习者提供）：无标准输出，fish 提示符显示退出码 `1`。

观察：

- `rg` 的退出码 `1` 表示没有匹配，而不是命令执行错误。
- 在该 RK3588 DTS 文件集合中未找到 `amp-cpus`，故这份 RK3588 AMP 参考不会在 `rockchip_amp_probe()` 中进入 `rockchip_amp_boot_cpus()` 循环，也不会据此填充 `/sys/rk_amp/boot_cpu` 所需的 CPU 信息。
- 因而必须修正此前的阶段性推断：`rockchip_amp` 驱动**具备**经 Rockchip SiP SMC 拉起 DTS 声明 CPU 的通用能力，但当前已读的 RK3588 AMP DTS **未证明使用了该能力**。
- 该参考仍显式启用 PMU CM0 相关时钟、UART5、GIC 路由、RPMsg/mailbox 和保留内存。**推测**：RK3588 参考更可能依赖已由更早固件启动或管理的 PMU CM0/MCU 实体；须以其固件加载链或资料验证，不能据此断言 Zephyr 可直接运行在被删除的 A55 `cpu_l3` 上。

### 步骤 10：定位 Linux 到安全固件的 AMP API

目的与预期结果：确认 `sip_smc_amp_config()` 的实现位置及其可用子命令，从而界定 Linux 与安全固件之间的 AMP 协议。

实际输出摘录（学习者提供）：

```text
drivers/firmware/rockchip_sip.c
267:int sip_smc_amp_config(u32 sub_func_id, u32 arg1, u32 arg2, u32 arg3)
275:EXPORT_SYMBOL_GPL(sip_smc_amp_config);

include/linux/rockchip/rockchip_sip.h
RK_AMP_SUB_FUNC_CFG_MODE = 0,
RK_AMP_SUB_FUNC_BOOT_ARG01,
RK_AMP_SUB_FUNC_BOOT_ARG23,
RK_AMP_SUB_FUNC_REQ_CPU_OFF,
RK_AMP_SUB_FUNC_GET_CPU_STATUS,
RK_AMP_SUB_FUNC_RSV, /* for RTOS */
RK_AMP_SUB_FUNC_CPU_ON,
RK_AMP_SUB_FUNC_SET_CPU_OFF_FIQ,
RK_AMP_SUB_FUNC_REQ_CPU_OFF_FIQ,
```

观察：

- `rockchip_amp` 调用的 API 由 `drivers/firmware/rockchip_sip.c` 导出，声明位于 Rockchip SiP 头文件，表明这是 Linux 内核与更高特权安全固件之间的厂商接口。
- 已知子命令涵盖 CPU mode、两组 boot 参数、CPU on/off、状态和 FIQ 控制；枚举中的 `RSV, /* for RTOS */` 是资料中的直接注释，说明该接口设计考虑了 RTOS 场景。
- 尚未读取函数内传给 `arm_smccc_smc()` 的具体 service ID，且未验证 R1 当前 TF-A/OP-TEE 是否实现该 service；因此不能在 R1 上试调用。

### 步骤 11：读取 AMP SMC 参数边界

目的与预期结果：确认 Linux 将哪些值作为 SMC 参数提交给安全固件，区分 Linux 侧控制请求与 EL3 实际执行。

实际输出（学习者提供）：

```c
int sip_smc_amp_config(u32 sub_func_id, u32 arg1, u32 arg2, u32 arg3)
{
    struct arm_smccc_res res;

    arm_smccc_smc(RK_SIP_AMP_CFG, sub_func_id, arg1, arg2, arg3,
                  0, 0, 0, &res);
    return res.a0;
}
```

观察：

- 依照 `arm_smccc_smc()` 调用顺序，Linux 传入的前五个寄存器参数依次是 service ID `RK_SIP_AMP_CFG`、AMP 子命令、`arg1`、`arg2`、`arg3`；其余参数为 0。
- `res.a0` 被直接作为函数返回值，故启动或配置的实际成功/失败由安全固件响应决定。
- Linux 驱动仅发出 Rockchip SiP 请求；实际 CPU mode 配置、CPU on/off 与入口跳转执行在该 SMC 的固件处理端。该处理端尚未定位，当前不能推断 R1 固件具备该服务。

### 步骤 12：固定 AMP SMC service ID

目的与预期结果：得到可在安全固件源码或二进制分析中精确检索的 AMP 请求标识。

实际输出（学习者提供）：

```c
#define RK_SIP_AMP_CFG  0x82000022
```

观察：

- Linux 发出的 AMP 请求 service ID 为 `0x82000022`。
- 后续定位固件处理端应优先搜索该数值、`RK_SIP_AMP_CFG` 或相应 AMP SiP dispatch，而不能仅凭“AMP”文本命中推断。

### 步骤 13：在本地源码中查找 AMP SMC 固件处理端

目的与预期结果：判断现有本地源码是否包含 EL3/安全固件对 `0x82000022` 的实现。

```fish
# Arch 主机 fish；只读检索
rg -l '0x82000022|RK_SIP_AMP_CFG' /home/loser/Study/rk3588/src | head -n 30
```

实际输出（学习者提供）：

```text
rockchip-linux-kernel-r1-dts-port/include/linux/rockchip/rockchip_sip.h
rockchip-linux-kernel-r1-dts-port/drivers/firmware/rockchip_sip.c
youyeetoo-r1-linux-kernel-5-10/include/linux/rockchip/rockchip_sip.h
youyeetoo-r1-linux-kernel-5-10/drivers/firmware/rockchip_sip.c
```

观察：

- 本地未找到 TF-A、BL31 或其他安全固件的处理端；现有命中均为 Linux 头文件和 Linux SMC 调用封装。
- 风火轮 R1 厂商 5.10 源码也保留同一 service ID 的声明和调用封装。这表明该厂商内核源码与 Rockchip AMP ABI 相容，但不能证明当前 R1 的预编译 BL31 实现 `0x82000022`，也不能证明 AMP 驱动/DTS 已启用。

### 步骤 14：检查 R1 厂商源码的 AMP Kconfig 入口

目的与预期结果：确认 R1 厂商内核源码是否包含 Rockchip AMP 驱动选项，并检查其默认配置是否显式选择该选项。

实际输出（学习者提供）：

```text
drivers/soc/rockchip/Kconfig
config ROCKCHIP_AMP
    tristate "Rockchip AMP support"
```

观察：

- R1 厂商内核源码包含可选的 `CONFIG_ROCKCHIP_AMP` 驱动入口。
- 同次查询未从 `arch/arm64/configs/rockchip_linux_defconfig` 输出 `CONFIG_ROCKCHIP_AMP=y` 或 `=m`。这只表明默认配置文件中没有显式选择它；运行中内核的实际 `.config` 尚待直接检查。

### 步骤 15：确认 R1 运行中内核的 AMP 配置

目的与预期结果：以目标板实际 `/proc/config.gz` 排除“源码有选项但运行镜像另行启用”的可能。

```bash
# R1 目标 Linux Bash；只读查询
zcat /proc/config.gz | grep -E '^CONFIG_ROCKCHIP_AMP=|^# CONFIG_ROCKCHIP_AMP is not set$'
```

实际输出（学习者提供）：

```text
# CONFIG_ROCKCHIP_AMP is not set
```

观察：

- 原厂 R1 运行内核明确未启用 Rockchip AMP 驱动；这解释了此前未发现 `/sys/rk_amp`、remoteproc/RPMsg 运行时入口和相关设备树节点的现象。
- 当前原厂 eMMC 系统不能直接启动 Rockchip AMP 参考路径。后续若采用该路径，至少需要自编内核启用驱动、移植/审计 R1 DTS 资源划分，并验证当前或可替换的 BL31 是否实现 SiP service `0x82000022`。

### 步骤 16：确认 RKNPU 候选内核的 AMP/IPC 配置

目的与预期结果：检查已构建并已完成 RAM 启动/NPU 推理验证的 Rockchip `develop-5.10` 候选内核，是否可作为 AMP 原型的配置基线。

实际输出（学习者提供；来自 `build/kernel-r1-dts-port/.config`）：

```text
CONFIG_MAILBOX=y
CONFIG_RPMSG=y
CONFIG_RPMSG_ROCKCHIP_MBOX=y
CONFIG_RPMSG_VIRTIO=y
CONFIG_ROCKCHIP_AMP=y
```

观察：

- 已验证的 RKNPU 0.9.8 候选内核配置已启用 Rockchip AMP 驱动、通用 mailbox、Rockchip mailbox RPMsg 与 VirtIO RPMsg。
- 因此该候选可作为 AMP 原型的**内核配置基线**；R1 DTS 尚未接入 AMP 节点，BL31 service 尚未验证，不能据此直接启动 AMP。
- 在移植任何 `reserved-memory` 前，必须先审计 R1 当前内核、OP-TEE 和现有保留区的物理地址，避免参考 AMP 区域与当前载荷重叠。

### 步骤 2：读取 CPU 启动方法、保留内存与 DTS 通信节点

目的与预期结果：确定 CPU 次级核由谁启动；检查现有设备树是否已为远端固件预留专用内存，或描述 mailbox/remoteproc/RPMsg 等节点。

```bash
# R1 目标 Linux；只读查询
printf '%s\n' '== CPU 节点与启动方式 =='
for node in /proc/device-tree/cpus/cpu@*; do
    printf '%s: ' "$node"
    if test -r "$node/enable-method"; then
        tr -d '\0' < "$node/enable-method"
    else
        printf '%s' '(no enable-method)'
    fi
    printf '\n'
done

printf '%s\n' '== reserved-memory 子节点 =='
find /proc/device-tree/reserved-memory -mindepth 1 -maxdepth 1 -type d \
    -printf '%f\n' 2>&1

printf '%s\n' '== AMP / IPC 相关 DTS 节点 =='
find /proc/device-tree -type d \
    \( -iname '*mailbox*' -o -iname '*mbox*' -o -iname '*remoteproc*' \
       -o -iname '*rproc*' -o -iname '*rpmsg*' -o -iname '*shared-memory*' \) \
    -print
```

实际输出（学习者提供；末段没有匹配项）：

```text
== CPU 节点与启动方式 ==
/proc/device-tree/cpus/cpu@0: psci
/proc/device-tree/cpus/cpu@100: psci
/proc/device-tree/cpus/cpu@200: psci
/proc/device-tree/cpus/cpu@300: psci
/proc/device-tree/cpus/cpu@400: psci
/proc/device-tree/cpus/cpu@500: psci
/proc/device-tree/cpus/cpu@600: psci
/proc/device-tree/cpus/cpu@700: psci
== reserved-memory 子节点 ==
cma
drm-logo@00000000
ramoops@110000
drm-cubic-lut@00000000
== AMP / IPC 相关 DTS 节点 ==
```

观察：

- 八个 CPU 节点均使用 `psci`。**已验证的含义**是 Linux 通过 PSCI 这一固件接口管理次级核的开关；这不是 Zephyr 已有启动入口，也不是 Linux 可在运行中安全地把任意在线核改为 Zephyr 的证据。
- `cpu@0` 至 `cpu@700` 是 CPU 节点的硬件地址单元命名，不能在未读取 `reg` 属性前直接把它们等同为 Linux 的逻辑 CPU 编号。
- `/reserved-memory` 中的 `cma` 是 Linux 可按需分配的连续内存池，`drm-logo` 与 `drm-cubic-lut` 服务显示，`ramoops` 保存崩溃/控制台持久日志；本次未发现可明确归属 Zephyr 的静态 carveout。
- 未发现名称含 mailbox、remoteproc、RPMsg 或 shared-memory 的 DTS 节点。名称搜索不能取代按 `compatible` 的完整审计，但与步骤 1 的 sysfs/Kconfig 结果一致：当前镜像没有暴露可直接复用的 AMP 控制平面。

### 步骤 17：审计原厂 R1 的物理内存布局

目的与预期结果：检查运行中内核代码、数据和现有保留区的物理范围，排除直接复用 Rockchip EVB AMP 参考 `amp_reserved` 地址的可能性。

```bash
# R1 目标 Linux Bash；只读查询
grep -E 'System RAM|Kernel code|Kernel data|reserved|CMA|ramoops|optee|OP-TEE' /proc/iomem
```

实际输出（学习者提供）：

```text
00110000-0012ffff : ramoops:dmesg
00130000-001affff : ramoops:console
001b0000-001fffff : ramoops:pmsg
00200000-083fffff : System RAM
  00400000-01c6ffff : Kernel code
  01c70000-022dffff : reserved
  022e0000-0269ffff : Kernel data
  08300000-08324fff : reserved
09400000-efffffff : System RAM
  e9f00000-edfeffff : reserved
1f0000000-1ffffffff : System RAM
  1fa7c0000-1fedfffff : reserved
  1fee2f000-1fee4efff : reserved
  1fee4f000-1fee4ffff : reserved
  1fee50000-1fef3ffff : reserved
  1fef42000-1fef42fff : reserved
  1fef43000-1fef43fff : reserved
  1fef44000-1fef54fff : reserved
  1fef55000-1fef55fff : reserved
  1fef56000-1ff00afff : reserved
  1ff00b000-1ffffffff : reserved
```

观察：

- Rockchip EVB AMP 参考的 `amp_reserved` 是 `0x00800000`–`0x02000000`；该区间与当前 R1 的 Kernel code `0x00400000`–`0x01c6ffff`、中间 reserved `0x01c70000`–`0x022dffff` 以及 Kernel data `0x022e0000`–`0x0269ffff` 重叠。
- 因此**已排除**把该 24 MiB 示例地址直接加入 R1 DTS 的做法。这不是配置差异，而是会从 Linux 正在执行的映像中夺取物理内存。
- `0x07a00000`–`0x08200000` 的参考 MCU/RPMsg/DMA 8 MiB 区位于当前第一段 System RAM 内；本次输出没有将其列为现有保留区，但它是否可安全 carveout 仍取决于候选内核实际 load/DT reserved-memory、CMA 与固件使用情况，尚未验证。

### 步骤 18：审计 RKNPU 0.9.8 候选内核的实际物理内存布局

目的与预期结果：在此前已通过 RAM 启动和 NPU 推理的无显示/无 Mali 候选中，读取最终运行时内存图，避免仅以原厂内核布局推断 AMP carveout。

操作：学习者在 U-Boot 从 userdata 临时加载 `r1-boot-fit-npu098-nodisplay-nogpu.img` 并以 `bootm ...#conf` 启动。此过程只读取 userdata，不写 eMMC；进入候选 Linux 后执行与步骤 17 相同的只读查询。

实际输出（学习者提供）：

```text
00110000-0012ffff : ramoops:dmesg
00130000-001affff : ramoops:console
001b0000-001fffff : ramoops:pmsg
00200000-083fffff : System RAM
  00400000-01bdffff : Kernel code
  01be0000-0220ffff : reserved
  02210000-025cffff : Kernel data
  08300000-08324fff : reserved
09400000-efffffff : System RAM
  e9f00000-edfeffff : reserved
1f0000000-1ffffffff : System RAM
  1fa7c0000-1fedfffff : reserved
  1fee2d000-1fee4cfff : reserved
  1fee4d000-1fee4dfff : reserved
  1fee4e000-1fef3dfff : reserved
  1fef40000-1fef41fff : reserved
  1fef42000-1fef52fff : reserved
  1fef53000-1fef53fff : reserved
  1fef54000-1ff008fff : reserved
  1ff009000-1ffffffff : reserved
```

观察：

- 候选 Kernel code 为 `0x00400000`–`0x01bdffff`，reserved 为 `0x01be0000`–`0x0220ffff`，Kernel data 为 `0x02210000`–`0x025cffff`。因此 EVB 的 `0x00800000`–`0x02000000` 24 MiB `amp_reserved` 仍与候选的代码和 reserved 范围重叠，继续排除直接复用。
- 参考公共 DTS 的连续 8 MiB 区 `0x07a00000`–`0x08200000` 完整落在候选第一段 System RAM `0x00200000`–`0x083fffff` 内，本次输出未标为 kernel/reserved；且终点低于现有 `0x08300000`–`0x08324fff` 保留区。这使其成为**可进一步审计的 carveout 候选**，而不是已获准占用的区域。
- 下一步须先验证 R1 DTS 中 mailbox 控制器、通道及 PMU CM0/UART5 等参考资源是否存在或冲突；在此之前不添加 `reserved-memory`，不启动 AMP。

## 结论

当前原厂 R1 系统不能直接复用一个已暴露的 remoteproc/RPMsg 路径来启动 Zephyr；Linux 正在调度全部 8 个 CPU，次级核开关经 PSCI 固件接口，且 DTS 未提供可明确归属 Zephyr 的专用内存或通信节点。待移植的 Rockchip 5.10 BSP 包含 RK3588 AMP DTS 参考和 Rockchip mailbox RPMsg 配置：它启动时从 Linux 移除 `cpu_l3`，预留 24 MiB AMP 区，以及另一个连续 8 MiB MCU/RPMsg/DMA 区；`rockchip_amp` 已确认接管指定 GIC IRQ 的优先级/affinity、启用 AMP 时钟/电源域。该通用驱动具备由 `amp-cpus` 触发 SiP SMC CPU 启动的能力，所用 Rockchip SiP AMP 协议还定义 RTOS 相关保留子命令；Linux 以 `RK_SIP_AMP_CFG` service ID `0x82000022` 发起请求，并由安全固件返回结果。R1 厂商内核源码也保留相同 Linux ABI 和可选 `CONFIG_ROCKCHIP_AMP`，但运行中原厂 R1 内核已确认将其设为未启用；相反，已完成 NPU RAM 验证的 Rockchip 5.10.252 候选配置已启用 AMP、mailbox 和 RPMsg。当前无 EL3 处理端源码，R1 DTS 也未接入 AMP。候选运行时内存图同样证明 EVB 示例的 24 MiB 地址与 R1 候选内核映像重叠，不能直接移植；但其 8 MiB MCU/RPMsg/DMA 参考区没有与候选当前 kernel/reserved 区重叠，成为待进一步审计的 carveout 候选。下一步先核对 mailbox 与 PMU CM0 资源，再决定是否形成 DTS 草案；不能把 CPU offline 当作完成方案。

## 关联知识与问题

- 支持决策：[DEC-20260810-002](../decision/dec-20260810-002-linux-zephyr-amp-long-term-direction.md)。
- 已知 NPU 候选启动边界：[EXP-20260815-002](exp-20260815-002-probe-r1-npu-runtime-chain.md)。

## 后续行动

- [ ] 在 R1 候选 DTS 中审计 mailbox 控制器、通道及 PMU CM0/UART5 参考资源；确认无冲突后再为 `0x07a00000`–`0x08200000` 形成只供静态检查的 carveout DTS 草案，不写 eMMC、不启动 AMP。
