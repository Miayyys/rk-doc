---
title: "EXP-20260805-001 识别 R1 的 Rockchip USB 设备"
type: experiment
status: verified
created: 2026-08-05
updated: 2026-08-05
tags: [rk3588, r1, rockusb, maskrom]
related:
  - "[[status/current]]"
  - "[[environment/hardware]]"
---

# EXP-20260805-001 识别 R1 的 Rockchip USB 设备

## 目标

确认主机识别到的 Rockchip USB 设备模式，并区分“已枚举”为 USB 设备与“已能安全烧录”的不同含义。

## 环境与前置条件

- **用户提供**：开发板为 youyeetoo R1（RK3588S，4 GB RAM，32 GB eMMC）。
- **用户提供**：主机为 Arch Linux。
- **用户提供**：`lsusb` 中出现 `ID 2207:350b`。
- **用户提供**：当前仅有一条 Type-C 数据线；该线已使 R1 枚举为 Rockchip USB 设备。
- Type-C 实物接口标签与 PCB 版本尚未记录。

## 步骤与证据

### 步骤 1：确认工具位置

执行端：Arch Linux 主机；当前目录：`~`。

```bash
command -v rkdeveloptool
```

实际输出：

```text
/usr/bin/rkdeveloptool
```

### 步骤 2：枚举 Rockchip 设备模式

执行端：Arch Linux 主机；当前目录：`~`。

```bash
rkdeveloptool ld
```

实际输出：

```text
DevNo=1	Vid=0x2207,Pid=0x350b,LocationID=106	Maskrom
```

退出码未单独记录。命令输出由学习者在终端中提供。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 主机是否有 `rkdeveloptool` | 有可执行文件或明确未安装 | `/usr/bin/rkdeveloptool` | 通过 |
| 设备模式 | `Maskrom` 或 `Loader` | `Maskrom` | 已确认 |

## 早期枚举证据

```text
ID 2207:350b
```

这是学习者提供的部分输出，不是完整的原始 `lsusb` 行。

## 结论与边界

- **已验证**：`rkdeveloptool` 将该 R1 识别为 `Maskrom`。MaskROM 是 SoC BootROM 提供的恢复/下载入口，不是“正在烧录”的过程本身。
- **推测**：原系统位于已移除的 TF 卡，板载 eMMC 没有可用启动链，因而 BootROM 回退到 MaskROM。
- 该实验不能区分 eMMC 为空、启动链损坏、强制进入 MaskROM 或其他启动失败原因；也没有读取或写入 eMMC。

## 下一步

建立 R1 的 3.3 V 串口调试条件，作为今后观察 U-Boot 和 Linux 启动日志的主要通道；在此之前不向 eMMC 写入镜像。

## 参考资料

- youyeetoo [R1 SBC 产品页](https://wiki.youyeetoo.cn/r1)，访问于 2026-08-05。
- MNT [RK3588 MaskROM 技术说明](https://community.mnt.re/t/rcore-rk3588-tech-note-entering-maskrom-usb-bootloader-mode/4191)，访问于 2026-08-05；该文示例中 `2207:350b` 为 MaskROM 枚举。
