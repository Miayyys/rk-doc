---
title: "EXP-20260820-002 检查 R1 实际 BL31 的 Rockchip SiP 入口"
type: experiment
status: active
created: 2026-08-20
updated: 2026-08-21
tags: [rk3588, amp, bl31, trusted-firmware, sip]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[status/current]]"
---

# EXP-20260820-002 检查 R1 实际 BL31 的 Rockchip SiP 入口

## 目标

确认当前 R1 启动链实际使用的 BL31 是否含 Rockchip SiP 服务入口，并继续以只读方式判断它能否支持 Linux AMP 驱动将使用的 `RK_SIP_AMP_CFG (0x82000022)`。

本实验尚不调用任何 SMC。出现通用 SiP 入口不能单独证明 AMP 子功能已实现。

## 环境与前置条件

- 执行端：Arch Linux 主机（fish）。
- 输入：此前从 R1 eMMC `p1` 只读导出的 `build/local/r1-20260816/r1-uboot-p1.img`，SHA-256 `3b09148574d57f9b76f8afb064dc21af6df2819e0c5ccf4bce18e08f56820001`。
- `p1` 是 FIT，image 1 为 ARM Trusted Firmware / BL31；没有读取或改变 R1 板端状态。

## 风险与恢复

- 影响范围：只在主机 `build/local/r1-20260816/` 生成可重新提取的 BL31 文件。
- 板端影响：无；没有连接板端、加载 DTB、调用 SMC 或写 eMMC。
- 恢复方法：删除本机提取文件即可；原始 p1 备份保持不变。

## 步骤与证据

### 步骤 1：从 p1 FIT 提取实际 BL31

目的与预期结果：不以源码或猜测替代运行启动链，先确定 p1 中真实的 ATF 载荷身份。

```fish
set p1 ~/Study/rk3588/build/local/r1-20260816/r1-uboot-p1.img
set atf ~/Study/rk3588/build/local/r1-20260816/r1-atf-1.bin

dumpimage -T flat_dt -p 1 -o $atf $p1
file $atf
sha256sum $atf
strings -a $atf | rg -i -m 30 'bl31|version|amp|sip|rk3588'
```

实际输出（学习者提供）：

```text
Image 1 (atf-1)
  Description:  ARM Trusted Firmware
  Data Size:    194076 Bytes = 189.53 KiB = 0.19 MiB
  Architecture: AArch64
  OS:           ARM Trusted Firmware
  Load Address: 0x00040000
  Hash value:   045b2cef2942b527ef832a6126233d4ac2ded6ba6c26d40bff6a03c96a1eab1d

r1-atf-1.bin: data
045b2cef2942b527ef832a6126233d4ac2ded6ba6c26d40bff6a03c96a1eab1d  r1-atf-1.bin

BL31: Preparing for EL3 exit to %s world
BL31: Initialising Exception Handling Framework
BL31: Initializing runtime services
rockchip_sip_svc
rk_sip_hdcp_config_handler
%s: unhandled sip (0x%llx)
plat_rockchip_bl31_entrypoint
rockchip_plat_sip_handler
```

观察：提取文件的 SHA-256 与 p1 FIT image 1 声明值一致，因此它是当前 R1 启动链使用的实际 ATF 载荷。字符串证明 BL31 和 Rockchip SiP 分发路径存在；`unhandled sip` 只说明该固件会拒绝未知 function ID，不能说明 `0x82000022` 是否在已处理集合中。筛选输出未包含可定位的 `AMP` 子功能或版本标识。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 实际 BL31 提取 | 从 p1 FIT image 1 获得 ATF 载荷 | 194,076 B，FIT hash 与文件 hash 一致 | 通过 |
| BL31/SiP 基础设施 | 存在 BL31 和 Rockchip SiP 分发线索 | 存在 `rockchip_sip_svc`、`rockchip_plat_sip_handler` 等字符串 | 通过 |
| `RK_SIP_AMP_CFG` 支持 | 能定位 `0x82000022` 的处理证据 | 当前字符串检查未获得 | 待验证 |

### 步骤 2：检查本机是否已有可比对的 TF-A/BL31 源码

目的与预期结果：优先使用已下载资料定位实际固件的源码或 SiP 分发表，避免把 Linux 内核的 SMC 调用封装误认为 BL31 实现。

```fish
rg -l -i 'rockchip_plat_sip_handler|rockchip_sip_svc' \
  ~/Study/rk3588/src ~/Study/rk3588/R1 \
  --glob '*.{c,h,S}'

rg --files ~/Study/rk3588/src ~/Study/rk3588/R1 | \
  rg -i '/(arm-trusted-firmware|trusted-firmware-a|atf|bl31|plat/rockchip)/' | \
  head -n 40
```

实际输出（学习者提供）：两条命令均无输出。

观察：在当前本机 `src/` 与 `R1/` 范围内，未找到含这两个 BL31 handler 符号的源码，也未找到按 TF-A/BL31/`plat/rockchip` 命名可识别的源码路径。这不能证明 R1 固件不支持 AMP；它只排除了“本机已具备可直接核验的匹配固件源码”这一途径。

### 步骤 3：与本机 rkbin 的 RK3588 BL31 候选核对

目的与预期结果：判断 `rkbin` 提供的同 SoC BL31 能否作为**实际 R1 BL31 的精确身份来源**。只有 SHA-256 完全相同才可作此关联；同为 RK3588 的 ELF 格式不是充分条件。

```fish
set atf ~/Study/rk3588/build/local/r1-20260816/r1-atf-1.bin
set candidate ~/Study/rk3588/src/rkbin/bin/rk35/rk3588_bl31_v1.54.elf

file $candidate
sha256sum $atf $candidate
```

实际输出（学习者提供）：

```text
/home/loser/Study/rk3588/src/rkbin/bin/rk35/rk3588_bl31_v1.54.elf: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV), statically linked, stripped
045b2cef2942b527ef832a6126233d4ac2ded6ba6c26d40bff6a03c96a1eab1d  /home/loser/Study/rk3588/build/local/r1-20260816/r1-atf-1.bin
4d111f0c793035dba84b65cfa741665814ae134af8243c3bb8a87d55a0937bc3  /home/loser/Study/rk3588/src/rkbin/bin/rk35/rk3588_bl31_v1.54.elf
```

观察：两个 SHA-256 不同，因此 `rk3588_bl31_v1.54.elf` 不是当前 R1 p1 FIT 中实际使用的**同一字节载荷**。是否只是封装不同，需要继续读取 ELF 的 `PT_LOAD` 段。

### 步骤 4：比较 ELF `PT_LOAD` 段与 p1 ATF 载荷布局

目的与预期结果：区分“同一代码的 ELF/raw 封装差异”和“实际加载布局也不同”。前者至少应能在各载入地址与文件大小上建立一致对应；本步骤仍不解包、反汇编或操作板端。

```fish
readelf -lW ~/Study/rk3588/src/rkbin/bin/rk35/rk3588_bl31_v1.54.elf
```

实际输出（学习者提供，省略 `GNU_STACK` 与节映射无关行）：

```text
Elf 文件类型为 EXEC (可执行文件)
Entry point 0x60000

  Type  Offset    VirtAddr             PhysAddr             FileSiz  MemSiz   Flg Align
  LOAD  0x010000  0x0000000000060000  0x0000000000060000  0x02461c 0x071000 RWE 0x10000
  LOAD  0x040000  0x00000000000f0000  0x00000000000f0000  0x006000 0x006000 R   0x10000
  LOAD  0x050000  0x00000000ff100000  0x00000000ff100000  0x009000 0x009000 RWE 0x10000
```

与步骤 1 中 p1 FIT 的 ATF image 元数据对照：

| 项目 | 实际 R1 p1 FIT | rkbin v1.54 ELF `PT_LOAD` | 判定 |
| --- | --- | --- | --- |
| 主 ATF 载荷 | load `0x40000`，194,076 B（`0x2f61c`） | phys `0x60000`，149,020 B（`0x2461c`） | 地址、大小均不一致 |
| 次级 ATF 载荷 | load `0xf0000`，24,576 B（`0x6000`） | phys `0xf0000`，24,576 B（`0x6000`） | 仅元数据相同；内容尚未逐段哈希 |
| PMU SRAM ATF 载荷 | load `0xff100000`，20,480 B（`0x5000`） | phys `0xff100000`，36,864 B（`0x9000`） | 地址相同、大小不一致 |

观察：主载荷和 PMU SRAM 载荷均无法建立一一对应，故 v1.54 ELF 不是实际 R1 p1 ATF 的简单 raw/ELF 重封装。次级 `0xf0000` 段虽同地址同大小，但不足以改变整体结论，且尚未逐段比较内容。该结果仍不能说明实际 R1 BL31 是否支持 AMP；它只说明不能以此 v1.54 ELF 作为实际固件或源码替代物。

### 步骤 5：定位 rkbin v1.54 候选的版本更新时间

目的与预期结果：用本地 Git 元数据判断该参考文件的版本时间是否至少与当前 R1 固件处于同一发布时期；这不是源码证明，只是排除明显的时间线矛盾。

```fish
git -C ~/Study/rk3588/src/rkbin log --format='%H %aI %s' -- \
  bin/rk35/rk3588_bl31_v1.54.elf | head -n 5
```

实际输出：

```text
4864ee05356b46444518e62f7214cadbc5e7ee03 2025-12-26T17:21:11+08:00 rk3588: bl31: update version to v1.54
```

观察：当前 R1 p1 FIT 的创建时间为 2024-09-29，而本地 `rkbin` 在 2025-12-26 才将该文件更新为 v1.54。因此除了二进制布局不匹配外，版本时间线也不支持把 v1.54 当作当前 R1 固件来源。Git 提交只记录 rkbin 二进制更新，未提供 R1 实际 BL31 的源码或 AMP SiP 支持证据。

### 步骤 6：核对内核 AMP 驱动的 DTS 契约与实际 R1 条件

目的与预期结果：确定加入 `rockchip,amp` / `amp-cpus` 节点是否只是 Linux 设备树改动，还是会立刻触发未验证的固件调用。

主机只读核对结果：候选内核的 `.config` 与 `rockchip_linux_defconfig` 均有 `CONFIG_ROCKCHIP_AMP=y`，Rockchip SoC Makefile 将其编入 `rockchip_amp.o`。该驱动从 `rockchip,amp` 节点的 `amp-cpus` 子节点读取四个必需属性：`id`、`entry`、`mode`、`boot-on`。

唯一找到的实际 `amp-cpus` DTS 参考是 `rk3568-amp.dtsi`：

```dts
amp-cpus {
        amp-cpu3 {
                id = <0x0 0x300>;
                entry = <0x0 0x2800000>;
                boot-on = <1>;
                mode = <0>;
        };
};
```

在本地 RK3588 DTS 范围中，`rk3588-amp.dtsi` 提供了 `rockchip,amp`、mailbox、RPMsg 和内存区域，但没有 `amp-cpus` 参考。故不能把 RK3568 的 CPU ID、entry 或 mode 直接套用于 R1/RK3588。

### 步骤 7：确认添加 CPU 描述会在 probe 阶段调用 BL31

目的与预期结果：确认 `boot-on = <0>` 是否能把候选限制为纯 DTS/驱动验证而不触及固件。

`rockchip_amp_boot_cpus()` 的实际顺序是：读取 `id`、`entry`、`mode`、`boot-on`，随后无条件调用：

```c
sip_smc_amp_config(RK_AMP_SUB_FUNC_CFG_MODE, cpu_id, cpu_mode, 0);
```

只有后续的 `RK_AMP_SUB_FUNC_CPU_ON` 才受 `boot-on` 控制。该封装实际发出的 function ID 为 `RK_SIP_AMP_CFG = 0x82000022`。因此，即使 `boot-on = <0>`，添加一个可被解析的 `amp-cpus` 节点也会向实际 BL31 发送未验证的 AMP 配置 SMC。

结论：当前静态 R1 AMP DTS 没有 `rockchip,amp`、`amp-cpus` 或相关启动属性，故它未因内核 probe 调用 AMP SMC。下一步不能是“先加 DTS 试试”；当前安全阻塞是取得与 R1 2024-09 实际 BL31 相匹配的厂商 TF-A/SDK 源码或明确支持资料，并离线定位 `RK_SIP_AMP_CFG (0x82000022)` 的 handler。该阻塞不影响已经完成的 Linux 静态隔离和 NPU LLM 回归。

### 步骤 8：核对公开官方 TF-A 的 RK3588 SiP handler

目的与预期结果：验证 `0x82000022` 是否为公开通用 TF-A 的 RK3588 接口，避免把 Linux BSP 头文件中的定义误当作每个 RK3588 BL31 都实现的 ABI。

主机在 `build/local/arm-trusted-firmware/` 浅克隆 `ARM-software/arm-trusted-firmware`，检出 commit `6a164dda7208d3f2c0a5c4a1681db5ec37532bf5`（2026-08-20，72 MiB）。对全树精确搜索 `RK_SIP_AMP_CFG`、`0x82000022` 与 `AMP_CFG`，没有匹配。RK3588 的实际公开 handler 为：

```c
switch (smc_fid) {
case RK_SIP_SCMI_AGENT0:
        scmi_smt_fastcall_smc_entry(0);
        SMC_RET1(handle, 0);
default:
        ERROR("%s: unhandled SMC (0x%x)\\n", __func__, smc_fid);
        SMC_RET1(handle, SMC_UNK);
}
```

观察：公开上游 TF-A 支持 RK3588 作为平台并含 Rockchip SiP 框架，但当前公开 RK3588 handler 不实现 AMP SMC `0x82000022`。这**不证明** R1 实际 BL31 不支持它：R1 的 2024-09 二进制与公开源码 commit 并不相同；但它证明不能把“官方 TF-A”当成该扩展的实现证据。

结论：Linux `rockchip_amp` 所依赖的是 Rockchip BSP/厂商固件扩展，而不是已在当前公开 TF-A RK3588 handler 中可验证的通用接口。后续只应查找与 R1 固件同期的 Rockchip BSP/厂家 SDK 中的 TF-A/Trust 源码或二进制版本说明；不以公开 TF-A 构建物替换 R1 BL31，不调用未知 SMC。

### 步骤 9：核对 Rockchip 官方 RK3588 BL31 的 AMP 发布证据与历史载荷

目的与预期结果：区分“公开通用 TF-A 未实现该接口”和“Rockchip 官方 BSP BL31 从未支持 AMP”这两个不同结论，并只读判断 R1 实际载荷是否恰好对应已知官方版本。

Rockchip 官方 `rkbin` 的 [RK3588 发布记录](https://github.com/rockchip-linux/rkbin/blob/master/doc/release/RK3588_EN.md)列出 `rk3588_bl31_v1.31.elf`（2022-11-09），说明为 **“Support amp function.”**。这是一条关于官方 RK3588 BL31 能力的直接资料证据；它与公开 `ARM-software/arm-trusted-firmware` 当前 RK3588 handler 的结果不矛盾，表明 AMP 是 Rockchip BSP 的扩展能力。

为避免只按发布日期推定 R1 使用的版本，主机按需从 `rkbin` 历史取得官方 v1.30、v1.31 和 v1.47 ELF，读取其 `PT_LOAD` 布局，并从 R1 p1 FIT 提取全部三个 ATF image。实际得到：

| 项目 | 主 ATF（`0x40000`） | 次级 ATF（`0xf0000`） | PMU SRAM ATF（`0xff100000`） |
| --- | ---: | ---: | ---: |
| R1 实际 p1 FIT | 194,076 B（`0x2f61c`） | 24,576 B（`0x6000`） | 20,480 B（`0x5000`） |
| 官方 v1.30 | 192,356 B（`0x2e764`） | 24,576 B（`0x6000`） | 20,480 B（`0x5000`） |
| 官方 v1.31 | 194,404 B（`0x2f764`） | 24,576 B（`0x6000`） | 20,480 B（`0x5000`） |
| 官方 v1.33 | 194,076 B（`0x2f61c`） | 24,576 B（`0x6000`） | 20,480 B（`0x5000`） |
| 官方 v1.47 | 204,860 B（`0x3203c`） | 24,576 B（`0x6000`） | 36,864 B（`0x9000`） |

实际 R1 三段 SHA-256 分别为 `045b2cef…1eab1d`、`30812190…dbb479`、`cb7bdbec…69d78a`。官方 v1.33 ELF 从偏移 `0x10000`、`0x40000`、`0x50000` 提取对应三个 `PT_LOAD` 文件范围后，三个 SHA-256 与 R1 三段**逐项完全相同**；v1.30、v1.31、v1.34 和 v1.47 都不能建立这一身份关系。故当前 R1 实际 BL31 可确认是官方 `rk3588_bl31_v1.33.elf` 的载荷，而不是仅凭 2024-09 p1 FIT 封装时间推测为 v1.47。

观察：v1.33 位于 v1.31 明记“Support amp function”之后，且官方 v1.32、v1.33 发布记录没有撤销该能力的条目。因此“当前 R1 使用 Rockchip 官方 AMP BL31 版本线”已获强证据支持。但当前仍缺少 v1.33 的公开 handler 源码与 RK3588 A55 的 `id`/`mode`/`entry` 契约，不能据此猜测 SMC 参数；禁止试探性调用 SMC 的结论保持不变。

### 步骤 10：核对官方 U-Boot 的 AMP 启动责任与同期 SiP ABI

目的与预期结果：不把 Linux `rockchip_amp` 的 `amp-cpus` probe 路径误认为唯一启动方式；确认与 R1 实际 BL31 v1.33 同期的 Rockchip 官方 U-Boot 如何传递 AMP 参数，以及当前 R1 U-Boot 是否已有该路径的直接证据。

主机只读查询 Rockchip 官方 U-Boot 的 `drivers/cpu/rockchip_amp.c` 历史。该文件在 2022-07-05 的提交 `5f1605846e65` 已存在；该提交源码 SHA-256 为 `614bdbfdec41b8eec9b85ddc7e2cb14a6e157b18dac6f8bcfb0a6ce3745a810b`。该时间早于 R1 实际 BL31 v1.33 所在的 2022-12 版本线。

2022 源码的实际启动约定是：

1. `amp_cpus_on()` 按分区名 `amp` 读取一个独立的 AMP FIT；不是从 Linux boot FIT 或 Linux DTS 直接取远端固件。
2. AMP FIT 的 `conf` 可含一个 `linux` 节点和若干 `loadables`；每个非 Linux CPU 固件节点读取 `cpu`（MPIDR）、`arch`、`load`、`hyp`、`thumb` 与可选 `boot-on`。
3. 其代码先按 `PE_STATE(aarch64, hyp, thumb, 0)` 形成处理器状态；不是默认状态时，以 `SIP_AMP_CFG` 子号 `AMP_PE_STATE` 配置该状态，随后以标准 `psci_cpu_on(cpu, entry)` 启动目标 CPU。只有“让 Linux 跑在非启动 CPU”这一特殊路径才另用子号 `AMP_BOOT_ARG01` 与 `AMP_BOOT_ARG23` 传递四个 Linux 启动参数。

官方 2022 头文件和当前官方 `next-dev` 头文件对 AMP ABI 的值一致：

```c
#define SIP_AMP_CFG    0x82000022
#define AMP_PE_STATE   0x0
#define AMP_BOOT_ARG01 0x1
#define AMP_BOOT_ARG23 0x2
```

两版头文件的差异仅涉及其余 SiP 服务和声明，没有改变上述四个定义；因此可确认这组 U-Boot 侧 SiP 调用编码至少从 2022-07 延续到当前参考树。这里的结论边界是“调用方 ABI 稳定”，不是对 R1 BL31 handler 参数语义的独立反汇编证明。

为核对 R1 当前启动程序，主机仅从已备份的 p1 FIT 提取 image 0：

```fish
set p1 ~/Study/rk3588/build/local/r1-20260816/r1-uboot-p1.img
set uboot ~/Study/rk3588/build/local/r1-20260816/r1-uboot.bin

dumpimage -T flat_dt -p 0 -o $uboot $p1
stat -c 'size=%s bytes' $uboot
sha256sum $uboot
strings -a $uboot | rg -i -m 40 \
  'rockchip amp|amp error|brought up.*cpu|amp\.img|amp-cpus|amp partition|smc pe-state|0x82000022'
```

实际输出（学习者主机）：

```text
size=1297800 bytes
eb906a97009ed1bfdc220828d55d08efbd9d9eee2fbbda97224d5d6ca9e5a6a1  r1-uboot.bin

== AMP-related strings (presence only) ==

== generic U-Boot identity ==
U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1
U-Boot 2017.09-g33a7c066a8-dirty #youyeetoo1 (Sep 29 2024 - 11:10:07 +0800)
```

观察：R1 image 0 的大小和 SHA-256 与 p1 FIT 对该 image 的声明一致。其字符串中没有官方 AMP 驱动的特征诊断文本；而 R1 已确认的 GPT 分区只有 `uboot`、`misc`、`boot`、`recovery`、`backup`、`rootfs`、`oem` 与 `userdata`，没有名为 `amp` 的分区。这两项证据与“当前 R1 U-Boot 没有走官方 AMP FIT 启动流程”一致，但二进制 strings 检查不能单独证明 `CONFIG_AMP` 一定未编入，也不能证明 BL31 不支持 AMP。

官方当前 `drivers/cpu/amp.its` 模板把 `cpu = <0x300>` 作为第三个 Cortex-A 目标的 MPIDR 示例；R1 静态 AMP DTB 中被移交的 A55 core3 正是 `cpu@300`。这是 CPU 标识可对齐的资料线索。该模板的处理器状态由 `arch`/`hyp`/`thumb` 推导；现有 Zephyr hello 已确认是非安全 AArch64（`CONFIG_ARMV8_A_NS=y`），但尚未验证其作为次级 CPU 固件应从 EL1 还是 EL2 进入。因此，不能据此填写 AMP FIT 的 `hyp` 或 Linux driver 的 `mode`，更不能据此执行 SMC。

### 步骤 11：确认现有 Zephyr AArch64 镜像的入口异常级别处理

目的与预期结果：确定 Zephyr 是否要求 BL31/启动器把次级 CPU 强制送入某一个异常级别，从而判断官方 AMP FIT 的 `hyp` 是否有一个不触发 Rockchip 专有 SiP 配置的候选值。

对已构建 hello 的 ELF 和 Zephyr v4.4.0 `arch/arm64/core/reset.S` 做主机只读检查，得到：

```text
000000005000100c <__reset>:
    mrs x0, currentel
    ...                         // 分支到 EL3 / EL2 / EL1

EL2 路径：
    bl  z_arm64_el2_init
    msr spsr_el2, ... EL1T ...
    msr elr_el2, ... EL1 path ...
    eret

EL1 路径：
    bl  z_arm64_el1_init
```

观察：该已验证镜像不是“只能从 EL1”或“只能从 EL2”进入。若启动器交给 EL2，Zephyr 先初始化 EL2，再显式 `eret` 到 EL1；若已在 EL1，则直接进入 EL1 初始化。它同样含 EL3 分支，但当前 R1 正常世界次级 CPU 不应据此假定会从 EL3 进入。

结合官方 U-Boot 的宏：对 AArch64、`hyp=1`、非安全、非 Thumb 的值，`PE_STATE(1, 1, 0, 0)` 为 ARM64 U-Boot 的默认处理器状态；`is_default_pe_state()` 对该值直接跳过 `AMP_PE_STATE` SiP 调用，然后只执行 `psci_cpu_on(cpu, entry)`。因此存在一个有源码支撑的**待验证原型路径**：让 A55 core3 以 AArch64 EL2 入口执行 Zephyr，再由 Zephyr 降至 EL1，避免为首次 CPU 启动试探 `0x82000022` 子号 0。

边界：这不是板端并行运行成功证据。现有 R1 U-Boot 没有现成 AMP FIT 装载路径，且尚未验证 core3 当前电源状态、PSCI 返回值、共享 UART/GIC 对 Linux 的影响或缓存同步。故当前不生成可执行的 `amp-cpus` 节点，不直接调用 SMC；后续启动器必须先提供可观察的 PSCI 返回值和不写 eMMC 的恢复路径。

## 结论

当前 R1 的 p1 启动载荷确实包含一个带 Rockchip SiP 服务分发器的 BL31，并已通过三个 `PT_LOAD` 段的逐项 SHA-256 精确确认其为 Rockchip 官方 `rk3588_bl31_v1.33.elf`。该版本位于官方 v1.31“Support amp function”之后，故 R1 使用的 BL31 属于官方 AMP 版本线；`rkbin` 中的 `rk3588_bl31_v1.54.elf` 与实际 R1 载荷不同，不能作为替换件或源码替代物。

官方 U-Boot 2022-07 AMP 调用方已定义并使用与当前一致的 `0x82000022` / `0,1,2` SiP ABI：以独立 `amp` 分区的 AMP FIT 提供 CPU MPIDR、镜像地址和处理器状态，再以 PSCI 拉起 CPU。当前 R1 的提取 U-Boot 没有 AMP 特征字符串且 GPT 无 `amp` 分区，故不能把这条现成 U-Boot 工作流视为可直接在 R1 上使用。该观察不否定 R1 BL31 的 AMP 能力，也不证明当前 U-Boot 的编译配置。

内核端 AMP 代码已编入候选，但当前 R1 静态 AMP DTS 没有 `rockchip,amp`/`amp-cpus` 节点。添加该节点即使 `boot-on=0` 也会触发 `CFG_MODE` SMC，不能作为无风险探针；RK3568 的唯一可见 CPU 节点参考不能移植为 RK3588/R1 参数。

公开官方 TF-A 的当前 RK3588 handler 仅处理 `RK_SIP_SCMI_AGENT0`，没有 `RK_SIP_AMP_CFG` / `0x82000022`。因此该 AMP ABI 应按 BSP/厂商扩展处理，不能用上游 TF-A 或新版 rkbin 代替实际 R1 BL31。

## 后续行动

- [ ] 在主机设计并静态审计一个最小的临时 PSCI 启动器：只为 MPIDR `0x300` 交接已验证 Zephyr 入口 `0x5000100c`，记录返回码，并在成功后返回 U-Boot 继续 RAM 启动 Linux；它不得使用 `RK_SIP_AMP_CFG`、不得写 eMMC，且先避免 Zephyr 对 GIC/UART 的全局初始化。此设计完成和回读前不执行板端启动。
