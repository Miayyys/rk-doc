---
title: "DEC-20260810-002 Linux+Zephyr AMP 长期学习方向"
type: decision
status: active
created: 2026-08-10
updated: 2026-08-19
tags: [rk3588, linux, zephyr, amp, edge-ai]
related:
  - "[[roadmap/learning-roadmap]]"
  - "[[status/current]]"
  - "[[experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites]]"
  - "[[experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot]]"
---

# DEC-20260810-002 Linux+Zephyr AMP 长期学习方向

## 背景与约束

学习者的终极项目是同一 RK3588S 上的 Linux+Zephyr 异构多处理（AMP）：Linux 运行 LLM 和边缘 AI 工作负载，Zephyr 执行实时任务。希望隔离性尽可能接近 MPU+MCU，同时具备低延迟消息同步和高频、大量数据交换能力。

当前运行的是厂商 Ubuntu 22.04 / Linux 5.10 系统；Zephyr 的具体任务、外设、数据流和实时指标均未确定。现阶段不进行 Zephyr 构建、启动链替换或烧录。

## 候选方向

| 方案 | 优点 | 缺点 | 风险 | 验证情况 |
| --- | --- | --- | --- | --- |
| 仅 Linux 绑核或隔离核 | 可快速试验 | Linux 仍控制中断、内存与设备，不能形成 Zephyr 独立执行环境 | 将 CPU 亲和性误判为 AMP 隔离 | 未采用为终极架构 |
| Linux + Zephyr AMP | 可按核、内存和外设划分职责，适合实时控制与 AI 协同 | 启动、内存、缓存、IPC 和设备所有权复杂 | 同 SoC 仍共享物理资源；强隔离能力需逐项验证 | 选作长期方向，尚未实现 |
| 外置 MCU + Linux | 物理隔离更强 | 增加硬件、总线和系统集成成本 | 不满足当前“先在 RK3588 上深入学习”的重点 | 保留为后续对照方案 |

## 决定

采用 Linux+Zephyr AMP 作为**长期综合项目方向**，但按现有路线先完成 Linux、启动链、设备树和驱动基础。未来的最小原型优先考虑：一个 Zephyr 核、私有内存与独占外设，以及小型受控共享内存窗口；控制消息与批量数据分开设计。第二个 Zephyr 核、EL2 虚拟化、IOMMU/DMA 隔离及高带宽 IPC 均须在基础原型有实测证据后再评估。

“接近 MPU+MCU”在此仅是期望的资源/故障隔离目标，不能预先等同于独立 MCU 的物理或安全隔离。

## 近期实施顺序（2026-08-17）

将近期工作明确拆成两个阶段，而不是立即将 LLM、外设控制和 Zephyr 一次组装：

1. **AMP 可行性原型优先**：先以一个 CPU 核、保留内存和 UART 心跳验证 Zephyr 的启动责任链与独立运行；随后验证最小 Linux↔Zephyr 通信。此阶段不要求接入 GPIO、传感器或 LLM。
2. **系统组装随后进行**：仅在 AMP 原型取得实机证据后，再将已验证的 Linux/RKNPU LLM 与 Zephyr 的实时外设任务通过受控 IPC 集成。

当前 RKNPU 0.9.8 FIT 是仅用于 RAM 启动验证的候选：它刻意关闭 Rockchip 显示 DRM 与 Mali GPU，虽已完成 W8A8 LLM 生成，但不是可替换原厂系统的完整 eMMC 启动镜像。因此近期保持原厂 eMMC `boot` 分区不变；候选 FIT 仅保留在 `/userdata` 和主机构建目录，作为可回归的临时启动载荷。

AMP 的第一轮只读勘察要回答三个问题：现有启动链能否为 Zephyr 保留并启动次级 CPU、Linux 能否可靠地排除该 CPU 与对应内存、现有 DTS/内核是否已有 remoteproc、mailbox 或其他合适 IPC 基础。CPU 亲和性或绑核本身不作为 AMP 隔离成功证据。

## 影响与复查条件

- 影响：后续 Linux 学习优先关注启动链、内存、设备树、驱动模型、中断、缓存和通信机制，为 AMP 做前置准备。
- 何时需要重新评估：若第一轮勘察表明次级核启动或内存所有权无法在现有链路安全实现；明确 Zephyr 的任务、独占外设、消息大小、峰值吞吐与时延目标后；或验证表明同 SoC 隔离无法满足实际需求时。
