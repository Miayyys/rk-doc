---
title: "ISSUE-20260809-003 R1 未从 Arch 主机共享 DHCP 获得租约"
type: issue
status: resolved
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, network, dhcp, networkmanager]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[environment/hardware]]"
  - "[[status/current]]"
---

# ISSUE-20260809-003 R1 未从 Arch 主机共享 DHCP 获得租约

## 现象与影响

- 首次发现时间：未知；于 2026-08-09T15:59:08+08:00 记录为待排查状态；于 2026-08-09T16:20:22+08:00 回归验证通过。
- 可观察现象：Arch 主机 `enp108s0` 为 `10.42.0.1/24`，R1 `eth0` 为 `169.254.80.143/16`，R1 在 DHCP 尝试后回到 `disconnected`。
- 影响范围：R1 不能自动取得主机共享网段地址，无法通过该直连链路稳定传输文件或访问主机网络。
- 发生频率：修复前已观察到至少一次 DHCP 尝试超时；最小放行后已成功获得租约。

## 环境与复现

- 环境基线：[硬件环境](../environment/hardware.md)。网线直连 Arch 主机 `enp108s0`，主机通过 `wlo1` 连接上游网络。
- 最近变更：主机直连口从 `manual` 的 `192.168.0.1/24` 改为共享后的 `10.42.0.1/24`；完整配置命令输出未保留。
- 最小复现步骤：R1 的 `Wired connection 1`（`ipv4.method=auto`）执行激活并等待 DHCP 完成。
- 预期结果：R1 从主机的共享 DHCP 获得 `10.42.0.0/24` 中的地址。
- 实际结果：R1 出现 `169.254.80.143/16` 链路本地地址，随后 `nmcli device status` 显示 `eth0` 为 `disconnected`。

## 原始证据

```text
# Arch 主机
enp108s0         UP             10.42.0.1/24 fe80::abf:b8ff:fec2:8a1b/64
UNCONN 0 0 10.42.0.1:53 0.0.0.0:* users:(("dnsmasq",pid=94100,fd=6))
UNCONN 0 0 0.0.0.0:67 0.0.0.0:* users:(("dnsmasq",pid=94100,fd=4))

# R1
eth0             UP             169.254.80.143/16 fe80::122e:5ffa:f228:b2ce/64
eth0    ethernet  disconnected  --
```

完整命令上下文见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

主机 UDP 67/68 原始抓包见[host-enp108s0-dhcp-capture-20260809.txt](../_assets/host-enp108s0-dhcp-capture-20260809.txt)。其中同一 R1 MAC `1e:a8:e4:78:ee:77` 多次发送 DHCP Discover；截至保存的抓包末尾，没有捕获到从 UDP 67 发出的 DHCPOFFER。

## 关联知识与实验

- 相关知识点：DHCP、IPv4 链路本地地址、NetworkManager shared 模式。
- 验证实验：[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。已验证临时静态地址 `192.168.0.2/24` 可 ping 主机 `192.168.0.1`，故物理链路和同网段 IPv4 通信正常。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：主机未启动 DHCP 服务 | R1 未获 DHCP 地址 | 检查 UDP 67 监听 | `dnsmasq` 监听 UDP 67 | 排除 |
| H2：R1 的 DHCP 请求没有到达主机 | 服务已监听但未获租约 | 主机抓取 UDP 67/68，同时触发 R1 重连 | 多个 Discover 已到达主机 | 排除 |
| H3：dnsmasq 缺少 DHCP 范围或共享地址配置 | Discover 已到达且未见 Offer | 检查 dnsmasq 启动参数和共享配置 | 已有 `--listen-address=10.42.0.1` 与 `--dhcp-range=10.42.0.10,10.42.0.254,3600` | 排除 |
| H4：dnsmasq 虽有正确范围但未发 Offer | Discover 已到达且未见 Offer | 检查防火墙和 dnsmasq 日志 | UFW 先丢弃 UDP 67，尚不单独归因于 dnsmasq | 排除 |
| H5：主机防火墙在本机 socket 前丢弃 DHCP 报文 | tcpdump 可见入站 Discover，但服务未回 Offer | 检查防火墙规则与计数器 | UFW 的 UDP 67 规则计数为 191/60074，随后跳转至 `drop` | 确认 |

## 根因

**已确认根因**：Arch 主机 UFW 的 IPv4 `INPUT` 默认策略为 `drop`。`ufw-after-input` 中 `udp dport 67` 规则已命中 191 个、60074 字节的数据包，并跳转到 `ufw-skip-to-policy-input` 的 `drop`。R1 的 DHCP Discover 因而在到达 `dnsmasq` 前被防火墙丢弃。原有用户放行规则仅适用于旧网段 `192.168.0.0/24`，不覆盖新的共享网段或 DHCP 的源地址 `0.0.0.0`。

## 解决或绕过方法

- 临时绕过（已验证）：主机 `192.168.0.1/24` 与 R1 运行时 `192.168.0.2/24` 可直接 ping；此方案不提供自动地址分配，且网段会与随身 Wi-Fi 上游冲突。
- 已验证修复：在 `enp108s0` 入站放行 DHCP 服务器端口 UDP 67。学习者未保留 UFW 新增规则的标准输出，但其后 R1 从 `10.42.0.10–254` 地址池获得 `10.42.0.192/24`，构成回归证据。因初始 Discover 源地址为 `0.0.0.0`，规则应按接口限制，不能仅按 `10.42.0.0/24` 限制源地址。
- 后续单独处理：DHCP 成功后，再按实际需要配置 DNS（UDP/TCP 53）和主机到 `wlo1` 的转发规则；不要在尚未验证 DHCP 时一次放宽全部防火墙。

## 回归验证

已完成 DHCP 与主机直连回归验证：R1 `eth0` 自动获得 `10.42.0.192/24`，并收到主机 `10.42.0.1` 的三次 ICMP 回复。用户未保留 ping 汇总统计，故只记录观察到的三次成功回复。DNS 与外网转发不属于本问题的已完成范围，仍需单独验证。

## 经验与后续行动

- 可复用的排障方法：按物理链路 → 地址 → 网络管理状态 → 服务监听 → 报文序列逐层定位 DHCP。
- [x] 在主机抓取 UDP 67/68，同时仅触发一次 R1 `eth0` 重连；已保存 Discover 证据，未见 Offer。
- [x] 读取 `dnsmasq` 的启动参数与共享配置，确认共享地址和 DHCP 范围。
- [x] 检查主机防火墙规则与计数器；确认 UDP 67 Discover 被 UFW 丢弃。
- [x] 仅放行 `enp108s0` 入站 UDP 67 后，R1 自动获得共享网段租约并可 ping 主机。
