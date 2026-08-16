# 实验记录

每次实际操作使用独立的 `EXP-YYYYMMDD-NNN` 记录，采用 [`templates/experiment.md`](../templates/experiment.md)。同一天编号从 `001` 递增，文件名形如 `exp-20260805-001-identify-board.md`。

实验失败也必须保留。重复实验在原记录追加带时间的运行结果；环境或目标发生明显变化时新建实验，并互相链接。

## 索引

### 阶段 0：环境与设备识别

- [EXP-20260805-001：识别 Rockchip USB 设备与启动模式](exp-20260805-001-identify-rockusb-device.md)
- [EXP-20260807-001：连接并验证 Debug UART](exp-20260807-001-connect-debug-uart.md)
- [EXP-20260807-002：通过 Debug UART 观察 Linux 启动](exp-20260807-002-boot-linux-via-debug-uart.md)

### 阶段 1：主机侧启动软件学习准备

- [EXP-20260809-003：检查 U-Boot 主机构建前置工具](exp-20260809-003-check-uboot-host-build-prerequisites.md)
- [EXP-20260809-004：获取并验证上游 U-Boot 源码](exp-20260809-004-acquire-upstream-uboot-source.md)

### 阶段 2：Linux 系统组成

- [EXP-20260810-001：识别运行中 Linux 的 PID 1](exp-20260810-001-identify-linux-pid-1.md)
- [EXP-20260812-001：定位 R1 运行中内核的构建配置](exp-20260812-001-locate-running-kernel-config.md)

### 阶段 3：内核与驱动基础

- [EXP-20260812-002：筛选第一个简单设备驱动实验候选](exp-20260812-002-select-first-simple-device.md)
- [EXP-20260812-003：检查运行内核源码入口](exp-20260812-003-check-running-kernel-source-availability.md)
- [EXP-20260812-004：定位上游 GPIO 驱动源码的有效 ref](exp-20260812-004-locate-upstream-gpio-source-ref.md)（进行中）

### 阶段 5：边缘 AI

- [EXP-20260814-001：检查 R1 NPU 首次配置脚本线索](exp-20260814-001-inspect-r1-npu-first-boot-script.md)（进行中）
- [EXP-20260815-002：探测 R1 NPU 运行时链路](exp-20260815-002-probe-r1-npu-runtime-chain.md)（进行中）

### 阶段 0：镜像与恢复准备

- [EXP-20260815-001：检查 R1 Ubuntu Camera 候选镜像](exp-20260815-001-inspect-r1-ubuntu-camera-image.md)（进行中）

实验结论应通过正文和 `related` 关联到对应知识笔记或问题记录；索引只按学习阶段提供入口。
