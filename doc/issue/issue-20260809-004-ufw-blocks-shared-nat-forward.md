---
title: "ISSUE-20260809-004 UFW 阻止 R1 共享 NAT 转发"
type: issue
status: archived
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, network, nat, ufw, networkmanager]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[issue/issue-20260809-003-r1-dhcp-lease-missing]]"
  - "[[status/current]]"
---

# ISSUE-20260809-004 UFW 阻止 R1 共享 NAT 转发

## 现象与影响

- 首次发现时间：未知；于 2026-08-09T16:24:55+08:00 记录为待排查状态。
- 可观察现象：R1 已获得 `10.42.0.192/24` 并能 ping 主机 `10.42.0.1`，但 ping `1.1.1.1` 为 3/3 超时。
- 影响范围：R1 无法经 Arch 主机 `wlo1` 使用共享 NAT 访问外部 IPv4。
- 发生频率：当前已观察一次完整的 3 包测试失败。

## 环境与复现

- 环境基线：[硬件环境](../environment/hardware.md)。R1 经 `enp108s0` 直连 Arch 主机，主机 `wlo1` 为上游网络接口。
- 最近变更：主机已启用 NetworkManager shared，R1 已获得 DHCP 租约；DHCP 入站放行见 [ISSUE-20260809-003](issue-20260809-003-r1-dhcp-lease-missing.md)。
- 最小复现步骤：在 R1 执行 `ping -c 3 1.1.1.1`。
- 预期结果：经主机 NAT 收到 3 次 ICMP 回复。
- 实际结果：3/3 超时；学习者报告主机能 ping 百度 IP，具体主机命令和输出未保留。

## 原始证据

```text
# R1
3 packets transmitted, 0 received, 100% packet loss, time 2025ms

# Arch 主机
chain FORWARD {
    type filter hook forward priority filter; policy drop;
    counter packets 91 bytes 7644 jump ufw-before-logging-forward
    counter packets 91 bytes 7644 jump ufw-before-forward
}
```

完整实验上下文见[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 关联知识与实验

- 相关知识点：Linux 转发、NAT、UFW `route allow`、NetworkManager shared。
- 验证实验：[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：主机上游网络不可用 | R1 外部 ping 超时 | 由学习者验证主机 ping 外部 IP | 学习者报告主机可 ping 百度 IP | 排除（证据未完整保存） |
| H2：R1 外网流量实际从 `wlo1` 出站 | 当前规则显式指定 `wlo1` | 检查 UFW 新规则计数与内核路由查询 | 新规则计数为 0 | 排除（出口待确认） |
| H3：R1 流量走主机其他出口（例如 `Meta`） | 主机存在 `Meta` 隧道接口 | `ip route get 1.1.1.1` | 待验证 | 保留 |

## 根因

**已确认的阻断点**：UFW IPv4 `FORWARD` 链默认策略为 `drop`，且 R1 外网测试后该链计数为 91 个包、7644 字节；未见任何放行规则。该链位于 R1 到主机上游网络的必经路径。

最终外网转发根因与修复尚未完成。学习者于 2026-08-09T16:35:29+08:00 决定当前只需要主机—板子直连通信，不需要共享 NAT 外网；因此本问题从当前学习范围归档。后续需要共享 NAT 时应重新激活并从“实际转发出口”继续验证，不应把本记录当作已解决。

## 解决或绕过方法

- 已尝试但不匹配的修复：学习者执行 `sudo ufw route allow in on enp108s0 out on wlo1 from 10.42.0.0/24`，UFW 返回 `Rule added`；本次测试后该规则计数为 0，说明出口接口条件未匹配。
- 不扩大为全局关闭 UFW 或默认允许转发。

## 回归验证

未完成，且当前不在范围内。后续需要共享 NAT 时，先查询转发数据包的实际出口，再应用最小 UFW 转发规则；成功后再单独验证 DNS。

## 经验与后续行动

- 可复用的排障方法：共享网络分别检查 DHCP 的 `INPUT` 路径与客户端访问外网的 `FORWARD` 路径。
- [x] 检查新增 `enp108s0` → `wlo1` UFW 路由规则；计数为 0，未命中。
- [ ] （已延期）查询转发数据包的实际出口并完成外部 IPv4 回归。
