---
title: "EXP-20260820-001 静态验证 R1 AMP DTS 的 CPU 与内存划分"
type: experiment
status: active
created: 2026-08-20
updated: 2026-08-22
tags: [rk3588, amp, device-tree, zephyr, kernel]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites]]"
  - "[[experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot]]"
  - "[[issue/issue-20260820-001-resource-dtb-overrides-fit-dtb]]"
  - "[[status/current]]"
---

# EXP-20260820-001 静态验证 R1 AMP DTS 的 CPU 与内存划分

## 目标

验证基于已完成 R1 DTS 移植、RKNPU 0.9.8 候选内核的一个 AMP DTS 变体，能否在**静态 DTB 层**同时做到：Linux 不再声明 A55 `cpu_l3`，并将 Zephyr 当前链接区域 `0x50000000`–`0x50100000` 标为不可映射的保留内存。

本实验不启动次级 CPU、不调用 SiP SMC、不启动 Zephyr，也不写 eMMC。后续仅在主机侧生成 RAM 启动 FIT；是否实际启动另行记录。

## 环境与前置条件

- 执行端：Arch Linux 主机（fish）。
- 板级源码：`src/rockchip-linux-kernel-r1-dts-port`，开始时为 `study/r1-dts-port` 的提交 `799622bab`；R1 DTS 移植及 RKNPU 0.9.8 候选内核已在此前实验中完成静态构建。
- 内核输出目录：`build/kernel-r1-dts-port/`，已有 ARM64 `rockchip_linux_defconfig` 与 R1 DTB 构建输出。
- Zephyr 已验证的独立启动链接区域：`0x50000000` 起，见 [EXP-20260819-002](exp-20260819-002-boot-zephyr-standalone-from-uboot.md)。
- 操作前状态：候选 Linux 仍在 DTS 中声明全部 8 个 CPU；未为 Zephyr 保留专用区域。

## 风险与恢复

- 影响范围：仅修改主机侧内核源码工作树中的 DTS Makefile 与新增 DTS 文件，并在主机输出目录生成 DTB。
- 板端影响：无；没有连接、加载或写入 R1，也没有改变 U-Boot、eMMC、CPU 在线状态或内存映射。
- 恢复方法：删除新增 DTS/Makefile 条目或用 Git 还原该工作树的未提交变更；`build/` 输出可重新生成且不纳入 Git。

## 步骤与证据

### 步骤 1：定位被 Linux 排除的 CPU 与关联 PMU 项

目的与预期结果：确认 `cpu_l3` 是 `cluster0/core3`，并确认 `arm_pmu` 的 affinity 列表必须同步删除该 CPU。

学习者在源码中读取到：

```text
cpu-map {
	cluster0 {
		core0 { cpu = <&cpu_l0>; };
		core1 { cpu = <&cpu_l1>; };
		core2 { cpu = <&cpu_l2>; };
		core3 { cpu = <&cpu_l3>; };
	};
}

cpu_l3: cpu@300 {
	compatible = "arm,cortex-a55";
	reg = <0x300>;
	enable-method = "psci";
};

arm_pmu: arm-pmu {
	interrupt-affinity = <&cpu_l0>, <&cpu_l1>, <&cpu_l2>, <&cpu_l3>,
			    <&cpu_b0>, <&cpu_b1>, <&cpu_b2>, <&cpu_b3>;
};
```

观察：`cpu_l3` 是小核簇的第四个 A55；只删除 CPU 节点会留下无效的 CPU map 与 PMU phandle，因此三个位置必须同步处理。

### 步骤 2：新增 AMP 静态检查 DTS 并注册 DTB

目的与预期结果：从现有 headless RKNPU 候选 DTS 派生新文件，保留既有 HDMI/显示规避配置，只加入 CPU 与内存声明的静态划分。

实际新增 `arch/arm64/boot/dts/rockchip/rk3588s-yyt-amp.dts`，其附加内容为：

```dts
/ {
	cpus {
		cpu-map {
			cluster0 {
				/delete-node/ core3;
			};
		};
	};

	reserved-memory {
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;

		zephyr_reserved: zephyr@50000000 {
			reg = <0x0 0x50000000 0x0 0x00100000>;
			no-map;
		};
	};
};

&arm_pmu {
	interrupt-affinity = <&cpu_l0>, <&cpu_l1>, <&cpu_l2>,
			     <&cpu_b0>, <&cpu_b1>, <&cpu_b2>, <&cpu_b3>;
};

/delete-node/ &cpu_l3;
```

并在 Rockchip DTS Makefile 中注册 `rk3588s-yyt-amp.dtb`。工作树差异统计为两个文件、81 行新增；`git diff --check` 和 `git diff --cached --check` 均无输出。

观察：变体保留原候选 DTS 的 headless RKNPU 配置。`no-map` 只让 Linux 不把这段物理 RAM 建立为常规映射；它不是 Zephyr 启动、缓存一致性或硬件访问授权机制。

### 步骤 3：通过 Kbuild 生成并反编译检查 DTB

目的与预期结果：由真实 Kbuild/DTC 处理 DTS，而非仅凭文本片段判断语法和节点合并结果。

```fish
make -C ~/Study/rk3588/src/rockchip-linux-kernel-r1-dts-port \
  O=~/Study/rk3588/build/kernel-r1-dts-port \
  ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  rockchip/rk3588s-yyt-amp.dtb
```

实际输出（学习者提供）：

```text
make: 进入目录“/home/loser/Study/rk3588/src/rockchip-linux-kernel-r1-dts-port”
  DTC     arch/arm64/boot/dts/rockchip/rk3588s-yyt-amp.dtb
make: 离开目录“/home/loser/Study/rk3588/src/rockchip-linux-kernel-r1-dts-port”
```

反编译后产物身份与关键节点：

```text
rk3588s-yyt-amp.dtb: Device Tree Blob version 17, size=233247,
boot CPU=0, string block size=20227, DT structure block size=212964
SHA-256: 891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22

arm_pmu: arm-pmu {
	interrupt-affinity = <0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c>;
};

zephyr_reserved: zephyr@50000000 {
	reg = <0x00 0x50000000 0x00 0x100000>;
	no-map;
};
```

对 `cpu@300|core3|zephyr@50000000|interrupt-affinity` 的完整匹配输出中只出现 PMU 与 `zephyr@50000000` 段；没有 `cpu@300` 或 `core3`。PMU 列表含 7 个 phandle，符合移除一个小核后的预期。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| Kbuild DTS 编译 | 生成独立 AMP DTB | DTC 成功生成 `rk3588s-yyt-amp.dtb` | 通过 |
| Linux CPU 描述 | 不含 `cpu_l3` / `cpu@300` | 反编译匹配中未出现 `cpu@300` | 通过（静态） |
| CPU 拓扑 | cluster0 不含 `core3` | 反编译匹配中未出现 `core3` | 通过（静态） |
| PMU affinity | 不再引用 `cpu_l3`，共 7 项 | 7 个 phandle | 通过（静态） |
| Zephyr RAM 区 | `0x50000000` 起 1 MiB 为 `no-map` | `reg=<0x0 0x50000000 0x0 0x100000>` 且含 `no-map` | 通过（静态） |
| AMP FIT 内核启动 | 进入候选 Linux | 早期 FIT 进入 `5.10.252`，但运行时仍使用 resource 内旧 DTB；替换 resource 后仍进入 `5.10.252` | 通过（仅内核；早期 DTB 覆盖问题见步骤 5） |
| AMP resource-DTB 到达 Linux | 7 CPU；无 `cpu@300`；存在 `zephyr@50000000` | 早期 FIT 为 8 CPU/旧 DTB；步骤 16 替换 resource 后为 7 CPU、`cpu@300` 缺失、保留节点存在 | 后续 resource 回归通过 |
| Linux+Zephyr AMP | 本实验不验证 | 未启动次级 CPU 或 Zephyr | 未验证 |

## 结论

R1 的 RKNPU 0.9.8 候选内核现具备一个可由 Kbuild 生成的 AMP 静态 DTS 变体：它从 Linux DTB 中排除了 `cpu_l3`、同步修正 PMU affinity，并把 Zephyr 现有镜像区域的首 1 MiB 标为 `no-map`。步骤 5–10 已保留并证明：早期 FIT 中的 AMP FDT 没有成为最终设备树，仍被 resource 内旧 `rk-kernel.dtb` 覆盖；步骤 16 以替换 resource 条目后的 RAM 候选才实际得到 7 CPU、没有 `cpu@300`、并看到了 Zephyr 保留区。该结果验证 Linux 资源排除声明已生效；不证明固件能启动被排除 CPU，也不证明 Zephyr 能与 Linux 并发执行。

### 步骤 4：生成静态 AMP 资源划分的 RAM 启动 FIT

目的与预期结果：把已验证的 AMP DTB 与已经完成 RKNPU/RKLLM RAM 启动验证的“无 Rockchip display DRM、无 Mali GPU”内核组合，形成只用于 U-Boot RAM 启动的候选；不写 p1/p3。

输入核对：

| FIT 部件 | 输入 | SHA-256 |
| --- | --- | --- |
| DTB | `build/kernel-r1-dts-port/.../rk3588s-yyt-amp.dtb` | `891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22` |
| Kernel | `build/kernel-r1-nodisplay-nogpu/.../Image` | `b26683be0c691b9676e298df4d751f8d1b225eaea086b6f65f2d8a81730e6a24` |
| Resource | `build/local/r1-20260816/r1-boot-resource.img` | `492cbec98ecfdd2b2d66f03694127d8744ec89339716b60b9d662456bc6569c6` |

主机生成命令：

```fish
mkimage -f ~/Study/rk3588/build/local/r1-20260816/r1-boot-fit-amp-static.its \
  -E -p 0x800 -B 0x200 \
  ~/Study/rk3588/build/local/r1-20260816/r1-boot-fit-amp-static.img

dumpimage -l ~/Study/rk3588/build/local/r1-20260816/r1-boot-fit-amp-static.img
sha256sum ~/Study/rk3588/build/local/r1-20260816/r1-boot-fit-amp-static.img
```

实际输出摘要：FIT 可由 `dumpimage` 解析，包含 233,247 B AMP DTB、34,836,992 B 内核和 638,976 B 原 R1 resource；输出文件 SHA-256 为 `64120723ace35d12045ccc40a3c115c6bf74177e9387fcc3fa3894736cc21cf4`。`conf` 引用 kernel、fdt 与 multi/resource。

传输核验（学习者提供）：文件已传至 R1 `/userdata/r1-ram-boot-test/r1-boot-fit-amp-static.img`，板端 `sha256sum` 输出同一值 `64120723ace35d12045ccc40a3c115c6bf74177e9387fcc3fa3894736cc21cf4`。它尚未由 U-Boot 加载或启动；p1/p3 和 U-Boot 环境均未改变。

观察：此组合把变量限制为 DTB 的 CPU/内存资源声明变化；内核与 resource 均沿用先前 RAM 启动链的输入。它仍不启动 Zephyr、不调用 SMC，也不构成 eMMC 镜像。

### 步骤 5：首次 RAM 启动后的 Linux 运行时核对

目的与预期结果：确认 FIT 中的 AMP DTB 是否实际到达 Linux，而不是仅确认候选内核本身启动。

学习者在 U-Boot 以 `ext4load mmc 0:8 0x0a200000 /r1-ram-boot-test/r1-boot-fit-amp-static.img` 和 `bootm 0x0a200000#conf` 启动后，Linux 中得到：

```text
root@R1:~# uname -r
5.10.252
root@R1:~# nproc
8
root@R1:~# cat /sys/devices/system/cpu/online
0-7
present: /proc/device-tree/cpus/cpu@300
absent:  /proc/device-tree/reserved-memory/zephyr@50000000
```

观察：`5.10.252` 证明本次 RAM FIT 的候选内核已启动；但在线 CPU 和运行时设备树都与 AMP DTB 的静态预期相反。因此，本次没有验证 Linux 的 7 CPU/`no-map` 资源划分，且尚未启动 Zephyr 或调用 SMC。**已验证现象**是 FIT 内核与运行时 DTB 未配套；**原因未知**，待检查 U-Boot 的 FDT 选择/搬运过程，不能先归因于 DTS 语法或 CPU 删除逻辑。

补充的运行时 FDT 身份（学习者提供）：`/sys/firmware/fdt` 存在，大小为 148 KiB，SHA-256 为 `57aedeb1e981f6ca56324c33aef173394e1edf2c82e38e9f4bb1951d5d33d47d`。它不等于 AMP DTB（233,247 B、`891778e…97a4c22`），也不等于主机普通 R1 DTB（`4d23f3a…7a7a1c2`）。该差异与 U-Boot 启动期修补 FDT 相容，但尚未导出结构，不能据此断定 U-Boot 选择了哪一份输入 DTB。

### 步骤 6：改变 FIT FDT load 地址的最小验证候选

假设：前一候选沿用 FIT FDT 的 `load = <0xffffff00>`，而 U-Boot 的已知 `fdt_addr_r` 为 `0x08300000`；候选内核启动但收到 8 CPU 基线 FDT，与 FDT 装载地址未实际接管启动路径相容。

为只改变这一变量，主机生成 `r1-boot-fit-amp-fdt-addr-r.img`：FDT 仍为同一 AMP DTB（SHA-256 `891778e…97a4c22`），kernel/resource 完全不变，仅将 FDT `load` 设为 `<0x08300000>`。`dumpimage` 已解析该候选，输出 SHA-256 为 `7f958cff20a7c44204814977e7922753bfe36ad840c3c7ca02049e212fd5531b`。

传输核验（学习者提供）：板端 `/userdata/r1-ram-boot-test/r1-boot-fit-amp-fdt-addr-r.img` 的 SHA-256 为同一值。它尚未由 U-Boot 加载或启动；p1/p3 与 U-Boot 环境均未改变，因此该假设仍待验证。

实际启动结果（学习者提供）：以同一 `ext4load` + `bootm` RAM 路径启动后，`nproc` 仍为 `8`、online 仍为 `0-7`，`cpu@300` 存在，`zephyr@50000000` 缺失。故“只需把 FIT FDT `load` 改为 `fdt_addr_r`”的假设被否定。该测试仍只在 RAM 运行，未写 p1/p3、未启动 Zephyr、未调用 SMC。

串口日志进一步显示：FIT 内 FDT 的 `Data Start` 为 `0x0a200800`，其 AMP DTB SHA-256 已校验为 `891778e…97a4c22`；但随后 U-Boot 输出 `Loading fdt from 0x08300000 to 0x08300000`，并从 `0x08300000` 启动 kernel。故已验证厂商 U-Boot 在 FDT 带 `load` 属性时没有把已验证的 FIT data 复制到该地址，而是把该地址上既有的基线 FDT 作为来源。该行为解释了候选内核与旧 DTB 混用。

### 步骤 7：移除 FDT `load` 属性的 in-place FIT 候选

目的与预期结果：避免 U-Boot 将 `load` 地址误作已装载数据的来源，让它直接使用 FIT 中已校验的 FDT 数据。此候选保持 AMP DTB、内核与 resource 完全不变，只移除 FDT 的 `load` 属性。

主机已生成 `r1-boot-fit-amp-fdt-in-place.img`。`dumpimage` 确认 FDT `Data Size` 为 233,247 B、hash 为 `891778e…97a4c22`，且不再显示 FDT Load Address；整体 SHA-256 为 `3baa28527dbac6ed86231466facf16abc932d1e8a2709a971ec8202a00b3f3bf`。

传输核验（学习者提供）：板端 `/userdata/r1-ram-boot-test/r1-boot-fit-amp-fdt-in-place.img` SHA-256 为相同值。尚未由 U-Boot 加载或启动；p1/p3 和 U-Boot 环境均未改变。

### 步骤 8：无 FDT `load` 属性候选的实机结果

目的与预期结果：验证厂商 U-Boot 能否直接将 FIT 内已校验的 AMP DTB 传给 Linux。

学习者在 U-Boot 执行：

```text
ext4load mmc 0:8 0x0a200000 r1-ram-boot-test/r1-boot-fit-amp-fdt-in-place.img
bootm 0x0a200000#conf
```

关键实际输出：

```text
Trying 'fdt' fdt subimage
  Description:  R1 AMP DTS used in place from FIT RAM data
  Data Start:   0x0a200800
  Data Size:    233247 Bytes = 227.8 KiB
Verifying Hash Integrity ... sha256+ OK
Booting using the fdt blob at 0xedd77a70
...
FDT and ATAGS support not compiled in - hanging
### ERROR ### Please RESET the board ###
```

观察：AMP DTB 的哈希仍经 U-Boot 验证，但移除 `load` 属性后，U-Boot 没有建立 Linux 所需的 FDT 交接地址，而是在 FDT/ATAGS 支持路径报错并停止。由此否定“只删除 `load` 即可直接传递 FIT 内 DTB”的假设。该次仅在 RAM 中加载与启动，未写 p1/p3、未保存 U-Boot 环境、未启动 Zephyr 或调用 SMC；复位后可回到原系统。

下一项待验证的最小方法：继续使用已验证的 `load = <0x08300000>` FIT，但在 `bootm` 前，将其已校验的 FIT FDT 数据 `0x0a200800`、长度 `0x38f1f` 显式复制到该 `load` 地址。此操作只覆盖 U-Boot 当前 DRAM 中的工作 FDT；目标地址位于已确认的 DRAM bank 内，复位即恢复，仍不写 eMMC。该假设尚未执行。

### 步骤 9：手动复制 FIT FDT 到 `fdt_addr_r` 后的运行时结果

目的与预期结果：验证将 FIT FDT 数据手动放入厂商 U-Boot 固定复用的 `0x08300000` 后，Linux 是否收到 AMP DTB。

学习者完成该 RAM 启动后，在 Linux 得到：

```text
0-7
cpu300-present
zephyr-reserved-absent
```

观察：Linux 仍管理 8 个 CPU，仍看到 `cpu@300`，仍未看到 Zephyr 的保留内存。由此否定“在 `bootm` 前向 `0x08300000` 手动复制 FIT FDT 即足以完成交接”的假设。当前输出尚不能区分两种原因：复制命令后的该地址不是预期 AMP DTB，或该地址虽正确但厂商 `bootm` 在后续阶段重新选择/覆盖了 FDT。下一次只在 U-Boot 检查复制后的 `0x08300000` 节点，不启动 kernel，以区分这两个假设。

### 步骤 10：验证手动复制后的 U-Boot FDT 内容

目的与预期结果：在不进入 `bootm` 的条件下，确认 `cp.b` 后的 `0x08300000` 是否已经是 AMP DTB。

学习者在重新加载、复制后执行：

```text
fdt addr 0x08300000
fdt print /reserved-memory/zephyr@50000000
```

实际输出：

```text
zephyr@50000000 {
	reg = <0x00000000 0x50000000 0x00000000 0x00100000>;
	no-map;
	phandle = <0x00000402>;
};
```

结论：已验证 `0x08300000` 在进入 `bootm` 前确实保存 AMP DTB；结合步骤 9 的 Linux 运行时仍为旧 DTB，可排除 FIT 数据源、复制地址和复制长度错误。**已验证**问题位于厂商 `bootm` 的后续 FDT 处理/选择阶段；具体是替换、重载还是板级 fixup 尚未确定。下一步只读定位串口中 `BOOTM: transferring to board FIT` 与 `DTB: rk-kernel.dtb` 文本在本机可用 U-Boot 源码/资料中的实现入口，不再重启或启动候选。

### 步骤 11：确认 `resource` 内的 `rk-kernel.dtb` 身份

目的与预期结果：验证串口的 `DTB: rk-kernel.dtb` 是否指向当前 `resource` 载荷内的旧 R1 DTB。

主机对原 `r1-boot-resource.img` 的索引表和内容做只读检查。其头部声明 3 个索引项；首项名称为 `rk-kernel.dtb`，元数据为 `hash_size=0x14`、`content_offset=0x4`（即 `0x800`）、`content_size=0x24172`（147,826 B）。从该偏移取出的 147,826 B 内容哈希为：

```text
resource rk-kernel.dtb sha256:
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546

original boot FIT fdt sha256:
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546

AMP DTB sha256:
891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22
```

结论：**已验证**原 `resource` 中的 `rk-kernel.dtb` 与原 boot FIT FDT 字节完全相同，且不同于 AMP DTB。结合 U-Boot 的 `DTB: rk-kernel.dtb` 日志、`resource_tool.c` 注释中“与 U-Boot `rkloader.c` 的 `FDT_PATH` 同步”的 `FDT_PATH "rk-kernel.dtb"`，可作出高置信度推断：厂商 `bootm` 的板级路径以 resource 内该条目作为最终 Linux DTB，覆盖或绕开 FIT `fdt` 子镜像。这解释了前述所有运行时旧 DTB 现象；仍需以替换 resource 条目的 RAM 候选进行实机验证。

### 步骤 12：解包原 resource 并确认可重建输入

目的与预期结果：用与厂商 resource 格式匹配的本机 `resource_tool.c` 解包原 resource，确认仅替换 DTB 时需要保留的条目。

学习者在 Arch 主机编译工具并解包：

```fish
cc -O2 -o ~/Study/rk3588/build/local/r1-resource-amp/resource_tool \
  ~/Study/rk3588/src/youyeetoo-r1-linux-kernel-5-10/scripts/resource_tool.c

~/Study/rk3588/build/local/r1-resource-amp/resource_tool --unpack \
  --image=~/Study/rk3588/build/local/r1-20260816/r1-boot-resource.img \
  ~/Study/rk3588/build/local/r1-resource-amp/original
```

编译出现 4 条 `assignment discards ‘const’ qualifier` 警告，均位于充电动画参数解析函数；工具仍成功执行。解包输出为：

```text
entry(0): path:rk-kernel.dtb  offset:4    size:147826
entry(1): path:logo.bmp       offset:293  size:215150
entry(2): path:logo_kernel.bmp offset:714 size:273006
Unack ... successed!
rk-kernel.dtb  147826 bytes
logo.bmp  215150 bytes
logo_kernel.bmp  273006 bytes
```

观察：resource 的三个实际内容项与索引分析一致。新 AMP DTB 为 233,247 B，较旧 DTB 大，因而不能原位覆盖；必须经工具重建索引、内容偏移与条目哈希。当前仅创建主机 `build/local/` 工作文件，未改源码、未操作板端或 eMMC。

### 步骤 13：重建并回读验证 AMP resource 候选

目的与预期结果：只替换 resource 内的 `rk-kernel.dtb`，确认重新打包后 DTB 正确、两个 logo 未变，且条目偏移已随更大的 DTB 更新。

学习者将 AMP DTB 复制到解包目录的 `rk-kernel.dtb`，再以：

```fish
resource_tool --pack --root=$work/original --image=$work/r1-resource-amp.img \
  $work/original/rk-kernel.dtb $work/original/logo.bmp \
  $work/original/logo_kernel.bmp
```

重建完成后重新解包到 `verify/`。学习者提供的回归输出：

```text
891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22  original/rk-kernel.dtb
891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22  verify/rk-kernel.dtb
logo-ok
kernel-logo-ok
```

主机进一步只读核验新 resource：文件为 724,480 B、SHA-256 `d055083f97efe0a14c6c13d16b6b76946ec9eb911be6160a04b85f2c0bda52b2`；其索引为 `rk-kernel.dtb` 233,247 B、offset 4，`logo.bmp` 215,150 B、offset 460，`logo_kernel.bmp` 273,006 B、offset 881。

结论：resource 候选在主机侧通过内容回归验证。相较原 resource（638,976 B），变大是 DTB 增长及按 512 B block 重排的正常结果；仍远小于原 64 MiB boot 分区，但尚未封入 FIT 或上板。未改源码、未操作板端或 eMMC。

### 步骤 14：封装含 AMP resource 的 RAM 启动 FIT

目的与预期结果：将已回读验证的新 resource 与此前实际启动过的无显示/无 Mali RKNPU 0.9.8 内核、AMP DTB 放入同一 FIT；确认三个子镜像均为预期输入后，才传入 R1 的 `/userdata` 做 RAM 启动回归。

学习者在 Arch 主机执行：

```fish
mkimage -f ~/Study/rk3588/build/local/r1-resource-amp/r1-boot-fit-amp-resource.its \
  -E -p 0x800 -B 0x200 \
  ~/Study/rk3588/build/local/r1-resource-amp/r1-boot-fit-amp-resource.img

dumpimage -l ~/Study/rk3588/build/local/r1-resource-amp/r1-boot-fit-amp-resource.img
sha256sum ~/Study/rk3588/build/local/r1-resource-amp/r1-boot-fit-amp-resource.img
```

`dumpimage` 的实际关键结果：

| 子镜像 | 大小 | SHA-256 |
| --- | ---: | --- |
| AMP FDT | 233,247 B | `891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22` |
| 5.10.252 kernel | 34,836,992 B | `b26683be0c691b9676e298df4d751f8d1b225eaea086b6f65f2d8a81730e6a24` |
| 含 AMP DTB 的 resource | 724,480 B | `d055083f97efe0a14c6c13d16b6b76946ec9eb911be6160a04b85f2c0bda52b2` |

配置 `conf` 引用 `kernel`、`fdt` 和 `resource`（`multi`）。生成的整体 FIT SHA-256 为 `a1359145cf07457aac2d7d628bf424201b6f4f632ffeb1dc482fd4327a2695bb`。

结论：主机侧组合载荷已静态通过；DTB 与 resource 内 DTB 都是同一个 AMP DTB。这还不是板端回归：尚未传输、未进入 U-Boot、未写 p1/p3、未保存 U-Boot 环境，未启动 Zephyr 或调用 SMC。

### 步骤 15：板端中转传输完整性核验

目的与预期结果：在进入 U-Boot 前确认 `/userdata` 中的 FIT 没有传输损坏，避免把网络传输问题混入后续启动结果。

学习者通过 SSH 在 R1 执行：

```text
a1359145cf07457aac2d7d628bf424201b6f4f632ffeb1dc482fd4327a2695bb  /userdata/r1-ram-boot-test/r1-boot-fit-amp-resource.img
```

结论：板端文件 SHA-256 与主机 FIT 一致。该步骤仅在 `/userdata` 新增中转文件；未写 p1/p3、未改变 U-Boot 环境，尚未从 U-Boot 加载或启动候选。

### 步骤 16：resource-DTB 候选的 RAM 启动回归

目的与预期结果：验证替换 resource 内 `rk-kernel.dtb` 后，厂商 U-Boot 实际交给 Linux 的设备树是否变为 AMP DTB。

学习者在 U-Boot 从 `mmc 0:8` 将候选读取至 `0x0a200000`，再执行 `bootm 0x0a200000#conf`。进入 Linux 后按顺序检查内核、CPU、运行时 DTS。学习者提供的原始串口文本为：

```text
5.10.252zephyr-reserved-absent
7
0-6
cpu300-absent
zephyr-reserved-present
```

其中首行尾部的 `zephyr-reserved-absent` 与内核版本输出粘连，属于此前或并行串口输出，不能把它解释为本轮最终检查结论。其后本轮按预期顺序得到 `nproc = 7`、online CPU 为 `0-6`、`cpu300-absent`、`zephyr-reserved-present`。

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 候选内核 | `5.10.252` | `5.10.252` | 通过 |
| Linux CPU 数 | 7 | 7 | 通过 |
| `cpu@300` | 不存在 | `cpu300-absent` | 通过 |
| Zephyr carveout | 存在 | `zephyr-reserved-present` | 通过 |

结论：resource 中的新 `rk-kernel.dtb` 已实际到达 Linux，完成了 Linux 侧对 A55 core3 的静态排除与 Zephyr 1 MiB `no-map` carveout 的 RAM 启动回归。该结果不等于 Zephyr 已启动，也未证明固件能启动被排除 CPU、CPU 执行上下文隔离、缓存一致性或 IPC；p1/p3 和 U-Boot 环境仍未写入。

### 步骤 17：7 CPU 候选上的 RKLLM 兼容性回归

目的与预期结果：确认 Linux CPU/carveout 资源划分没有破坏已验证的 RKNPU 0.9.8 + RKLLM 主线。

学习者在目标 Bash 中执行：

```bash
cd /userdata/rkllm-api-demo
./llm_demo-r1 ./models/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm 2048 4096
```

实际输出在 `rkllm_init()` 阶段停止：

```text
I rkllm: rknpu driver version: 0.9.8, platform: RK3588
I rkllm: Enabled cpus: [0, 1, 2, 3, 4, 5, 6]
I rkllm: Enabled cpus num: 4
E rkllm: Mismatch between enabled CPUs mask and expected count. Please check the configuration.
rkllm init failed
```

观察：此前 8 CPU 候选的同一 Runtime/模型曾列出 `[4, 5, 6, 7]` 并完成文本生成；当前失败发生在 NPU 推理提交之前。故本轮未完成“7 CPU 下 NPU LLM 回归”，也不能据此判定 RKNPU 驱动或模型退化。该现象作为独立兼容性问题记录于 [ISSUE-20260821-001](../issue/issue-20260821-001-rkllm-cpu-mask-after-amp-carveout.md)。

学习者再以 `taskset -c 3-6` 仅限制该 demo 进程到四个 CPU 后重复运行，Runtime 仍打印 `[0, 1, 2, 3, 4, 5, 6]` 并报同一 mismatch。故进程级 affinity 不会改变其 CPU 选择；当前应从系统 CPU topology 而不是继续穷举 `taskset` CPU 组合排查。

随后目标侧只读最大频率输出为：

```text
cpu0 max_khz=1800000
cpu1 max_khz=1800000
cpu2 max_khz=1800000
cpu3 max_khz=2256000
cpu4 max_khz=2256000
cpu5 max_khz=2256000
cpu6 max_khz=2256000
```

结论：当前 7 CPU 运行时的 3 个 A55 是 CPU0–2，4 个 A76 已重新编号为 CPU3–6。它解释了 Runtime 从此前可用的 `[4,5,6,7]` 选择变为错误的全体 `[0..6]` 的环境差异，但未反编译或证明 Runtime 的内部判断算法。下一步使用 `rkllm.h` 提供的显式 CPU 配置字段验证，而不是撤销静态隔离，决策见 [DEC-20260821-004](../decision/dec-20260821-004-preserve-rkllm-cpu-numbering-during-amp-prototype.md)。

### 步骤 18：显式 A76 CPU mask 的 RKLLM 回归

目的与预期结果：保持 resource-DTB 已验证的 7 CPU/Zephyr carveout，不改 RKNPU driver，通过 RKLLM 公共头文件的参数让 Runtime 明确使用当前四个 A76 CPU。

学习者在目标原生编译的独立 demo 中，基于原厂 `llm_demo.cpp`，仅在既有 `base_domain_id` 与 `embed_flash` 配置后增加：

```cpp
param.extend_param.enabled_cpus_num = 4;
param.extend_param.enabled_cpus_mask = CPU3 | CPU4 | CPU5 | CPU6;
```

原 `llm_demo-r1` 未被覆盖；该 demo 使用同一 `librkllmrt.so`、模型和 RKNPU 0.9.8。实际运行输出：

```text
I rkllm: rknpu driver version: 0.9.8, platform: RK3588
I rkllm: Enabled cpus: [3, 4, 5, 6]
I rkllm: Enabled cpus num: 4
rkllm init success
user: ok
robot: Alright,
```

结论：在 Linux 已让出 A55 core3、剩余 7 CPU 且 Zephyr carveout 仍生效的同一 RAM 候选中，RKLLM 完成了真实短文本生成。此问题是 demo 未显式适配变更后的 Linux logical CPU 编号，不是本轮 RKNPU driver 的故障；driver 全程仍为 `0.9.8`。该结果不代表 Zephyr 已启动或 Linux/Zephyr 已并发运行。

## 关联知识与问题

- 资源划分方向：[Linux+Zephyr AMP 长期方向](../decision/dec-20260810-002-linux-zephyr-amp-long-term-direction.md)。
- 运行时前置盘点：[EXP-20260817-001](exp-20260817-001-inventory-r1-amp-runtime-prerequisites.md)。
- 独立 Zephyr 启动和内存基址：[EXP-20260819-002](exp-20260819-002-boot-zephyr-standalone-from-uboot.md)。

## 后续行动

- [ ] 只读梳理 R1 当前 BL31、Rockchip AMP driver 与 `amp-cpus` DTS 所要求的 CPU 启动责任链；在明确固件支持前，不调用 SMC、不启动次级 CPU、不写 eMMC。
