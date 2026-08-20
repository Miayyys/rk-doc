---
title: "EXP-20260820-002 检查 R1 实际 BL31 的 Rockchip SiP 入口"
type: experiment
status: active
created: 2026-08-20
updated: 2026-08-20
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

## 结论

当前 R1 的 p1 启动载荷确实包含一个带 Rockchip SiP 服务分发器的 BL31。AMP 启动责任链并未因“完全没有 SiP”而立即排除，但 `RK_SIP_AMP_CFG` 的具体支持状态仍未知；当前本机也没有可直接核验的匹配 TF-A/BL31 源码。不得仅凭通用 handler 名称向板端试探性调用 SMC。

## 后续行动

- [ ] 只读列出已有 `src/rkbin` 的 BL31 二进制，并以 SHA-256 与实际 `r1-atf-1.bin` 比对；若存在精确相同载荷，可取得其文件名和 Git 来源线索。继续不调用 SMC、不加载 AMP DTB、不写 eMMC。
