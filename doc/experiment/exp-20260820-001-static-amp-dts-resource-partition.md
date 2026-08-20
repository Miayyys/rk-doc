---
title: "EXP-20260820-001 静态验证 R1 AMP DTS 的 CPU 与内存划分"
type: experiment
status: verified
created: 2026-08-20
updated: 2026-08-20
tags: [rk3588, amp, device-tree, zephyr, kernel]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites]]"
  - "[[experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot]]"
  - "[[status/current]]"
---

# EXP-20260820-001 静态验证 R1 AMP DTS 的 CPU 与内存划分

## 目标

验证基于已完成 R1 DTS 移植、RKNPU 0.9.8 候选内核的一个 AMP DTS 变体，能否在**静态 DTB 层**同时做到：Linux 不再声明 A55 `cpu_l3`，并将 Zephyr 当前链接区域 `0x50000000`–`0x50100000` 标为不可映射的保留内存。

本实验不启动次级 CPU、不调用 SiP SMC、不启动 Zephyr、不生成或加载 FIT，也不写 eMMC。

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
| Linux+Zephyr AMP | 本实验不验证 | 未启动候选 Linux/次级 CPU/Zephyr | 未验证 |

## 结论

R1 的 RKNPU 0.9.8 候选内核现具备一个可由 Kbuild 生成的 AMP 静态 DTS 变体：它从 Linux DTB 中排除了 `cpu_l3`、同步修正 PMU affinity，并把 Zephyr 现有镜像区域的首 1 MiB 标为 `no-map`。该结论只证明设备树资源声明正确，不证明 Linux 实际不会调度该核，也不证明固件能启动它或 Zephyr 能与 Linux 并发执行。

## 关联知识与问题

- 资源划分方向：[Linux+Zephyr AMP 长期方向](../decision/dec-20260810-002-linux-zephyr-amp-long-term-direction.md)。
- 运行时前置盘点：[EXP-20260817-001](exp-20260817-001-inventory-r1-amp-runtime-prerequisites.md)。
- 独立 Zephyr 启动和内存基址：[EXP-20260819-002](exp-20260819-002-boot-zephyr-standalone-from-uboot.md)。

## 后续行动

- [ ] 只读核对 R1 当前启动链的 BL31/SiP 是否支持 `RK_SIP_AMP_CFG (0x82000022)`，以确定 Linux 被排除的 `cpu_l3` 是否存在可验证的启动责任链；不向板端发出 SMC、不加载此 AMP DTB、不写 eMMC。
