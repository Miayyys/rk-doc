---
title: "ISSUE-20260820-001 resource DTB 覆盖 FIT AMP DTB"
type: issue
status: resolved
created: 2026-08-20
updated: 2026-08-21
tags: [rk3588, u-boot, fit, device-tree, amp]
related:
  - "[[experiment/exp-20260820-001-static-amp-dts-resource-partition]]"
  - "[[note/uboot-fit-image]]"
  - "[[note/r1-emmc-partition-layout]]"
  - "[[status/current]]"
---

# ISSUE-20260820-001 resource DTB 覆盖 FIT AMP DTB

## 现象与影响

- 首次发现时间：2026-08-20（具体时间未记录）。
- 可观察现象：AMP DTB 经 Kbuild 生成且由 U-Boot 对 FIT 子镜像完成 SHA-256 校验，但候选 Linux 仍为 8 个在线 CPU，存在 `cpu@300`，没有 `zephyr@50000000`。
- 影响范围：AMP DTS 的 CPU 隔离和 Zephyr `no-map` 内存划分均不能到达 Linux；尚未影响已验证的 RKNPU/RKLLM RAM 推理。
- 发生频率：对 FIT 内 `fdt` 直接替换、改变 `load` 地址和手动覆盖 `fdt_addr_r` 的已执行测试均复现运行时旧 DTB。

## 环境与复现

- 环境基线：R1、厂商 U-Boot、RAM 启动的 Linux 5.10.252 RKNPU 0.9.8 无显示/无 Mali 候选；eMMC p1/p3 未写入。
- 最近变更：仅在 RAM FIT 内引入 AMP DTB；该 DTB 静态排除 `cpu@300`、加入 `zephyr@50000000`。
- 最小复现步骤：从 `/userdata/r1-ram-boot-test/` 以 `ext4load mmc 0:8 ...` 读取 AMP FIT，并用 `bootm <addr>#conf` 启动；进入 Linux 后检查 CPU 与保留内存节点。
- 预期结果：Linux 有 7 个 CPU，且设备树存在 `zephyr@50000000`。
- 实际结果：Linux 输出 `0-7`，`cpu300-present`，`zephyr-reserved-absent`。

## 原始证据

```text
U-Boot FIT 子镜像：
  Description: R1 AMP DTS ...
  Data Size: 233247 Bytes
  Hash value: 891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22
  Verifying Hash Integrity ... sha256+ OK

Linux 运行时：
0-7
cpu300-present
zephyr-reserved-absent

U-Boot 在手动复制后：
fdt print /reserved-memory/zephyr@50000000
zephyr@50000000 {
    reg = <0x00000000 0x50000000 0x00000000 0x00100000>;
    no-map;
};

原 resource 的 rk-kernel.dtb SHA-256：
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546
原 boot FIT FDT SHA-256：
abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546
AMP DTB SHA-256：
891778e2332f2238c784a0f4371f695d87470102c3bbf8d6c9c50172e97a4c22
```

## 关联知识与实验

- FIT 与 resource 的外置载荷结构：[FIT 笔记](../note/uboot-fit-image.md)。
- eMMC `boot` 分区结构：[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。
- 主实验和逐步日志：[EXP-20260820-001](../experiment/exp-20260820-001-static-amp-dts-resource-partition.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：AMP DTS 编译或 FIT 哈希错误 | Linux 收到旧 DTB | Kbuild 静态检查与 U-Boot FIT SHA-256 校验 | AMP DTB 节点正确，哈希校验通过 | 排除 |
| H2：FIT FDT 未放到 U-Boot 的固定 FDT 地址 | `load=0x08300000` 后仍为旧树 | 手动复制 AMP DTB，再在 U-Boot 打印节点 | `0x08300000` 可打印 `zephyr@50000000` | 排除 |
| H3：厂商 `bootm` 后续改选 resource 中的 `rk-kernel.dtb` | 串口显示 `DTB: rk-kernel.dtb`；resource 有同名条目 | 对 resource 条目与原 FIT FDT 做内容哈希对照，并用厂商工具解包 | 三项已成功解包；`rk-kernel.dtb` 与原 FIT FDT 哈希相同，均不同于 AMP DTB | 高置信度，待替换 resource 回归 |

## 根因

**已由回归验证支持的结论**：厂商 `bootm` 的板级路径最终采用 `resource.img` 内的 `rk-kernel.dtb`，使 FIT 的 `fdt` 子镜像未成为 Linux 的基础设备树。原 resource 的该条目仍是 R1 原厂 DTB。

当前没有与板端 U-Boot 精确匹配的 `rkloader.c` 源码，故“覆盖、重载或绕开”的具体函数级实现未知；不能将该部分写为已验证事实。

## 解决或绕过方法

- 临时诊断法：在 U-Boot RAM 内检查/替换 `fdt_addr_r`，可验证地址内容，但不能绕过后续 resource 选择。
- 已验证绕过：解包原 resource，保持两个 logo 条目不变，仅用 AMP DTB 替换 `rk-kernel.dtb`，重建 resource 后作为 RAM FIT 的 resource 子镜像启动。
- 保持的边界：本次仅 RAM 回归；不写 eMMC p3、不更新 U-Boot 环境、不启动 Zephyr、不调用 AMP SMC。

## 回归验证

主机侧 resource 重建已通过：回读的 `rk-kernel.dtb` 为 AMP DTB 哈希 `891778e…97a4c22`，两个 logo 与解包输入字节一致；新 resource 为 724,480 B、SHA-256 `d055083f…0bda52b2`。它已与同一 AMP DTB、既有候选 kernel 封入 RAM FIT，整体 SHA-256 `a1359145…7a2695bb`。

RAM 回归已完成：候选启动后 Linux 为 `5.10.252`、`nproc` 为 7、online CPU 为 `0-6`、`cpu300-absent`、`zephyr-reserved-present`。这满足本问题的成功标准。Zephyr 启动与 IPC 不属于本问题的回归范围。

## 经验与后续行动

- 可复用的排障方法：当 FIT 内 DTB 校验正确而 Linux 设备树不符时，分别验证 FIT data、U-Boot 工作 FDT、resource 条目和运行时 FDT，不能只看 FIT 校验成功。
- 已完成：resource-DTB RAM 回归。后续工作转入“在 7 CPU Linux 下复验 NPU LLM”及真正的次级 CPU 启动可行性，不再作为本问题的待办。
