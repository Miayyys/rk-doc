---
title: "DEC-20260902-001 建立 LZAMP 作为后续工程根目录并迁移至 develop-6.12"
type: decision
status: active
created: 2026-09-02
updated: 2026-09-02
tags: [rk3588, amp, linux, zephyr, lzamp]
related:
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
  - "[[experiment/exp-20260901-001-mailmsg-protocol-performance]]"
  - "[[status/current]]"
---

# DEC-20260902-001 建立 LZAMP 作为后续工程根目录并迁移至 develop-6.12

## 背景与约束

R1 Linux+Zephyr AMP 与 MailMsg 原型探索已基本完成。后续需要一个独立的工程根目录，统一容纳新的 Rockchip Linux、新 Zephyr、MailMsg 以及工具和测试，便于以后独立上传 GitHub。现有 Linux `5.10.252` + MailMsg V1 R7 的 RAM-only 验证基线应保留为参考；它没有写入 eMMC。

后续源码目标采用 Rockchip 官方最高 BSP 分支 `develop-6.12`，锁定 commit `470f9dccbdc42e7b8a824d0a5c5640a10e9457d2`，版本为 `6.12.69`。最高版本不等于最活跃：已知 6.12 tip 事件日期为 2026-06-25，仍需板上验证。6.12 源码侧已确认包含 `ROCKCHIP_AMP`、`RKNPU`、RK3588 AMP DTS，以及 MT7921E PCI 表中的 `14c3:0616`/MT7922 条目；这些源码存在性事实不等于 R1 运行兼容性。

## 候选方案

| 方案 | 优点 | 缺点 | 风险 | 验证情况 |
| --- | --- | --- | --- | --- |
| 在现有 Linux `5.10.252` + MailMsg V1 R7 工作树上继续扩展 | 保留已验证的 R1 RAM-only 基线，改动少 | 原型与后续工程边界混在一起，难以独立整理和上传 | 继续受旧 BSP、旧板级适配和未验证补丁影响 | R7 RAM-only 基线已验证；不作为后续新工程根目录 |
| **建立 LZAMP，纳入锁定的 `develop-6.12`** | 新 Linux、Zephyr、MailMsg、工具和测试集中管理，便于独立发布 | 需要重新做源码、配置、DTS、启动和 ABI 适配 | 6.12 最高版本不代表活跃度或 R1 兼容性；R1 板级适配仍未完成 | LZAMP 主机四项单元测试通过；迁移后的 Zephyr app 使用 Zephyr 4.4.0 与现有交叉工具链构建成功；Image/官方 EVB4 DTB 主机构建成功，并在 EVB4 诊断 DTB 上完成 RAM-only booti/eMMC 短读；R1 兼容仍待验证 |
| 选择其他 BSP 分支作为新根目录 | 可能更接近既有资料或活跃开发 | 与“锁定官方最高 BSP”目标不一致，需重新比较 | 分支选择依据和后续维护边界不明确 | 不采用；未形成新的验证结论 |

## 决定

建立 **LZAMP** 作为后续工程根目录。今后新的 Rockchip Linux、新 Zephyr、MailMsg 实现以及相关工具/测试均放入该工程边界，目标是以后可以独立上传 GitHub。

将 Rockchip 官方 `develop-6.12` 固定在 commit `470f9dccbdc42e7b8a824d0a5c5640a10e9457d2`（版本 `6.12.69`）作为新工程的 Linux 源码目标。保留 Linux `5.10.252` + MailMsg V1 R7 RAM-only 已验证基线，不写入 eMMC；5.10 R7 与 post-R7 direct-doorbell 工作树补丁分开，后者尚未验证。

## 影响与复查条件

- 影响：后续工程文档、构建、测试和发布边界以 LZAMP 为根；既有 R7 结果只作为 `5.10.252`/MailMsg V1 的参考基线，不自动迁移为 6.12 已验证事实。
- 当前已确认：6.12 源码含 AMP、RKNPU、RK3588 AMP DTS 和 MT7921E PCI 表条目；LZAMP 主机四项单测通过，迁移后的 Zephyr app 已用现有 Zephyr 4.4.0 与交叉工具链构建成功。`rockchip_linux_defconfig` 生成的 Image/官方 EVB4 DTB 也已在主机成功构建；现有厂商 U-Boot 可直接 `ext4load` 后 `booti`，在诊断 DTB 上完成 EVB4 model 的 RAM-only 启动和 eMMC 短读。项目自有 DTS/DTB 已统一命名为 `rk3588s-lzamp-linux.dts/.dtb`，model/compatible 已固定，主机构建 DTB SHA-256 为 `1ddfea41247dc88f80af6b25782bdceaae7cc8897ecdc25d046c4066113d8d95`。
- 尚未验证：上述正式命名 DTB 尚未上板回归；R1 最小板级 DTS、R1 启动兼容性、RKLLM ABI、MailMsg/PSCI、Wi-Fi firmware，以及 6.12 在 R1 上的 AMP、MailMsg、NPU、无线、显示和长期运行兼容性。旧 5.10/YYT DTS 名称仅为历史证据，不代表 LZAMP 新 DTS 命名或运行结果。
- 何时需要重新评估：源码/config/DTS preflight 或后续 RAM 启动验证发现 6.12 无法满足 R1 的硬件资源、AMP、NPU 或 MailMsg 约束时，再重新比较其他官方分支；在此之前不把“最高版本”写成“最活跃”或“已适配”。
