---
title: "ISSUE-20260810-001 R1 systemd degraded 与失败单元"
type: issue
status: active
created: 2026-08-10
updated: 2026-08-15
tags: [rk3588, linux, systemd]
related:
  - "[[experiment/exp-20260810-001-identify-linux-pid-1]]"
  - "[[experiment/exp-20260814-001-inspect-r1-npu-first-boot-script]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
  - "[[status/current]]"
---

# ISSUE-20260810-001 R1 systemd degraded 与失败单元

## 现象与影响

- 首次发现时间：未知。
- 可观察现象：更新前的失败列表含 `apport-autoreport.service` 与 `rockchip.service`；学习者报告更新后，`systemctl is-system-running` 仍输出 `degraded`，失败列表只剩 `rockchip.service`。
- 影响范围：具体功能影响未知。当前可通过 UART 获取 root Shell，不能仅凭 `degraded` 判断系统整体不可用。
- 发生频率：当前启动后的单次观察；重启后是否复现未验证。

## 环境与复现

- 环境基线链接：[当前状态](../status/current.md)。
- 最近变更：本轮仅执行 `ps`、`readlink`、`ls` 与 `systemctl` 等只读命令；没有启停服务、修改配置或烧录。
- 最小复现步骤：在 R1 Linux Shell 执行 `systemctl --failed --no-pager`。
- 预期结果：列出当前失败单元，或显示没有失败单元。
- 实际结果：更新前列出两个失败服务；更新后列出一个失败服务。

## 原始证据

```text
  UNIT                      LOAD   ACTIVE SUB    DESCRIPTION
● apport-autoreport.service loaded failed failed Process error reports when aut…
● rockchip.service          loaded failed failed Setup rockchip platform enviro…

LOAD   = Reflects whether the unit definition was properly loaded.
ACTIVE = The high-level unit activation state, i.e. generalization of SUB.
SUB    = The low-level unit activation state, values depend on unit type.
2 loaded units listed.
```

更新后（2026-08-15，学习者终端输出）：

```text
  UNIT             LOAD   ACTIVE SUB    DESCRIPTION
● rockchip.service loaded failed failed Setup rockchip platform environment

1 loaded units listed.
```

本次结果证明当前 `degraded` 由 `rockchip.service` 至少直接贡献。`apport-autoreport.service` 当前未出现在失败列表，但尚未检查其是否被移除、禁用或成功运行，不能称为“已解决”。

## 关联知识与实验

- 相关知识点：systemd 的 `degraded` 是失败单元的汇总状态，不等同于内核或全部用户空间停止。
- 验证实验：[EXP-20260810-001](../experiment/exp-20260810-001-identify-linux-pid-1.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：`rockchip.service` 失败与厂商平台环境有关 | 单元名称及描述含 Rockchip platform environment | 只读查看该服务状态和最近日志 | 待验证 | 保留 |
| H2：`apport-autoreport.service` 是非核心错误报告服务，失败不影响当前核心启动 | 单元描述为 Process error reports | 只读查看该服务状态和依赖关系 | 当前未列为失败，未继续追踪 | 归档 |
| H3：两个失败单元有共同根因 | 更新前同一失败列表有两个单元 | 比较各自状态、日志和依赖 | 当前失败列表已不再同时包含二者 | 否定 |

## 根因

**直接失败原因已确认**：`rockchip.service` 执行 `/etc/init.d/rockchip.sh` 后以状态 2 退出；服务日志显示脚本尝试解包 `/rknpu2-rk3588-*.tar`，`tar` 未能打开该路径并退出。

**底层原因未知**：该 tar 文件为何不存在或未匹配、它是否为当前系统预期的厂商载荷，以及其与 NPU 运行时的实际关系，均未验证。

## 解决或绕过方法

未修改脚本、未添加 tar 文件、未重启服务。因 NPU LLM 已成为近期项目核心，本问题于 2026-08-14 重新激活；仍先以只读检查缩小范围。

## 回归验证

未开始；在根因确认并实施改动前不适用。

## 经验与后续行动

- 可复用的排障方法：先用 `systemctl --failed` 将汇总状态拆成具体单元，再一次只检查一个单元的状态与日志。
- 重新激活原因：当前 NPU LLM 项目必须验证厂商运行时是否可用，`rockchip.service` 的 NPU tar 路径成为直接排查对象。
- [ ] 读取更新后 `rockchip.service` 的完整失败状态和最近日志，确认它仍由哪个命令、以什么退出码失败。
