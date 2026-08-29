---
title: "EXP-20260822-001 构建 R1 Linux-Zephyr 共享内存 PING 原型"
type: experiment
status: verified
created: 2026-08-22
updated: 2026-08-29
tags: [rk3588, amp, zephyr, shared-memory, cache, psci]
related:
  - "[[experiment/exp-20260821-003-build-r1-psci-cpu-on-heartbeat]]"
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[status/current]]"
---

# EXP-20260822-001 构建 R1 Linux-Zephyr 共享内存 PING 原型

## 目标

在既有 Linux+Zephyr 单向共享页心跳基础上，构建一个不引入 RPMsg 的最小双向共享内存 PING/response 原型，并将 Linux 驱动、AMP DTB、Zephyr 固件和 RAM-only FIT 组合起来；记录其 RAM candidate 运行时回归及一次 RKLLM 同负载共存观察。

## 环境与前置条件

- 执行端：Arch Linux 主机构建；运行端：当前 RAM-only FIT candidate，未写启动分区或保存 U-Boot 环境。
- Linux：Rockchip 5.10.252、RKNPU 0.9.8 R1 AMP 候选；Image 为 37,675,520 B。
- Zephyr：`src/zephyr-amp-shmem-ping/`，入口固定为 `0x5000100c`，共享状态页基址为 `0x500ff000`。
- 前置运行时边界：`EXP-20260821-003` 已验证 Linux 与 Zephyr 的单向 `HB` 心跳，但尚未验证双向消息。

## 风险与恢复

- 影响范围：仅生成主机 `build/local/r1-amp-shmem-ping/` 中的可再生内核、DTB、resource 和 FIT 候选。
- 板端影响：仅使用 RAM-only FIT candidate 启动并进行运行时回归；未写 eMMC、U-Boot 环境或启动分区。
- 恢复方法：不使用该候选即可继续原 eMMC 启动；主机候选可从构建目录重新生成。

## 步骤与证据

### 步骤 1：实现固定 cache-line 的 request/response 布局

Linux 侧新增 `ping`/`response` sysfs 接口，Zephyr 侧使用 `src/zephyr-amp-shmem-ping/src/main.c` 的单槽轮询 responder。共享区域从 `0x500ff000` 起按独立 64 B cache line 划分：状态/检查点页之后依次为 request payload、request commit、response payload、response commit。payload 含 command、value、status、result；commit 含 `sequence` 与按位取反值，用于检测完整提交。

Zephyr 对 request commit/payload 执行显式 cache invalidate，对 response payload/commit 执行显式 cache flush，并在各阶段使用 `dsb sy`。`PING` 命令返回成功状态和 `value + 1`；未知命令返回错误状态。源码的静态断言要求 payload 与 commit 均恰占一个 64 B cache line。

这是主机源码审计与构建输入记录，不是 Linux↔Zephyr 板端通信证据。

### 步骤 2：构建 Linux、AMP DTB 与 Zephyr

主会话提供的主机构建结果如下：

| 部件 | 实际结果 | 判定 |
| --- | --- | --- |
| Zephyr bin | 36,896 B（`0x9020`），entry `0x5000100c`，SHA-256 `86c483581eb15d67c4af597363163de72e6aadc93df36e433a3379fdd75a8801` | 通过（静态） |
| AMP DTB | 233,743 B，SHA-256 `38146bf6c121dcfe0a86dcb00294e195c709d70f479b0976fde08c9dc66a3269` | 通过（静态） |
| Linux Image | 37,675,520 B，SHA-256 `f050ddef09ef83f730ff5dcec50d2b5163cadbe3de77331399fd01c78c3211d2` | 通过（静态） |
| DTS image-size | 已同步为 `0x9020` | 通过（静态） |

编译通过只证明输入和输出可生成；没有证明 Linux 启动、Zephyr 执行或 PING 回应。

### 步骤 3：重建并回读 RAM-only resource/FIT

主机会将新 AMP DTB 放入 resource 的 `rk-kernel.dtb`，logo 继承原条目；resource 与 FIT 回读后以 `cmp` 核对通过。主会话提供的封装结果：

| 部件 | 实际结果 | 判定 |
| --- | --- | --- |
| resource | SHA-256 `96d5e4c769bf4cd9e72d6c8aac9e9d20428d0c596f20395059dd738e924803c3` | 通过（主机回读） |
| FIT | `build/local/r1-amp-shmem-ping/r1-boot-fit-amp-shmem-ping.img`，38,636,544 B，SHA-256 `382f8a927deae881de8bc9a5553917513ecfd095a3c642a25a6f6015fe18a0af` | 通过（主机回读） |
| resource/FIT 内容 | `cmp` 回读通过；logo 与原 resource 一致；总大小小于 64 MiB | 通过（静态） |

FIT 仍是 RAM-only 候选；本步骤只记录主机回读，后续步骤已单独记录该候选的启动、DTB 交接和 PING 运行时证据，未验证持久化启动路径。

### 步骤 4：RAM-only FIT 启动与顺序 PING 回归

目的与预期结果：在不写启动分区、不保存 U-Boot 环境的前提下，验证 resource-DTB 候选能进入 Linux，完整装载 Zephyr，并由 Linux 发出顺序 PING 后读回一致的 response。

主会话确认的新 RAM-only FIT 已成功启动。板端运行时观察到：`nproc=7`，`zephyr` reserved 节点存在，驱动的 `image`/`ping`/`response`/`start`/`status` 接口均存在；Zephyr 映像完整装载为 `36896/36896`。一次 `start` 后串口出现 CPU3 normal-world 交接，随后状态为：

```text
affinity=off (1) cpu_on_attempted=1 cpu_on_ret=0 magic=0x48420001 current_el=4
```

Linux 写入 `41` 至 `ping`，等待 1 秒后读取：

```text
request_seq=1 response_seq=1 valid=1 command=1 value=41 status=0 result=42
```

随后状态页心跳为 `magic=0x48420066,current_el=4`。这些输出证明当前 RAM candidate 上标准 PSCI 启动的 Zephyr 已运行在 EL1，并完成一次 Linux→Zephyr→Linux 单槽 PING/response；request/response 的显式 cache maintenance 对该次交接可见。此处尚未覆盖异常恢复、并发/吞吐或长期稳定性，后续顺序请求见下文。

在同一运行时实例中，Linux 随后顺序写入第二个请求 `100`，读取为：

```text
request_seq=2 response_seq=2 valid=1 command=1 value=100 status=0 result=101
```

这补充验证了同一实例内连续请求的序列推进和响应结果；它仍不是并发、吞吐、异常恢复或长期稳定性证据。

### 步骤 5：RKLLM 同负载后的 PING 回环

在同一 7 CPU RAM candidate 运行时实例中，运行 `llm_demo-amp`；其内部启用 CPU 为 `[3,4,5,6]`、count 为 `4`，输出 `rkllm init success`，对输入 `ok` 生成 `Alright,`。该过程无需重启 Zephyr。随后 Linux 写入 `7` 至 `ping`，读回：

```text
request_seq=3 response_seq=3 valid=1 command=1 value=7 status=0 result=8
```

此后状态为 `magic=0x4842052d current_el=4`。这验证了同一实例中一次 RKLLM 生成与后续一次 PING 回环共存；不代表压力、并发、长期稳定性或性能验证。

同一运行时实例随后又完成五次顺序请求：

```text
request_seq=4 response_seq=4 valid=1 command=1 value=200 status=0 result=201
request_seq=5 response_seq=5 valid=1 command=1 value=201 status=0 result=202
request_seq=6 response_seq=6 valid=1 command=1 value=202 status=0 result=203
request_seq=7 response_seq=7 valid=1 command=1 value=203 status=0 result=204
request_seq=8 response_seq=8 valid=1 command=1 value=204 status=0 result=205
```

因此当前运行时证据累计为序列 `1`–`8` 的顺序 request/response；仍不扩大为并发、压力、吞吐、异常恢复或长期稳定性结论。

### 步骤 6：mailbox0 controller 的 RAM-only probe

目的与预期结果：在不接入 mailbox client/channel、不启动新的 Zephyr IPC 协议且不写启动分区的前提下，仅启用 AMP DTS 中的 RK3588 `mailbox0` controller，确认 Linux platform driver 能绑定。

主机生成了新的 RAM-only FIT 候选 `build/local/r1-amp-mailbox-probe/r1-boot-fit-amp-mailbox-probe.img`。该候选使用启用 `mailbox0` 的 DTB，resource 中的 `rk-kernel.dtb` 已解包后与输入 `cmp` 一致，DTB 中 `/mailbox@fec60000/status` 为 `okay`。候选大小为 38,636,300 B，SHA-256 为：

```text
38313e25c788ae9454105caca9dfe910b9f2c374de7da23f88594593693c5e22
```

候选通过 SSH 传至板端 `/userdata/r1-mailbox-probe.img`，板端 SHA-256 与主机一致。U-Boot 仅从 `mmc 0:8` 的 userdata 读取该文件并 RAM 启动，未写 eMMC 启动分区或保存 U-Boot 环境。

板端运行时验证结果：

```text
/sys/bus/platform/devices/fec60000.mailbox
/sys/bus/platform/drivers/rockchip-mailbox
okay
```

其中第一行是设备目录存在，第二行是 `readlink -f` 得到的 driver 路径，第三行是运行时 `/proc/device-tree/mailbox@fec60000/status`。此前 `dmesg | grep "mailbox"` 无输出；该现象不能否定 probe，sysfs 绑定和运行时 DTB 才是本次判断依据。

本步骤只证明 `mailbox0` controller 已由 Linux 的 `rockchip-mailbox` 驱动绑定；尚未验证 mailbox client、具体 TX/RX channel、Zephyr mailbox ISR、Linux↔Zephyr doorbell 或 RPMsg/virtio。

### 步骤 7：静态接入 mailbox client 的 DT wiring

在同一 AMP DTS 的 `amp-cpu-on-heartbeat` 节点中加入 mailbox client 描述：

```dts
mbox-names = "amp-rx", "amp-tx";
mboxes = <&mailbox0 0>, <&mailbox0 3>;
```

主机使用已有 kernel `.config` 编译 `rockchip/rk3588s-yyt-amp.dtb` 成功。对编译产物执行 `fdtget` 得到：

```text
amp-rx amp-tx
1a3 0 1a3 3
```

这表示两个 client 引用都指向同一个 `mailbox0` phandle（编译后的值为 `0x1a3`），接收通道为 0、发送通道为 3。该 DTB 仅完成主机侧静态 wiring 验证，尚未重新打包 resource/FIT 或在板端启动；Linux AMP 驱动也尚未调用 `mbox_request_channel_byname()`，因此不能据此宣称 channel 已申请或消息已发送。

### 步骤 8：审计 mailbox IRQ 的方向与候选 Zephyr 路由

主会话确认 `mailbox0` DTS 的四个 Linux IRQ 声明为 `GIC_SPI 61`–`64`。GIC SPI 在 raw INTID 上加 32，故其 raw GIC INTID 为 93–96；板端 `/proc/interrupts` 也将 `fec60000.mailbox` 显示在 93–96。

对 Linux `drivers/mailbox/rockchip-mailbox.c` 的源码审计表明：Linux TX 路径写入 A2B 的 CMD/DAT，而 Linux IRQ handler 读取并清除 B2A。因此，Linux 可见的 93–96 是“对端→Linux”的中断，不是“Linux 通过 TX3→Zephyr”的中断。这纠正了把 controller IRQ 与 Linux TX 通道混为同一对象的假设。

资料/源码对照还发现官方 `rk3588-amp.dtsi` 同时引用 `mailbox0` 通道 0/3，并以 `amp-irqs` 将 raw GIC INTID 100 路由到 CPU3。当前 DTS 与所审计源码没有直接证明 TX3 对应 INTID 100；这只是下一轮 Zephyr ISR/GIC route 验证的候选，不能作为连接结论。

因此验证次序固定为：先让 Zephyr **不依赖中断**、被动轮询 A2B 的状态/command/data，确认 Linux TX3 的实际写入能到达；该观察已在步骤 9 完成。下一步才单独启用并验证 Zephyr 的 GIC route/ISR；步骤 8 本身没有验证 Zephyr ISR、doorbell 或 RPMsg。

### 步骤 9：RAM-only mailbox A2B 只读观察

目的与预期结果：在 RAM-only mailbox probe 实例中，由 Linux 通过 TX3 写入 A2B 后，让 Zephyr CPU3 以只读 MMIO 映射被动观察同一组寄存器；预期两侧在同一时点读到一致的 status/command/data。此步骤不依赖 Zephyr ISR，也不发送 B2A 响应。

主会话确认的运行时观察结果如下：Linux TX3 返回 `mailbox_tx_ret=0`，Linux TX 快照为 `A2B status=0/cmd=1/data=41`；同一时点 Zephyr CPU3 观察到 `mbox_observation marker=0x4d424f58, status=0, cmd=1, data=41`。

该结果证明 A2B 寄存器可见性/路径已验证。它没有验证 Zephyr ISR、中断路由、B2A/Linux RX，也没有验证 RPMsg。

### 步骤 10：CPU3 GIC SPI100 状态观察

目的与预期结果：在 CPU3 执行 `irq_enable(100)` 后，只读观察 Zephyr 侧 GIC 状态，并在一次 Linux TX3 ping 后检查是否出现 mailbox 中断状态；该步骤用于验证候选 SPI100 是否已启用并路由到 CPU3，不据此推断 TX3 的替代中断号。

主会话确认的 Zephyr 共享观察为：`GICE 0x47494345/0x8fc00211/0x0/0x300`。其中 SPI100 的 enable word 含 bit4，pending word 为 `0`，IROUTER low 为 `0x300`。一次 Linux TX3 ping 后仍未出现 MISR，因此候选 IRQ100 对该 TX3 事件未触发。

第二次观察结果变为零的直接原因已确认：Linux ping 实现按设计会先清空观察区；不能把该零值解释为 CPU3 未执行或状态丢失。

本步骤排除了“IRQ100 未启用/未路由”这一假设，但没有证明 IRQ100 是 TX3 事件对应的中断，也没有指定任何替代 INTID。A2B ISR/doorbell 仍未验证。

### 步骤 11：v2 标准 SPI pending 只读扫描

目的与预期结果：在 Linux TX3 PING 前后，只读扫描 Zephyr GICD_ISPENDR 的标准 SPI INTID32–511窗口，比较基线与差分；同时保留 A2B 观察值，用于确认寄存器写入与 pending 扫描结果的边界。

主会话确认的 v2 扫描结果为：标准 SPI INTID32–511 返回 `PN00 0x504e3030/0/0/0`，基线与差分均为零。同一状态下 A2B 观察为 `a2b_at_tx/a2b_now=0/1/0x29`，表明寄存器写入保留，但本扫描窗口未见新增标准 SPI pending。

本步骤结论严格限于“未观测到标准 SPI pending 差分”。这不等同于绝对没有中断，也不否定其他通知路径；A2B ISR/doorbell 仍未验证。

### 步骤 12：A2B_INTEN 只读观察

目的与预期结果：只读观察 A2B_INTEN、A2B_STATUS 以及 TX3 对应的 CMD3/DAT3，比较 Linux TX3 PING 前后的门控与寄存器状态；该步骤用于判断“通知 gate 未打开”是否足以解释未见 pending，不修改寄存器。

主会话确认的结果为：启动时 `A2BI=0x41324249/0/0/0`；一次 Linux TX3 ping 后为 `A2BP=0x41324250/0/0/1`，Linux 同一状态的 A2B 观察为 `a2b_now=0/1/0x29`。因此可明确记录：`A2B_INTEN=0`、`A2B_STATUS=0`、`CMD3=1`、`DAT3=0x29`。

该结果支持“gate 未打开可能解释无 pending”的假设；尚未验证设置 bit3 的语义，也未验证 A2B ISR。

### 步骤 13：A2B gate enable 只读回读

目的与预期结果：在只读观察流程中启用 A2B gate bit3，再比较 Linux TX3 PING 前后的 A2B_INTEN、A2B_STATUS、CMD3/DAT3 和候选 IRQ100 状态；不据此推断具体替代通知线路。

主会话确认的结果为：启动时 `A2BE=0x41324245/0/0x8/0`；PING 后 `A2BP=0x41324250/0x8/0x8/0x1`，Linux 同时观察到 `a2b_at_tx/a2b_now=0x8/0x1/0x29`。

这证明 bit3 写入可以回读，并使 A2B_STATUS bit3 置位。候选 IRQ100 ISR 仍未命中；不能据此断言任何具体替代线路。

### 步骤 14：gate-open GIC pending 只读扫描

目的与预期结果：在 gate-open pending scan candidate 上执行一次 Linux TX3 PING，比较 raw GIC INTID100 的 pending 基线与差分；只读观察 GIC pending，不据此宣称 Zephyr ISR 已执行。

主会话确认的板端结果为：Linux PING `41` 得到 response `42`；A2B 状态为 `a2b_at_tx/a2b_now=0x8/0x1/0x29`，Zephyr 共享观察为 `mbox_observation=0x50454e44/0x64/0x0/0x10`。

`MARK PEND` 记录表明 raw GIC INTID 100 的 pending 基线由 `0` 变为 `0x10`（word3 bit4）。这直接证实 Linux TX3 经 A2B channel 3 gate 产生标准 GIC SPI100 pending。该结果不证明 Zephyr ISR 已触发，也不表示完整 doorbell 已完成。

### 步骤 15：one-shot Zephyr IRQ100 ISR 接收

目的与预期结果：在 gate-open candidate 上受控接收一次 IRQ100，确认 Zephyr ISR 是否读取到 Linux TX3 经 A2B channel 3 gate 产生的事件；该步骤不验证 B2A 或全双工。

主会话确认的板端结果为：one-shot IRQ candidate 在 Linux PING `41` 后 response `result=42`；状态为 `a2b_at_tx/a2b_now=0x8/0x1/0x29`，Zephyr 观察为 `mbox_observation=0x4d495352/0x8/0x1/0x29`（`MISR`），heartbeat 继续到 `5`。Zephyr IRQ100 ISR 实际触发，读取到 A2B status bit3、`CMD3=1`、`DAT3=41`。

ISR 按 one-shot 设计自行 mask，且未确认/ack mailbox。该结果验证 Linux→Zephyr doorbell（mailbox0 A2B ch3 gate→GIC100）；不宣称 B2A 或 full duplex 已验证。

### 步骤 16：B2A one-shot reverse notification

目的与预期结果：在 B2A one-shot candidate 上确认 Zephyr B2A channel 0 写入是否到达 Linux `amp-rx` callback，并保留启动前后计数与 A2B/Zephyr 状态；不把未记录的 IRQ93 计数扩大为本次实测证据。

板端启动前状态为 `image=0, mailbox_rx_count=0`。Zephyr start 并由 Linux PING `41` 后，response 为 `seq=1 valid=1 result=42`；状态为 `mailbox_rx_count=1 mailbox_tx_count=1 mailbox_tx_ret=0 a2b_at_tx/a2b_now=0x8/0x1/0x29 mbox_observation=0x4d495352/0x8/0x1/0x29 magic=HB5 current_el=4`。

`mailbox_rx_count` 从 `0` 到 `1` 证明 Zephyr B2A channel 0 write 到达 Linux `amp-rx` callback；与已验证的 A2B 方向共同形成双向 doorbell 原型。本次未提供或记录 IRQ93 前后计数，不能宣称该单独证据。Linux mailbox driver ack 语义来自源码，实际寄存器清零未直接读取。

### 步骤 18：U-Boot mailbox controller 只读访问矩阵

主会话确认的 U-Boot 只读访问结果为：mailbox0（TRM 对应 `MCU_PMU`）可读，读取窗口返回全 0；mailbox1（对应 `MCU_DDR`）读取异常；mailbox2（对应 `MCU_NPU`）可读，读取窗口返回全 0。全 0 只能说明本次读取窗口的观察值，不能推出 controller 永远空闲；mailbox2 已完成本次用户态读取，后续不再盲读。

该访问矩阵与 Linux probe 结果不能直接等同：U-Boot 阶段访问条件不同，不能据此指定 mailbox1 异常的唯一根因，也不能断言 mailbox1 安全闲置或 DDR 固件未使用。

### 步骤 19：mailbox1 Linux RAM-only probe

独立 RAM-only candidate 仅启用 mailbox1（`mailbox@fec70000`），mailbox0/2 disabled，未加入 mailbox client、Zephyr 或 CPU3 carveout。外置 FIT 载荷布局的候选启动成功；旧的内嵌 data FIT 被厂商 `bootm` 拒绝，仅为封装兼容问题，未进入 Linux。

候选 Linux 中确认 `/sys/bus/platform/devices/fec70000.mailbox` 存在，driver realpath 为 `/sys/bus/platform/drivers/rockchip-mailbox`。这验证 Linux 可正常 probe 并访问 mailbox1；结合驱动源码，probe 会启用 PCLK 并注册 IRQ。U-Boot 读取异常的解释仅限于“启动阶段/访问条件不同”，不能归结为 PCLK 唯一根因。

候选 Linux 的 `grep 'fec70000.mailbox' /proc/interrupts` 显示 GICv3 raw `101,102,103,104` 共四行，所有 CPU 计数均为 `0`。这证明 Linux driver 已注册 mailbox1 的四个 B2A IRQ，且本实验未产生事件；不能据此称固件未使用 mailbox1。

### 步骤 17：连续三轮双向序号验证

主会话确认连续完成三轮单槽 request/response 与 B2A callback 序号对应：

| 轮次 | request/response | Linux callback |
| --- | --- | --- |
| 1 | `seq=1 value=41 result=42` | `rx_count=1 rx_last=0xb2a00001/0x1` |
| 2 | `seq=2 value=100 result=101` | `rx_count=2 rx_last=0xb2a00001/0x2` |
| 3 | `seq=3 value=7 result=8` | `rx_count=3 rx_last=0xb2a00001/0x3` |

第 2/3 轮 Zephyr observation 分别为 `B2AT/0x1/0xb2a00001/0x2` 与 `B2AT/0x1/0xb2a00001/0x3`，heartbeat 分别到 `HB8`、`HB10`；A2B snapshots 保持 `0x8/0x1/0x29`。因此共享响应序号、B2A ACK 序号和 Linux callback 计数一一对应。

本验证仍是单槽、手动 sleep/response polling，不是并发、压力或超时证明。另更正返回值解释：`mbox_send_message` 返回非负 cookie（如 `0/1/2`）表示成功提交，负数才表示失败；之前不能把非零正值表述为错误。

### 步骤 20：mailbox1-only Linux 与 LLM 共存回归

在 mailbox1-only RAM candidate（Linux 已绑定 `fec70000.mailbox`）上，用户运行已验证的 `llm_demo-amp`，对 `ok` 得到 `Alright` 生成；随后同一 candidate 上重复运行多次均正常，后续运行速度快于第一次，但未记录精确次数或性能数值。

该结果限定为当前 R1、RKNPU 0.9.8、指定 RKLLM 模型下 mailbox1 controller bind 与一次/多次实际 NPU 推理共存；不覆盖 DDR DFS、休眠、长期压力，也不证明 mailbox1 可自由占用或解释后续运行更快的原因。

### 步骤 21：all-mailbox Linux bind 与 LLM 共存

RAM-only all-mailbox candidate 中 mailbox0/1/2 均为 `status=okay`，无 mailbox client、Zephyr 或 CPU3 carveout，成功进入 Linux。用户确认 `fec60000.mailbox`、`fec70000.mailbox`、`fece0000.mailbox` 三个设备的 driver 均为 `/sys/bus/platform/drivers/rockchip-mailbox`；其上 `llm_demo-amp` 多次推理正常。

这验证三 controller 同时 Linux bind 与当前 RKNPU/RKLLM 工作负载多次共存；不包括 mailbox1/2 主动消息、固件所有权、低功耗/DFS、长期压力验证，也不授权最终占用。后续架构原则可记录为：共享内存协议与 doorbell 后端分层，controller/channel 由 DTS 配置；该原则不是已实现代码的描述。

### 步骤 22：mailbox0 四通道矩阵单次双向闭环

2026-08-26，R1 RAM-only matrix candidate 在不修改 eMMC 的前提下，对 mailbox0 的四个逻辑通道分别完成一次 Linux→CPU3→Linux 闭环。Linux 发送与 CPU3 回执的共享观察为：ch0 `rx=1 0xb2a00000/0x28 tx=1/0`；ch1 `rx=1 0xb2a00001/0x2a tx=1/0`；ch2 `rx=1 0xb2a00002/0x2b tx=1/0`；ch3 `rx=1 0xb2a00003/0x29 tx=1/0`。CPU3 observation 为 `0x4d345249`（`M4RI`），`a2b_now=0xf`。

该结果验证候选 SPI97–100 路由至 CPU3，以及四个逻辑通道各自独立的 A2B/B2A 闭环。未验证 A2B_STATUS 远端确认/清除语义；本轮 Zephyr 刻意不写 A2B_STATUS，并在每次 IRQ 触发后 one-shot 屏蔽，因此最终 `a2b_now=0xf` 只记录本轮观察状态，不能推出持久化或清除语义。

### 步骤 24：mailbox0 ch3 连续两轮通信

2026-08-26，修正测试客户端 `knows_txdone=false` 后，在当前 RAM candidate 上 mailbox0 ch3 完成连续两次 Linux→CPU3→Linux 通信。第二轮板端状态为：`ch3=rx:2/0xb2a00003/0x2a,tx:2/1 ... a2b_now=0x0/0xa2b00003/0x2a mbox_observation=0x41434b43/0x8/0x0/0x8`。两轮最终 `a2b_now=0x0`，均观察到 pending 清零；其中 `tx` 的非负值 `1` 是 mailbox 队列 cookie，表示成功提交，不是错误码。

该结果仅验证当前 RAM candidate 上同一 mailbox0 ch3 的连续两次 Linux→CPU3→Linux 闭环及每轮最终 pending 为 0；未验证四通道重复、并发通信或 mailbox1/2 写入行为。

### 步骤 25：mailbox0 ch0–ch2 回执与清 pending

同一 2026-08-26 可重复 RAM candidate 上，mailbox0 ch0/ch1/ch2 均完成发送、CPU3 回执并清 pending。板端状态为：`ch0=rx:1/0xb2a00000/0xa,tx:1/0`，`ch1=rx:1/0xb2a00001/0xb,tx:1/0`，`ch2=rx:1/0xb2a00002/0xc,tx:1/0`，并保留 ch3 `rx:2/0xb2a00003/0x2a,tx:2/1`；`a2b_now=0x0/...`，`mbox_observation=0x41434b43/0x4/0x0/0x4`。按 ACKC 探针定义，字段为写前 `A2B_STATUS=0x4`、写后回读 `0x0`、写入掩码 `0x4`，直接对应 ch2。

结合此前 ch3 的 ACKC `0x8/0x0/0x8`，目前至少 ch2/ch3 的 A2B_STATUS 清除已有直接观测；四个逻辑通道的独立闭环可复用，但本次不构成并发、高吞吐或 mailbox1/2 验证。

### 步骤 23：A2B_STATUS ch3 clear probe

在同一 R1 RK3588 mailbox0 ch3、当前 RAM candidate 上，Linux 发送 ch3 后，板端状态为：`ch3=rx:1/0xb2a00003/0x29,tx:1/0 ... a2b_now=0x0/0xa2b00003/0x29 mbox_observation=0x41434b43/0x8/0x0/0x8`。其中 `0x41434b43` 为 `ACKC`；按该探针定义，观测字段依次为写前 `A2B_STATUS=0x8`、写后回读 `0x0`、写入掩码 `0x8`。

直接实验依据表明：在本板 RK3588 mailbox0 ch3、当前 RAM candidate 上，CPU3 向 A2B_STATUS 写 `BIT(3)` 清除了对应 pending 位。尚未验证连续重复门铃、全通道或其他 controller 的清除行为，不能将该结果泛化。

### 步骤 26：12 控制器候选的 mailbox1 ch0 请求

目的与预期结果：在不改变持久化启动介质的前提下，检查 12 控制器候选是否能为 mailbox1 ch0 建立请求路径；若访问触发异常，立即停止该候选，不将其作为通知层。

主会话确认的运行时结果为：mailbox1 ch0 request 在 `rockchip_mbox_startup` 发生 synchronous external abort。该候选因此停止，不继续探索 mailbox1/2，也不把该次异常解释为唯一硬件根因或推广到其他控制器。

本步骤属于 RAM-only 候选边界；没有改变 eMMC、U-Boot 环境或持久化启动配置。

### 步骤 27：mailbox0 四通道版本与协议 groundwork 主机构建

主机侧当前内核/Zephyr mailbox0 四通道版本已编译成功。新建协议 groundwork（`src/amp-protocol/r1_amp_protocol.h/.c`）仅通过主机 C 单元测试；这只说明主机测试输入可编译并通过测试，不代表 Linux↔CPU3 集成、RAM-only FIT 运行或板端 mailbox 通信已经验证。

### 步骤 28：mailbox0 ch0 RAM-only 基线回归

目的与预期结果：在不改动持久化启动介质的前提下，使用当前 mailbox0 四通道基线候选启动 Linux，加载 Zephyr mailbox0 基线固件，并仅回归 ch0 的一次 doorbell/回执路径。

主会话确认的新候选 `r1-boot-fit.img` 已传至板端，SHA-256 前缀为 `85383311…`。启动后运行时为 Linux `5.10.252`、`nproc=7`，`zephyr` carveout 存在。加载 `36,912 B` 的 `zephyr-mbox0-baseline.bin` 后，PSCI `CPU_ON` 返回 `0`；Zephyr 观察到 `current_el=4` 和 heartbeat。

执行 doorbell `0 41` 后，ch0 状态为：`m0c0=rx:1/0xb2a00000/0x29,tx:1/0`，`a2b_now=m0:0`。本次仅验证 ch0 基线回归，不是新协议集成，也不代表本轮已完成四通道全测。

### 步骤 29：RAM-only MailMsg V1 最小闭环

目的与预期结果：在不改动 eMMC 的前提下，启动 MailMsg V1 候选，加载 Zephyr 固件，并验证一次共享内存队列消息经 mailbox0 ch0 通知后返回 PONG。

主会话确认候选 FIT 为 `build/local/r1-mailmsg-v1/r1-boot-fit-mailmsg-v1-external.img`，SHA-256 为 `e7c73a7d3628654badbd3a98559307f32325519043049a32d2abd39d19070400`；Zephyr `/userdata/zephyr-test/mailmsg-v1.bin` SHA-256 为 `a2b386ebda0493c7aa72f7a7f07156af0872e51ee9874db2cb365a57ec9ebf46`。启动后 Linux 为 `5.10.252`、`nproc=7`；CPU3 PSCI `CPU_ON ret=0`，Zephyr `current_el=4`。

Linux `mailmsg_ping` 写入 priority `0`、value `41`；`mailmsg_response` 返回 `priority=0 valid=1 type=2 sequence=1 value=42`。状态为 `m0c0=rx:1/0xb2a10000/0x0,tx:1/0`、`a2b_now=m0:0x0`。

本步骤仅证明一次共享内存 MailMsg 队列与 mailbox0 ch0 通知的 PING/PONG；未验证优先级 1–3、负载/并发、大数据、可靠性或持久化 eMMC。

### 步骤 30：MailMsg V1 连续四优先级消息

同一 2026-08-27 RAM-only MailMsg V1 session 中，连续写入 priority/value `0/10`、`1/20`、`2/30`、`3/40`。`mailmsg_response` 依次返回 `p0 seq2 val11`、`p1 seq3 val21`、`p2 seq4 val31`、`p3 seq5 val41`，均为 `type=2 valid=1`。

对应状态为：`m0c0 rx2 cmd 0xb2a10000 data0 tx2/1`；`m0c1 rx1 0xb2a10001 data1 tx1/0`；`m0c2 rx1 0xb2a10002 data2 tx1/0`；`m0c3 rx1 0xb2a10003 data3 tx1/0`；`a2b_now=m0:0`。

该结果证明四 priority 的独立映射和一次连续四消息的正确 PING/PONG；不证明抢占调度、并发/高负载、队列满策略、长期稳定或大数据。

### 步骤 31：RAM-only MailMsg V2 ch0 正常路径

目的与预期结果：在不改变持久化启动介质的前提下，验证 V2 固定帧及 CRC32 的板端正常路径，经 mailbox0 ch0 完成一次共享内存 PING/PONG。

前一次 V2 候选使用旧的 `36,912 B` DTS sysfs loader size 上限，写入时出现 `cat: File too large`，固件被截断；该次结果无效，不作为 V2 运行证据。修正 DTS sysfs loader size，从 `0x9030` 更新为 `0xa030` 后重新验证。

修正后的运行结果为：Zephyr image 完整写入，`image=41008/41008`；PSCI `CPU_ON ret=0`。Linux priority `0`、value `41` 后，`mailmsg_response` 返回 `type=2 seq=1 value=42`；状态为 `m0c0 rx 0xb2a10000/0x0, tx 1/0`，pending 为 `0`。

本步骤仅验证 CRC32 V2 的板端正常路径（一次 ch0 PING/PONG）。payload 篡改后的拒收仍只有通用 host C unit test 证据；本步骤不证明板端篡改拒收、故障恢复、其他 priority、并发、压力或长期稳定性。

### 步骤 32：RAM-only MailMsg V2 CRC fault candidate

目的与预期结果：在不改变持久化启动介质的前提下，使用测试专用注入验证 V2 对 payload 损坏的检测/拒收，并观察坏帧对同一 priority ring 及其他 priority ring 的影响。

Linux test-only injection 在 CRC 计算完成、producer 发布前翻转 payload；该注入不代表正常发送路径。正常 priority 0 value `41` 仍返回 PONG `42`。注入后 Zephyr 可观察到 `CRCE/0/-4/1`，没有 PONG，CPU3 heartbeat 持续。

随后在同一 priority 0 ring 发送正常 value `200`，仍无响应；在 priority 1 发送 value `300`，得到 PONG `301`。本次观察表明坏帧留在队头时会堵住同一 priority 的后续消息，而 priority 1 ring 仍可独立处理；不扩展为通用调度或恢复语义。

本步骤已验证板端 CRC V2 的检测、拒收和 priority ring 隔离。原地恢复、丢帧/跳过、重试、重置等策略尚未定义或验证；也未验证并发、压力、长期稳定性或其他 priority 的故障处理。

### 步骤 33：RAM-only MailMsg V3 可靠性策略回归

目的与预期结果：在不改变持久化启动介质的前提下，验证 V3 固定的 priority 可靠性策略：priority 0/1 使用 ACK/NACK，priority 2/3 不使用 ACK/NACK；错误帧应释放 slot，协议不自动重传。

主机构建输入为 RAM-only FIT `build/local/r1-mailmsg-v3-reliability/r1-mailmsg-v3-reliability.img`，SHA-256 为 `82cb3a1f1169602321ffc51978e5633d9fa72471b7210bf74211a4a41d346aed`；Zephyr `mailmsg-v3-reliability.bin` 大小 `41008 B`，SHA-256 为 `f217a3510f82fbf2a0ff7131fe66d8e93f6d2809ffc9d10ebf02e5a401d10e2b`。板端加载完整，`image=41008/41008`；CPU3 启动成功，状态为 `affinity=off(1) cpu_on_ret=0 current_el=4`。上述路径仅为 RAM-only 候选，未写启动分区或 U-Boot 环境。

已观察到以下运行结果：

1. priority 0 正常请求 `41` 收到 ACK：`type=3 sequence=1 peer_sequence=1 status=0`，随后收到 PONG：`type=2 sequence=2 value=42`。
2. priority 0 的 CRC 注入请求原序列 `2` 收到 NACK：`type=4 sequence=3 peer_sequence=2 status=1`，没有 PONG。随后立即发送正常 priority 0 请求 `200`，收到 ACK：`type=3 sequence=4 peer_sequence=3 status=0`，以及 PONG：`type=2 sequence=5 value=201`。这证明已提交的坏帧释放 slot，未阻塞同一 priority 0 ring。
3. priority 2 的 CRC 注入返回 `valid=0 reason=empty`，没有反馈消息；随后正常 priority 2 请求 `200` 仅收到 PONG：`priority=2 type=2 sequence=7 value=201`。这证明 best-effort priority 的坏帧立即丢弃并可继续处理后续消息。
4. priority 1 正常请求 `300` 收到 ACK：`priority=1 valid=1 type=3 sequence=8 peer_sequence=7 status=0`，随后收到 PONG：`priority=1 valid=1 type=2 sequence=9 value=301`。CRC 注入请求 `400` 收到 NACK：`priority=1 valid=1 type=4 sequence=10 peer_sequence=8 status=1`，没有 PONG。

本步骤已验证板端 V3 的固定策略：priority 0/1（本步骤均覆盖正常 ACK 与 CRC 错误 NACK）使用可靠反馈，priority 2（本步骤覆盖）不使用 ACK/NACK，错误帧释放 slot；priority 3 同属无反馈类但尚未板端验证。ACK 表示传输层校验/接收确认，不表示应用处理完成。自动重传明确未实现，后续由发送端策略决定重传、丢弃、降级或复位。本步骤未验证 priority 3、并发、压力、长期稳定性或 eMMC 持久化。

### 步骤 34：RAM-only MailMsg V3 queue-full 回归

目的与预期结果：在 CPU3 已启动且 Zephyr 尚未消费前，验证 p3 ring 的 7-slot 可用上限，以及第 8 条入队立即返回 `FULL`/空间不足而不覆盖已有消息。

此前尝试在 CPU3 未启动时灌队列无效：Linux `start` 会在 PSCI `CPU_ON` 前重置共享协议区，源码与观察均确认这是测试顺序问题，不是丢帧。修正为先加载 Zephyr、执行 `start`，确认 `cpu_on_ret=0`、`current_el=4`，再进行入队。

本次 RAM-only FIT 位于板端 `/userdata/r1-ram-boot-test/r1-mailmsg-v3-queue-full.img`，SHA-256 为 `6bec6de793ce20e770a2436db860fde7c362f335b22f52460aa2e2fe1e538d81`；Zephyr 位于 `/userdata/zephyr-test/mailmsg-v3-queue-full.bin`，大小 `41008 B`，SHA-256 为 `96dbf38cdcfa79f7d28afd48e6b8d6ed9e0b5f646b1a0ae06f704a74b498448d`。本次不写 eMMC。

Zephyr 在首次 doorbell 前不消费；对 p3 test-only `mailmsg_queue_push` 连续写入 `1..7` 均成功，第 8 条返回 `No space left on device`，退出码为 `1`。随后执行 raw `doorbell 3 0`（不新增 MailMsg 帧）唤醒消费者，`mailmsg_response` 按序返回 7 条 p3 PONG：`seq1..7`、`value2..8`。

该步骤已验证运行期 ring 有 8 个物理 slot、7 个可用 slot；`FULL` 立即非阻塞返回且不覆盖，已入队消息保留并在通知后按序消费。doorbell 仅为通知，不承载消息数据。队列满后的重试、丢弃、降级或报警仍由上层调用者决定，自动重传未实现。本步骤不覆盖并发、压力、长期稳定或持久化。

### 步骤 35：RAM-only MailMsg V3 endpoint 集成回归

目的与预期结果：在不改变持久化启动介质的前提下，验证 Linux 与 Zephyr 使用通用 endpoint API 完成四个 priority 的代表路径，并保留 CRC 注入测试钩子的错误反馈观察。

主机新增通用 endpoint 层 `src/mailmsg/mailmsg_endpoint.{h,c}`：一次生命周期绑定 Linux/CPU3 角色和本端序号，提供发送（入队后调用通知）与接收；入队结果和通知结果分离。endpoint 不包含调度、线程、重传或业务策略。主机 endpoint、protocol、notify、mailbox0 四项测试通过；Zephyr 完整构建成功，固件大小 `41008 B`、SHA-256 为 `e999bb38cceaecfb56e9855b65fe1261ef175d2ad6bd3aa89b82d058aedf5c86`。Linux 测试驱动的正常 `mailmsg_ping`/`mailmsg_response` 已使用 endpoint API；CRC 注入和 queue-without-doorbell 仍保留为测试钩子的直接构帧路径。Linux Image 完整构建成功，SHA-256 为 `dbf6eb967759e5084059713f178b3ab0ef95410d85658703ba1476b089508ef1`，`vmlinux` 含 endpoint/ring 符号。

本次 RAM-only FIT 为 `/userdata/r1-ram-boot-test/r1-mailmsg-endpoint.img`，大小 `38636544 B`，SHA-256 为 `56b9a8fe0f5aa0f130ab1a7ab17cc8c0a840953d6ef3a8bc6f24067e062741c`；配套 Zephyr 固件为 `/userdata/zephyr-test/mailmsg-endpoint.bin`。本次不写 eMMC。板端 Linux 为 `5.10.252`、`nproc=7`；Zephyr `image=41008/41008`，`cpu_on_ret=0`、`current_el=4`，心跳持续。

板端正常请求结果如下：

1. p0 输入 `41`：ACK `type=3 sequence=1 peer_sequence=1 status=0`，随后 PONG `type=2 sequence=2 value=42`。
2. p1 输入 `100`：ACK `type=3 sequence=3 peer_sequence=2 status=0`，随后 PONG `type=2 sequence=4 value=101`。
3. p2 输入 `200`：仅 PONG `type=2 sequence=5 value=201`。
4. p3 输入 `300`：仅 PONG `type=2 sequence=6 value=301`。

四个通道各为 `rx=1`、`tx=1`，通知结果均为 `SENT`。随后 p0 CRC 注入 `400` 返回 NACK `type=4 sequence=7 peer_sequence=5 status=1`，无 PONG；状态为 `CRCE`，CPU3 心跳继续。该错误路径仍是测试钩子的直接构帧，正常路径则经 endpoint API。由此验证当前候选的 p0/p1 ACK、p2/p3 无 ACK/NACK 代表路径及 p0 CRC 错误反馈；不将一次四通道回归扩展为并发、压力或完整产品验证。queue-full 本轮未重新测试，步骤 34 的 p3 正常及 FULL 代表路径结论保持不变；自动重传仍未实现。

### 步骤 36：RAM-only MailMsg V3 混合突发与满队列隔离回归

目的与预期结果：在新的 endpoint candidate 会话中，验证四个独立 priority ring 的 FIFO 消费、test-only 无 doorbell 预填与 raw doorbell 唤醒关系，并验证 p3 满队列不会阻塞其他 priority 的入队、通知或消费。

本次运行环境为 Linux `5.10.252`、`nproc=7`；Zephyr `image=41008/41008`，`CPU_ON ret=0`，已到达 EL1。第一段测试使用 test-only `mailmsg_queue_push` 预填且不发送 doorbell：p0 写入 `100..102`、p1 写入 `200..202`、p2 写入 `300..306`、p3 写入 `400..406`；随后用 raw `doorbell 0..3` 分别唤醒。结果为 p0 ACK/PONG `101..103`、p1 ACK/PONG `201..203`、p2 仅 PONG `301..307`、p3 仅 PONG `401..407`。四个 priority 均按各自 FIFO 顺序消费，未见串帧、漏帧或 CRC 错误。该 raw doorbell 不经过 endpoint notify；状态中的 `notify=-11` 是直接 raw 唤醒路径的观察值，不应解释为 endpoint 通知失败。

第二段在干净会话中先用 test-only 路径预填 p3 `500..506`，第 8 条 `507` 返回 `ENOSPC`，退出码为 `1`。随后通过正常 endpoint 发送 p0 `900`，得到 ACK `type=3 sequence=1 peer_sequence=9 status=0` 和 PONG `type=2 sequence=2 value=901`；p3 随后返回 PONG `sequence=3..9`、`value=501..507`，全部成功消费。状态观察为 m0c0 通知 `SENT`、m0c3 使用 raw doorbell、`a2b_now=0`。因此本次代表路径中 p3 `FULL` 未阻塞 p0 入队、通知或消费，也未丢失已入队的 p3 帧。`peer_sequence=9` 源于 test-only 入队对第 8 次失败尝试仍消耗本地序号，是当前测试实现观察，不是串队证据。

本步骤验证的是当前 mailbox0 四通道后端的顺序突发与满队列隔离代表路径；不覆盖并发生产者/消费者、全局调度公平性或抢占、吞吐/延迟、长期稳定性、自动重传、大载荷或 eMMC 持久化。

### 步骤 37：RAM-only MailMsg V3 TX-full 观测

目的与预期结果：修正 FIT 封装方式后，在不改变持久化启动介质的前提下，观察 endpoint 正常 p0 请求及反向 PONG 入队满时的诊断状态；该观察不改变重试、重排或丢弃策略。

主机默认 `mkimage -f` 生成的是内嵌数据 FIT；vendor U-Boot 读取后打印 usage。此前可工作的候选使用外置 `data-position/data-size`。本次以 `mkimage -E -p 0x800 -B 0x200` 重新封装最终候选，恢复 FDT `0x800`、kernel `0x39a00`、resource `0x2427c00` 的偏移，总大小 `38636544 B`，RAM boot 成功。板端 artifact 为 `/userdata/r1-ram-boot-test/r1-mailmsg-tx-full-observation-external.img`，SHA-256 为 `5ebf4a8222d343cdc319311ea18ebf1e376f6ec8a2ecbd08f154b9f25f7eec0c`；Zephyr 为 `/userdata/zephyr-test/mailmsg_tx_full_observation.bin`，大小 `41008 B`，SHA-256 为 `8aab1e3fc0838f36e840ab4ec551164020942a43363152287d87641a2bfd6a99`。Linux 启动为 `5.10.252`、`nproc=7`；Zephyr 初始化 TX-full 诊断状态为 `valid=1 commit=1 count=0 priority=4 type=0 result=0`。

正常 endpoint p0 发送 `600..603` 均退出码 `0`。板端 response 依次为 ACK/PONG：ACK `sequence=1`、PONG `sequence=2 value=601`；ACK `sequence=3`、PONG `sequence=4 value=602`；ACK `sequence=5`、PONG `sequence=6 value=603`；以及 ACK `sequence=7 peer_sequence=4`。随后状态为 `mailmsg_tx_full valid=1 commit=2 count=1 priority=0 type=2 result=-2`。这确认反向 PONG 入队在已有 7 个 response frame 后出现一次 TX-full 观测（`count:1`）；状态只作报告，未引入自动重传、重排、丢弃或其他策略变化。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 共享内存协议布局 | request/response 各由独立 64 B cache line 承载 | Zephyr 源码静态断言和固定地址布局满足 | 通过（静态） |
| 双方 cache maintenance | Linux/Zephyr 对交接数据显式维护 cache | Zephyr 端 invalidate/flush 与 `dsb sy` 已构建；Linux 端已编入 ping/response 接口 | 通过（主机静态） |
| Linux Image、AMP DTB、Zephyr | 可生成且 DTS image-size 与 bin 一致 | 三项构建通过，Zephyr `0x9020` 与 DTS 同步 | 通过（静态） |
| resource/FIT 封装 | resource 条目、logo、FIT 回读一致且小于 64 MiB | `cmp` 通过，FIT 38,636,544 B | 通过（静态） |
| RAM 启动与 Linux 侧接口 | 候选进入 Linux，sysfs ping/response 可用 | `nproc=7`、reserved 节点存在，五个接口均存在，image `36896/36896` | 通过（运行时） |
| PSCI 与 Zephyr 执行 | 一次 `start` 后 CPU3 交接并到达 EL1 | `cpu_on_ret=0`、`magic=0x48420001`、`current_el=4` | 通过（运行时） |
| Linux→Zephyr→Linux PING | 顺序请求均得到 response，序列和结果可回读 | `request_seq=1`–`8` 均 `valid=1`；值与结果依次为 `41→42`、`100→101`、`7→8`、`200→201`、`201→202`、`202→203`、`203→204`、`204→205` | 通过（八次顺序运行时） |
| RKLLM + PING 同负载共存 | RKLLM 生成后无需重启 Zephyr 即可完成 PING 回环 | `llm_demo-amp` 启用 CPU `[3,4,5,6]`、count `4`，`rkllm init success`，`ok` 输出 `Alright,`；随后 `request_seq=3 ... value=7 ... result=8`，状态 `magic=0x4842052d current_el=4` | 通过（一次同负载回归） |
| mailbox0 controller 绑定 | 新 DTB 启用 mailbox0 后 Linux platform driver 绑定 | 设备 `/sys/bus/platform/devices/fec60000.mailbox` 存在，driver 为 `/sys/bus/platform/drivers/rockchip-mailbox`，运行时 status 为 `okay`；FIT SHA-256 与板端一致 | 通过（RAM-only controller probe） |
| mailbox client DT wiring | 编译产物包含 RX/TX 名称和 mailbox 通道引用 | `fdtget` 输出 `amp-rx amp-tx` 与 `1a3 0 1a3 3`；尚未进入 resource/FIT 或板端运行时 | 通过（静态） |
| mailbox IRQ 方向 | 区分 Linux controller IRQ 与 Linux TX 通道 | DTS `GIC_SPI 61`–`64` 对应 raw INTID 93–96，`/proc/interrupts` 显示同一范围；Rockchip mailbox 源码显示 Linux IRQ 接收/清除 B2A，Linux TX 写 A2B CMD/DAT | 通过（源码与板端对照） |
| TX3 与 Zephyr IRQ 的对应 | 找到可验证候选但不提前等同 | 官方 AMP DTS 使用 mailbox0 0/3 并路由 raw INTID100 至 CPU3；没有直接 TX3=100 证据 | 待验证 |
| Linux TX3→Zephyr CPU3 A2B 可见性 | Zephyr 被动观察到 Linux TX3 的写入 | Linux `mailbox_tx_ret=0`；Linux 读到 `A2B status=0/cmd=1/data=41`，同一时点 Zephyr 读到 `marker=0x4d424f58, status=0, cmd=1, data=41` | 通过（RAM-only 只读 MMIO 观察） |
| CPU3 GIC SPI100 状态 | 确认候选 IRQ100 的 enable、pending 和路由状态 | `GICE 0x47494345/0x8fc00211/0x0/0x300`；enable word 含 bit4、pending word 为 `0`、IROUTER low 为 `0x300`；Linux TX3 ping 后无 MISR | 通过（排除未启用/未路由）；TX3 对应关系待验证 |
| v2 标准 SPI pending 扫描 | 比较 Linux TX3 PING 前后的 GICD_ISPENDR 标准窗口 | INTID32–511 返回 `PN00 0x504e3030/0/0/0`，基线/差分均为零；同时 `a2b_at_tx/a2b_now=0/1/0x29` | 未观测到标准 SPI pending 差分 |
| A2B_INTEN 只读观察 | 比较门控、状态和 TX3 寄存器 | 启动 `A2BI=0x41324249/0/0/0`；TX3 ping 后 `A2BP=0x41324250/0/0/1`，Linux `a2b_now=0/1/0x29`；即 `A2B_INTEN=0`、`A2B_STATUS=0`、`CMD3=1`、`DAT3=0x29` | 支持 gate 未打开可能解释无 pending；bit3 语义/ISR 待验证 |
| A2B gate enable 只读回读 | 验证 bit3 写入、状态置位及 TX3 观察 | 启动 `A2BE=0x41324245/0/0x8/0`；PING 后 `A2BP=0x41324250/0x8/0x8/0x1`，Linux `a2b_at_tx/a2b_now=0x8/0x1/0x29` | bit3 可回读并使 A2B_STATUS bit3 置位；IRQ100 ISR 未命中 |
| gate-open GIC pending 扫描 | 比较 Linux TX3 PING 前后的 raw GIC INTID100 pending | PING `41`→`42`；`a2b_at_tx/a2b_now=0x8/0x1/0x29`；`mbox_observation=0x50454e44/0x64/0x0/0x10`；pending 基线 `0`→`0x10`（word3 bit4） | 通过（确认 A2B ch3 gate 产生 SPI100 pending）；ISR/完整 doorbell 待验证 |
| one-shot Zephyr IRQ100 ISR 接收 | 受控接收一次 A2B ch3 gate 事件 | PING `41`→`result=42`；`a2b_at_tx/a2b_now=0x8/0x1/0x29`；`mbox_observation=0x4d495352/0x8/0x1/0x29`（MISR）；heartbeat 到 `5`；ISR 读到 status bit3、`CMD3=1`、`DAT3=41` | 通过（Linux→Zephyr doorbell）；ISR 自行 mask、未 ack；B2A/full duplex 待验证 |
| B2A one-shot reverse notification | Zephyr B2A ch0 write 到达 Linux amp-rx callback | 启动前 `image=0, mailbox_rx_count=0`；PING 后 `seq=1 valid=1 result=42`，`mailbox_rx_count=1 mailbox_tx_count=1 mailbox_tx_ret=0`，`a2b_at_tx/a2b_now=0x8/0x1/0x29`，`mbox_observation=0x4d495352/0x8/0x1/0x29`，`magic=HB5 current_el=4` | 通过（B2A callback 计数0→1）；IRQ93 计数及实际寄存器清零未直接验证 |
| U-Boot mailbox 只读访问矩阵 | 比较 mailbox0/1/2 访问条件与观察值 | mailbox0→MCU_PMU 可读全0；mailbox1→MCU_DDR 读异常；mailbox2→MCU_NPU 可读全0 | 通过（只记录本次窗口）；不推出永久空闲或唯一根因 |
| mailbox1 Linux RAM-only probe | mailbox1 启用后 Linux platform driver probe | `/sys/bus/platform/devices/fec70000.mailbox` 存在，driver `/sys/bus/platform/drivers/rockchip-mailbox`；外置 FIT 启动成功，旧内嵌 data FIT 仅被 bootm 拒绝 | 通过（Linux probe/PCLK/IRQ 注册由源码支持）；U-Boot 异常原因未唯一确定 |
| mailbox1 B2A IRQ 注册 | Linux 暴露 mailbox1 的四个 B2A IRQ | `/proc/interrupts` 显示 GICv3 raw `101,102,103,104` 四行，所有 CPU 计数为 `0` | 通过（已注册且本实验无事件）；不推出固件未使用 |
| mailbox1-only LLM 共存 | mailbox1 bind candidate 上运行 NPU LLM | `llm_demo-amp` 对 `ok` 生成 `Alright`；随后多次重复回归正常，后续运行更快但无精确次数/数值 | 通过（当前 candidate 共存）；不代表 DFS/休眠/长期压力或最终占用 |
| all-mailbox Linux bind 与 LLM | 三 controller 同时 bind 后运行 LLM | `fec60000/fec70000/fece0000.mailbox` driver 均为 `rockchip-mailbox`；`llm_demo-amp` 多次推理正常 | 通过（当前工作负载多次共存）；不验证主动消息、固件所有权、低功耗/DFS/长期压力 |
| 连续三轮双向序号 | request/response、B2A ACK 和 callback 计数连续对应 | `1: 41→42, rx=1, rx_last=.../0x1`；`2: 100→101, rx=2, rx_last=.../0x2`；`3: 7→8, rx=3, rx_last=.../0x3`；观察 `B2AT/.../0x2`、`B2AT/.../0x3`，heartbeat `HB8/HB10`，A2B `0x8/0x1/0x29` | 通过（连续单槽双向序号）；非并发/压力/超时 |
| mailbox0 四通道矩阵闭环 | ch0–ch3 分别完成 Linux→CPU3→Linux 单次闭环 | ch0 `rx=1 0xb2a00000/0x28 tx=1/0`；ch1 `rx=1 0xb2a00001/0x2a tx=1/0`；ch2 `rx=1 0xb2a00002/0x2b tx=1/0`；ch3 `rx=1 0xb2a00003/0x29 tx=1/0`；CPU3 observation `0x4d345249`（M4RI），`a2b_now=0xf` | 通过（候选 SPI97–100 路由至 CPU3，四通道独立 A2B/B2A 闭环）；A2B_STATUS 远端确认/清除语义未验证 |
| A2B_STATUS ch3 clear | 验证 CPU3 写 BIT(3) 是否清除对应 pending 位 | ch3 `rx:1/0xb2a00003/0x29,tx:1/0`；`a2b_now=0x0/0xa2b00003/0x29`；`mbox_observation=0x41434b43/0x8/0x0/0x8`（ACKC；写前/写后/掩码） | 通过（限 mailbox0 ch3 当前 RAM candidate）；连续重复门铃、全通道及其他 controller 未验证 |
| mailbox0 ch3 连续两轮通信 | 修正 `knows_txdone=false` 后重复验证同一 ch3 闭环及清零 | 第二轮 `ch3=rx:2/0xb2a00003/0x2a,tx:2/1`；`a2b_now=0x0/0xa2b00003/0x2a`；`mbox_observation=0x41434b43/0x8/0x0/0x8`；两轮最终 pending 均为 0 | 通过（限当前 RAM candidate mailbox0 ch3）；非负 `tx=1` 为成功提交 cookie；不覆盖四通道重复、并发或 mailbox1/2 |
| mailbox0 ch0–ch2 回执与清 pending | 验证 ch0/ch1/ch2 发送、CPU3 回执及 pending 清零 | ch0 `rx:1/0xb2a00000/0xa,tx:1/0`；ch1 `rx:1/0xb2a00001/0xb,tx:1/0`；ch2 `rx:1/0xb2a00002/0xc,tx:1/0`；`a2b_now=0x0/...`；`mbox_observation=0x41434b43/0x4/0x0/0x4`（ACKC，ch2 写前/写后/掩码） | 通过（限当前 RAM candidate）；结合 ch3 可确认至少 ch2/ch3 直接观测清除；不覆盖并发、高吞吐或 mailbox1/2 |
| 12 控制器候选 mailbox1 ch0 request | 建立 mailbox1 ch0 请求路径 | `rockchip_mbox_startup` 发生 synchronous external abort；候选停止 | 失败（RAM-only 候选）；不将异常归结为唯一根因，不继续探索 mailbox1/2 |
| mailbox0 四通道版本主机构建 | 当前内核/Zephyr mailbox0 四通道版本可编译 | 主机编译成功；未上板 | 通过（主机静态） |
| 协议 groundwork 主机 C 单元测试 | `r1_amp_protocol.h/.c` 可由主机测试验证 | C 单元测试通过；尚未 Linux↔CPU3 集成、上板或 RAM-only 运行 | 通过（主机单元测试；非板端证据） |
| mailbox0 ch0 RAM-only 基线回归 | 基线候选启动、CPU3 进入 Zephyr 后完成一次 ch0 doorbell/回执 | `r1-boot-fit.img` 已传板（SHA-256 前缀 `85383311…`）；Linux `5.10.252`、`nproc=7`、Zephyr carveout 存在；`zephyr-mbox0-baseline.bin` 36,912 B，PSCI `CPU_ON ret=0`，`current_el=4`、heartbeat；`0 41` 后 `m0c0=rx:1/0xb2a00000/0x29,tx:1/0`、`a2b_now=m0:0` | 通过（RAM-only、仅 ch0；非新协议集成，非四通道全测） |
| RAM-only MailMsg V1 最小闭环 | MailMsg priority 0 消息经 mailbox0 ch0 通知后返回 PONG | FIT SHA-256 `e7c73a7d3628654badbd3a98559307f32325519043049a32d2abd39d19070400`；Zephyr SHA-256 `a2b386ebda0493c7aa72f7a7f07156af0872e51ee9874db2cb365a57ec9ebf46`；Linux `5.10.252`、`nproc=7`、CPU_ON ret=0、`current_el=4`；`priority=0 valid=1 type=2 sequence=1 value=42`；`m0c0=rx:1/0xb2a10000/0x0,tx:1/0`、`a2b_now=m0:0x0` | 通过（一次 RAM-only PING/PONG；未验证 priority 1–3、负载/并发、可靠性或 eMMC） |
| MailMsg V1 连续四优先级消息 | 连续 priority 0–3 消息分别返回 value+1 且保持有效 | `p0 seq2 val11`、`p1 seq3 val21`、`p2 seq4 val31`、`p3 seq5 val41`，均 `type=2 valid=1`；m0c0–m0c3 分别收到对应 controller 数据，`a2b_now=m0:0` | 通过（一次连续四消息；不覆盖抢占、并发/高负载、队列满、长期稳定或大数据） |
| RAM-only MailMsg V2 ch0 正常路径 | V2 帧经 mailbox0 ch0 完成一次 PING/PONG，CRC32 正常路径可运行 | 修正 DTS loader size `0x9030→0xa030` 后，`image=41008/41008`；`CPU_ON ret=0`；`type=2 seq=1 value=42`；`m0c0 rx 0xb2a10000/0x0, tx 1/0`，pending `0` | 通过（仅板端 CRC V2 正常路径；篡改拒收仍仅 host unit test） |
| RAM-only MailMsg V2 CRC fault candidate | 注入 payload 损坏后板端拒收，且其他 priority ring 可继续处理 | CRC 计算后/producer 发布前翻转 payload；Zephyr `CRCE/0/-4/1`、无 PONG、CPU3 heartbeat 持续；priority 0 后续 value `200` 无响应；priority 1 value `300→301` | 通过（板端检测/拒收/priority ring 隔离；恢复、丢帧/重试/重置未定义且未验证） |
| RAM-only MailMsg V3 可靠性策略 | priority 0/1 ACK/NACK，priority 2/3 无反馈；错误帧释放 slot；不自动重传 | FIT `82cb3a1f…d346aed`；Zephyr `41008 B`、SHA-256 `f217a351…d10e2b`；`image=41008/41008`、`CPU_ON ret=0`、`current_el=4`；p0 正常 `41`→ACK `type=3 seq=1 peer=1 status=0`→PONG `type=2 seq=2 value=42`；p0 CRC 注入原 seq2→NACK `type=4 seq=3 peer=2 status=1`、无 PONG，随后 p0 `200`→ACK `type=3 seq=4 peer=3 status=0`→PONG `seq=5 value=201`；p1 正常 `300`→ACK `type=3 seq=8 peer=7 status=0`→PONG `type=2 seq=9 value=301`，CRC 注入原 seq8→NACK `type=4 seq=10 peer=8 status=1`、无 PONG；p2 CRC 注入 `valid=0 reason=empty`，随后 p2 `200`→PONG `type=2 seq=7 value=201` | 通过（板端验证 p0/p1 可靠 ACK/NACK、坏帧释放 slot、p2 无反馈/立即丢弃；未覆盖 p3、自动重传、并发/压力或 eMMC） |
| RAM-only MailMsg V3 queue-full | p3 ring 达到 7 条可用上限后第 8 条立即 FULL，不覆盖；通知后 7 条按序消费 | FIT `/userdata/r1-ram-boot-test/r1-mailmsg-v3-queue-full.img` SHA-256 `6bec6de793ce20e770a2436db860fde7c362f335b22f52460aa2e2fe1e538d81`；Zephyr `41008 B` SHA-256 `96dbf38cdcfa79f7d28afd48e6b8d6ed9e0b5f646b1a0ae06f704a74b498448d`；`cpu_on_ret=0/current_el=4`；p3 push `1..7` 成功，第 8 条 `No space left on device`、exit `1`；raw `doorbell 3 0` 后 PONG `seq1..7/value2..8` | 通过（p3 正常路径及 FULL 代表路径；不覆盖并发/压力/持久化；doorbell 仅通知） |
| RAM-only MailMsg V3 endpoint 集成 | Linux/Zephyr endpoint API 完成 p0–p3 正常回归；p0 CRC 注入返回 NACK；通知均为 SENT | FIT `/userdata/r1-ram-boot-test/r1-mailmsg-endpoint.img`，38636544 B，SHA-256 `56b9a8fe0f5aa0f130ab1a7ab17cc8c0a840953d6ef3a8bc6f24067e062741c`；Zephyr `41008 B`，SHA-256 `e999bb38cceaecfb56e9855b65fe1261ef175d2ad6bd3aa89b82d058aedf5c86`；Linux `5.10.252`、`nproc=7`、`cpu_on_ret=0`、`current_el=4`；p0/p1 ACK+PONG、p2/p3 PONG、p0 CRC NACK 无 PONG，四通道 `rx=1 tx=1` | 通过（当前 mailbox0 四通道后端/代表路径；不覆盖并发/压力/queue-full 复测/完整产品） |
| RAM-only MailMsg V3 混合突发与满队列隔离 | test-only 预填四个 priority 后 raw doorbell 按 FIFO 消费；p3 满队列后 p0 仍可正常 endpoint 入队/通知/消费 | 第一段 p0/p1 ACK+PONG、p2/p3 PONG，分别返回 `101..103`、`201..203`、`301..307`、`401..407`；第二段 p3 `500..506` 入队，第 8 条 `507` 返回 `ENOSPC` exit `1`，p0 `900` ACK `seq1 peer9`/PONG `seq2 value901`，p3 PONG `seq3..9/value501..507`；m0c0 `SENT`、m0c3 raw doorbell、`a2b_now=0` | 通过（当前 mailbox0 四通道后端/代表路径；不覆盖并发/公平性/抢占/吞吐延迟/长期稳定/自动重传/大载荷/eMMC） |
| RAM-only MailMsg V3 TX-full 观测 | 修正为外置数据 FIT 后完成 RAM boot，并观察反向 PONG 入队满状态 | FIT `/userdata/r1-ram-boot-test/r1-mailmsg-tx-full-observation-external.img`，SHA-256 `5ebf4a8222d343cdc319311ea18ebf1e376f6ec8a2ecbd08f154b9f25f7eec0c`；Zephyr `41008 B`，SHA-256 `8aab1e3fc0838f36e840ab4ec551164020942a43363152287d87641a2bfd6a99`；p0 `600..603` 均 exit `0`，ACK/PONG `601..603` 后 ACK `seq7 peer4`；`mailmsg_tx_full valid=1 commit=2 count=1 priority=0 type=2 result=-2` | 通过（TX-full 诊断观察；不改变重试/重排/丢弃策略） |

## 结论

已完成最小共享内存 PING 原型的主机侧 Linux/Zephyr/DTB/resource/FIT 构建与回读核验，并在当前 RAM candidate 上完成序列 `1`–`8` 的顺序运行时回归。标准 PSCI 启动后的 Zephyr 与 Linux 对八次单槽请求均返回 `valid=1` 且结果正确；其中同一实例中 `llm_demo-amp` 以显式 CPU `[3,4,5,6]` 成功初始化并对 `ok` 生成 `Alright,`，无需重启 Zephyr 即完成第三次 PING `7→8`。随后以单独 RAM-only 候选启用 `mailbox0`，确认 Linux `rockchip-mailbox` controller 绑定；又在主机侧确认 AMP DTS 的 mailbox client wiring 指向通道 0/3，并审计确认 Linux controller IRQ 93–96 的方向是对端→Linux，而非 Linux TX3。最新 RAM-only 只读 MMIO 观察中，Linux TX3 返回 `mailbox_tx_ret=0` 且读到 `A2B status=0/cmd=1/data=41`，Zephyr CPU3 同一时点读到 `marker=0x4d424f58, status=0, cmd=1, data=41`，因此 A2B 寄存器可见性/路径已验证。CPU3 执行 `irq_enable(100)` 后的 GIC 观察为 `GICE 0x47494345/0x8fc00211/0x0/0x300`，排除了 IRQ100 未启用/未路由；Linux TX3 ping 后无 MISR，故不能把 IRQ100 认作该 TX3 事件的对应中断。v2 只读扫描中，标准 SPI INTID32–511 返回 `PN00 0x504e3030/0/0/0`，基线/差分均为零，同时 `a2b_at_tx/a2b_now=0/1/0x29` 表明寄存器写入保留但本扫描窗口未见新增标准 SPI pending。A2B_INTEN 只读观察中，启动为 `A2BI=0x41324249/0/0/0`，Linux TX3 ping 后为 `A2BP=0x41324250/0/0/1`，Linux 同一状态为 `a2b_now=0/1/0x29`，即 `A2B_INTEN=0`、`A2B_STATUS=0`、`CMD3=1`、`DAT3=0x29`；这支持“gate 未打开可能解释无 pending”的假设。随后 A2B gate enable 只读回读为启动 `A2BE=0x41324245/0/0x8/0`、PING 后 `A2BP=0x41324250/0x8/0x8/0x1`，Linux `a2b_at_tx/a2b_now=0x8/0x1/0x29`，证明 bit3 写入可回读并使 A2B_STATUS bit3 置位；gate-open pending scan candidate 上 Linux PING `41` 得到 `42`，`mbox_observation=0x50454e44/0x64/0x0/0x10`，raw GIC INTID100 pending 基线由 `0` 变为 `0x10`（word3 bit4），直接证实 Linux TX3 经 A2B channel 3 gate 产生标准 GIC SPI100 pending。该结果不证明 Zephyr ISR 已触发，也不表示完整 doorbell 已完成；候选 IRQ100 ISR 仍未验证。v2 结果仅表示未观测到标准 SPI pending 差分，不等同于绝对没有中断或否定其他通知路径。第二次观察为零是 Linux ping 按设计先清观察区所致。结论仍不代表压力、并发、吞吐、异常恢复、长期稳定性或性能验证；A2B ISR/doorbell、B2A/Linux RX、RPMsg、外设和 mailbox client channel request 仍未验证，未写启动分区或保存 U-Boot 环境。

补充结论：12 控制器候选在 mailbox1 ch0 request 的 `rockchip_mbox_startup` 触发 synchronous external abort，已停止并回退 mailbox0 四通道。当前内核/Zephyr mailbox0 四通道版本已完成主机构建；协议 groundwork 仅通过主机 C 单元测试，尚未 Linux↔CPU3 集成、上板或 RAM-only 运行验证。

补充结论：新的 `r1-boot-fit.img` RAM-only 基线回归已启动 Linux `5.10.252`（`nproc=7`，Zephyr carveout present），加载 `36,912 B` 的 `zephyr-mbox0-baseline.bin` 后 PSCI `CPU_ON ret=0`，Zephyr `current_el=4` 并持续 heartbeat；doorbell `0 41` 后仅 ch0 得到 `m0c0=rx:1/0xb2a00000/0x29,tx:1/0`、`a2b_now=m0:0`。该结果不称为新协议集成，也不扩展为本轮四通道全测。

补充结论：2026-08-27 RAM-only MailMsg V1 候选完成一次 priority 0 的共享内存 PING/PONG。`mailmsg_ping` 写入 value 41，`mailmsg_response` 返回 `priority=0 valid=1 type=2 sequence=1 value=42`；状态为 `m0c0=rx:1/0xb2a10000/0x0,tx:1/0`、`a2b_now=m0:0x0`。FIT SHA-256 为 `e7c73a7d3628654badbd3a98559307f32325519043049a32d2abd39d19070400`，Zephyr SHA-256 为 `a2b386ebda0493c7aa72f7a7f07156af0872e51ee9874db2cb365a57ec9ebf46`。仅覆盖一次 MailMsg priority 0 与 mailbox0 ch0 通知，不覆盖 priority 1–3、负载/并发、大数据、可靠性或 eMMC。

补充结论：同一 MailMsg V1 RAM-only session 连续发送 priority/value `0/10`、`1/20`、`2/30`、`3/40` 后，依次收到 `p0 seq2 val11`、`p1 seq3 val21`、`p2 seq4 val31`、`p3 seq5 val41`，均 `type=2 valid=1`；m0c0–m0c3 分别收到对应映射，`a2b_now=m0:0`。这仅验证四 priority 独立映射和一次连续四消息 PING/PONG，不覆盖抢占调度、并发/高负载、队列满策略、长期稳定或大数据。

补充结论：V2 前一次候选因旧 DTS sysfs loader size `0x9030` 对应的 `36,912 B` 上限触发 `cat: File too large`，固件被截断，结果无效。修正为 `0xa030` 后，Zephyr image 完整写入 `41008/41008`，`CPU_ON ret=0`；priority 0 value 41 返回 `type=2 seq=1 value=42`，`m0c0 rx 0xb2a10000/0x0, tx 1/0`，pending 为 0。该结果仅证明板端 CRC V2 正常路径一次运行；payload 篡改拒收仍仅由 host unit test 证明，不扩展为板端拒收或故障恢复。

补充结论：2026-08-28 RAM-only V2 CRC fault candidate 中，Linux test-only injection 在 CRC 计算后、producer 发布前翻转 payload；正常 priority 0 value `41→42`，注入后 Zephyr 观察 `CRCE/0/-4/1`、无 PONG且 CPU3 heartbeat 持续。随后 priority 0 value `200` 仍无响应，priority 1 value `300→301` 正常返回。由此仅确认板端检测/拒收、坏帧对同 priority 队头的阻塞现象以及 priority ring 隔离；原地恢复、丢帧/跳过、重试、重置等策略未定义且未验证，不做技术根因扩展。

## 关联知识与问题

补充结论：one-shot IRQ candidate 已实测 Zephyr IRQ100 ISR 触发并读取 A2B status bit3、`CMD3=1`、`DAT3=41`；ISR 自行 mask 且未 ack mailbox。因此 Linux→Zephyr doorbell（mailbox0 A2B ch3 gate→GIC100）已验证，但 B2A/full duplex 仍未验证。

补充结论：B2A one-shot 已实测 `mailbox_rx_count` 从 `0` 到 `1`，证明 Zephyr B2A ch0 write 到达 Linux amp-rx callback；与 A2B 共同形成双向 doorbell 原型。未记录 IRQ93 前后计数，实际寄存器清零未直接读取，尚不等同于完整双向消息/ack 或 RPMsg 验证。

补充结论：连续三轮中 request/response 序号、B2A ACK 序号和 Linux callback 计数一一对应；该结果仍限于单槽、手动 sleep/response polling。`mbox_send_message` 非负返回值是成功提交 cookie，负数才是失败。

补充结论：2026-08-26 的 RAM-only matrix candidate 在不修改 eMMC 的前提下完成 mailbox0 ch0–ch3 各一次 Linux→CPU3→Linux 闭环，验证候选 SPI97–100 路由至 CPU3 与四通道独立 A2B/B2A 闭环。A2B_STATUS 远端确认/清除语义仍未验证；Zephyr 本轮不写 A2B_STATUS、每次 IRQ 后 one-shot 屏蔽，最终 `a2b_now=0xf` 不表示持久化。

补充结论：同日 A2B clear probe 在当前 RAM candidate 的 mailbox0 ch3 上观察到 `ACKC/0x8/0x0/0x8`，按探针定义为写前 A2B_STATUS `0x8`、写后回读 `0x0`、写入掩码 `0x8`；直接支持 CPU3 写 A2B_STATUS `BIT(3)` 清除对应 pending 位。该证据不覆盖连续重复门铃、全通道或其他 controller。

补充结论：修正客户端 `knows_txdone=false` 后，当前 RAM candidate 的 mailbox0 ch3 已完成连续两次 Linux→CPU3→Linux；第二轮 `rx=2`、`tx=2/1`、数据为 `0xb2a00003/0x2a`，两轮最终 `a2b_now=0x0`。`tx=1` 是非负 mailbox 队列 cookie，表示成功提交；该结果不扩展为四通道重复、并发或 mailbox1/2 行为。

补充结论：同一可重复 RAM candidate 上 ch0/ch1/ch2 均完成发送、CPU3 回执并清 pending；`ACKC/0x4/0x0/0x4` 按定义直接记录 ch2 的写前 status、写后回读和掩码。结合此前 ch3 的 `ACKC/0x8/0x0/0x8`，至少 ch2/ch3 的清除已有直接观测，四通道独立闭环可复用；不宣称并发、高吞吐或 mailbox1/2 已验证。

补充结论：2026-08-28 RAM-only MailMsg V3 reliability candidate 已完成板端回归。FIT `build/local/r1-mailmsg-v3-reliability/r1-mailmsg-v3-reliability.img` SHA-256 为 `82cb3a1f1169602321ffc51978e5633d9fa72471b7210bf74211a4a41d346aed`，Zephyr `mailmsg-v3-reliability.bin` 为 `41008 B`、SHA-256 `f217a3510f82fbf2a0ff7131fe66d8e93f6d2809ffc9d10ebf02e5a401d10e2b`；板端 `image=41008/41008`、`affinity=off(1)`、`cpu_on_ret=0`、`current_el=4`。priority 0 正常 `41` 返回 ACK `type=3 sequence=1 peer_sequence=1 status=0` 后 PONG `type=2 sequence=2 value=42`；CRC 注入原序列 2 返回 NACK `type=4 sequence=3 peer_sequence=2 status=1` 且无 PONG，随后正常 `200` 返回 ACK `type=3 sequence=4 peer_sequence=3 status=0` 与 PONG `type=2 sequence=5 value=201`，证明坏帧释放 slot 且不阻塞 p0。priority 1 正常 `300` 返回 ACK `type=3 sequence=8 peer_sequence=7 status=0` 后 PONG `type=2 sequence=9 value=301`；CRC 注入原序列 8 返回 NACK `type=4 sequence=10 peer_sequence=8 status=1` 且无 PONG。priority 2 CRC 注入返回 `valid=0 reason=empty` 且无反馈，随后正常 `200` 仅返回 PONG `priority=2 type=2 sequence=7 value=201`，证明 best-effort 错误帧立即丢弃并可继续处理。该证据验证 p0/p1 可靠 ACK/NACK、p0 坏帧释放 slot 及 p2 无反馈代表路径；ACK 是传输校验/接收确认，不是应用完成确认。priority 3、并发/压力、长期稳定、eMMC 及自动重传仍未验证；自动重传未实现，由发送端后续策略决定。队列满 `FULL` 的立即、非阻塞、不覆盖行为是设计边界，未在本步骤验证；重试、丢弃、降级或报警由上层调用者决定。

- 支持：`EXP-20260821-003` 已验证的 Linux→Zephyr 单向共享页心跳和 cache 可见性。
- 已验证：当前 RAM candidate 的八次顺序 Linux→Zephyr→Linux PING/response 与显式 cache-maintained 可见性；同一实例中一次 RKLLM 生成后仍完成 PING 回环。
- 已验证（静态）：AMP DTS 的 `mbox-names`/`mboxes` 已编译进 DTB，RX/TX 分别指向 `mailbox0` 通道 0/3；未验证 client 驱动申请或消息传输。
- 已验证：Linux `rockchip-mailbox` 的 IRQ 是 B2A 接收方向，不能用 Linux 的 93–96 IRQ 观察来判断 A2B TX3 是否到达 Zephyr。
- 已验证：RAM-only 只读 MMIO 观察确认 Linux TX3 写入的 A2B `status/cmd/data` 在同一时点对 Zephyr CPU3 可见；该结果不扩展为 ISR、中断路由、B2A/Linux RX 或 RPMsg 证据。
- 已验证（候选 IRQ 排除）：CPU3 `irq_enable(100)` 后 GIC 观察显示 SPI100 已启用且路由至 CPU3；Linux TX3 ping 后无 MISR，因此排除“IRQ100 未启用/未路由”，但不指定任何替代 INTID。第二次观察归零由 Linux ping 按设计清观察区造成。
- 已验证（v2 扫描边界）：Linux TX3 PING 前后，标准 SPI INTID32–511 的 GICD_ISPENDR 扫描均为 `PN00 0x504e3030/0/0/0`，同时 `a2b_at_tx/a2b_now=0/1/0x29`。结论仅为未观测到标准 SPI pending 差分，不等同于绝对没有中断或否定其他通知路径。
- 已验证（A2B_INTEN 只读边界）：启动 `A2BI=0x41324249/0/0/0`，Linux TX3 ping 后 `A2BP=0x41324250/0/0/1`，Linux 同状态为 `a2b_now=0/1/0x29`；即 `A2B_INTEN=0`、`A2B_STATUS=0`、`CMD3=1`、`DAT3=0x29`。支持 gate 未打开可能解释无 pending；bit3 语义及 A2B ISR 仍未验证。
- 已验证（A2B gate enable 回读）：启动 `A2BE=0x41324245/0/0x8/0`，PING 后 `A2BP=0x41324250/0x8/0x8/0x1`，Linux `a2b_at_tx/a2b_now=0x8/0x1/0x29`；bit3 写入可回读并使 A2B_STATUS bit3 置位。候选 IRQ100 ISR 仍未命中，不能据此断言具体替代线路。
- 已验证（gate-open pending）：Linux PING `41→42` 时，`a2b_at_tx/a2b_now=0x8/0x1/0x29`，`mbox_observation=0x50454e44/0x64/0x0/0x10`，raw GIC INTID100 pending 基线由 `0` 变为 `0x10`（word3 bit4）。这直接证实 A2B channel 3 gate 产生标准 SPI100 pending；不证明 Zephyr ISR 或完整 doorbell 已完成。
- 已验证（B2A one-shot）：启动前 `image=0, mailbox_rx_count=0`；Zephyr start + Linux PING `41` 后 `seq=1 valid=1 result=42`，`mailbox_rx_count=1 mailbox_tx_count=1 mailbox_tx_ret=0`，证明 Zephyr B2A ch0 write 到达 Linux amp-rx callback。与 A2B 方向共同形成双向 doorbell 原型；未记录 IRQ93 前后计数，且实际寄存器清零未直接读取，Linux ack 语义仅来自源码。
- 待验证候选：官方 AMP DTS 将 raw INTID 100 路由到 CPU3；其启用和路由已观察，但未证实该中断就是 TX3，也不推断任何替代 INTID。
- 待验证：压力、并发、吞吐、异常恢复、长期稳定性、性能、RPMsg 以及外设行为。

## 后续行动

补充记录（2026-08-28，通知合并候选）：主机三项通知层单元测试与完整 ARM64 Image 编译通过。已生成并传板 RAM-only 候选 `r1-mailmsg-v3-notify-coalesce.img`（SHA-256 `f8709830d4411b620f02a1a2d29ec3f08d9f709c61cb22b18e27d93067fd3083`）及 Zephyr 固件（SHA-256 `c6fdf6a1f55b8e61dfb231c8a1e13e7cefe46789a7c06c090900a54f1a11e746`）；尚未上板运行。设计/实现语义为：通知层区分 `SENT`、`COALESCED`、`FAILED`；发送前若硬件 A2B_STATUS 对应 priority pending 位已置，则不调用 `mbox_send_message`、不占 Linux core TX 队列，直接标记 `COALESCED`，状态 `tx_ret=-EBUSY` 记录观察原因；其他后端失败保留负 errno。sysfs 入队成功但通知失败仍返回入队成功，通知不自动重传。`MAILMSG_TEST_HOLD_A2B_USEC=500000` 仅用于复现 pending，默认不开启。硬件 `COALESCED` 尚未验证。

补充结论（2026-08-28，RAM-only 硬件验证）：在最终 FIT SHA-256 `f8709830d4411b620f02a1a2d29ec3f08d9f709c61cb22b18e27d93067fd3083` 上 CPU3 已启动。板端状态为 `m0c0=rx:1/0xb2a10000/0x0,tx:1/-16,notify:1/1/1/0`、`a2b_now=m0:0`、`mailmsg_notify=1`；最后通知结果为 `COALESCED`，发送计数 1、合并计数 1、失败计数 0，`-16` 是内核 `EBUSY` 作为观察到的门铃 pending 原因，不是消息入队失败。p0 两次连续请求分别返回 ACK `seq1 peer1 status0`、PONG `seq2 value101`，以及 ACK `seq3 peer2 status0`、PONG `seq4 value201`。该结果限于 mailbox0 ch0：第二条消息合并到既有 A2B pending 门铃且未占 Linux core TX 队列；不等价于高并发或压力测试。

- [ ] 在不改变持久化启动介质的前提下，验证通知三态在并发、压力及其他 mailbox 通道下的边界；保持入队结果与通知结果分离，暂不自动重传、接入 RPMsg 或写入 eMMC。
