---
title: "EXP-20260814-001 检查 R1 NPU 首次配置脚本线索"
type: experiment
status: active
created: 2026-08-14
updated: 2026-08-14
tags: [rk3588, npu, rknpu, systemd]
related:
  - "[[status/current]]"
  - "[[issue/issue-20260810-001-systemd-degraded-failed-units]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
---

# EXP-20260814-001 检查 R1 NPU 首次配置脚本线索

## 目标

确认 `rockchip.service` 所执行脚本中，RK3588 NPU tar 载荷的直接调用位置及其最小上下文；不下载、解包或启动任何厂商组件。

## 环境与前置条件

- 执行端：R1 目标 Linux 的 root Shell。
- 硬件及版本：youyeetoo R1 V2；RK3588S；Ubuntu 22.04 / Linux 5.10.110。
- 连接方式：Debug UART 或已验证的 SSH。
- 操作前状态：`rockchip.service` 已知失败，直接日志指向 `/rknpu2-rk3588-*.tar`。

## 风险与恢复

- 影响范围：仅读取 `/etc/init.d/rockchip.sh`，不改变文件、服务或 NPU 状态。
- 备份：不需要。
- 恢复方法：不需要。

## 步骤与证据

### 步骤 1：定位 NPU 与 tar 相关代码

目的：确定 RK3588 分支是否直接引用 NPU tar 文件，并保留命中行的局部上下文。预期找到型号分支、解包命令及清理逻辑；是否只在首次启动运行须由后续读取完整条件块确认。

```sh
# R1 目标 Linux root Shell
grep -n -C 5 -E 'rknpu|rkllm|rknn|tar' /etc/init.d/rockchip.sh
```

实际输出；退出码未记录：

```text
49-        rk3568|rk3566)
50-             MALI=bifrost-g52-g2p0
51-             ISP=rkaiq_rk3568
52-             sed -i "s/always/none/g" /etc/X11/xorg.conf.d/20-modesetting.conf
53-             sed -i "s/glamor/exa/g" /etc/X11/xorg.conf.d/20-modesetting.conf
54:             [ -e /usr/lib/aarch64-linux-gnu/ ] && tar xvf /rknpu2-rk3568-*.tar -C /
55-             [ -e /usr/lib/aarch64-linux-gnu/ ] && apt install -fy --allow-downgrades /camera_engine_$ISP*.deb
56-             ;;
57-        rk3588|rk3588s)
58-             ISP=rkaiq_rk3588
59-             MALI=valhall-g610-g6p0
60:             [ -e /usr/lib/aarch64-linux-gnu/ ] && tar xvf /rknpu2-rk3588-*.tar -C /
61-             [ -e /usr/lib/aarch64-linux-gnu/ ] && apt install -fy --allow-downgrades /camera_engine_$ISP*.deb
62-             ;;
63-    esac
64-
65-    apt install -fy --allow-downgrades /libmali-*$MALI*-x11*.deb
--
119-    then
120-       rm /dev/rfkill
121-    fi
122-
123-    rm -rf /*.deb
124:    rm -rf /*.tar
125-
126:    # The base target does not come with lightdm/rkaiq_3A
127-if [ -e /etc/gdm3/daemon.conf ]; then
128:    systemctl restart gdm3.service || true
129-elif [ -e /etc/lightdm/lightdm.conf ]; then
130:    systemctl restart lightdm.service || true
131-fi
132:    systemctl restart rkaiq_3A.service || true
133-
134-    touch /usr/local/first_boot_flag
135-fi
```

观察：RK3588/RK3588S 分支在目录 `/usr/lib/aarch64-linux-gnu/` 存在时，通过 `&&` 执行 `tar xvf /rknpu2-rk3588-*.tar -C /`。同一脚本还会删除根目录匹配的 `*.tar`，并在第 134 行创建 `first_boot_flag`。当前截取没有显示包围第 49–134 行的起始条件，因此不能据此断言 NPU tar 仅首次启动解包，也不能断言 tar 的删除就是其缺失根因。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| RK3588 分支直接引用 NPU 载荷 | 出现 RKNPU tar 路径或没有匹配 | 第 60 行引用 `/rknpu2-rk3588-*.tar` | 通过 |
| 解包前置条件可见 | 显示条件或无条件执行 | `/usr/lib/aarch64-linux-gnu/` 存在时执行 | 通过 |
| NPU tar 调用的外层生命周期条件 | 明确首次启动或每次启动 | 未包含外层 `if` 起始行 | 不确定 |
| 脚本的 tar 清理逻辑 | 出现或没有出现 | 第 124 行 `rm -rf /*.tar` | 通过；影响待确认 |

## 结论

**已验证**：`rockchip.sh` 的 RK3588/RK3588S 分支包含 NPU tar 解包命令；该命令以 `/usr/lib/aarch64-linux-gnu/` 存在为直接条件。脚本中还存在根目录 tar 清理和 `first_boot_flag` 创建。

**待验证**：NPU 解包代码是否由首次启动条件包围；tar 缺失究竟是镜像制作时遗漏、首次配置后清理、手动删除还是其他原因；解包后会提供哪些运行时文件；当前 NPU 驱动是否已经可用。

## 关联知识与问题

- 支持或修正的知识点：systemd 服务失败的直接命令不自动等于硬件驱动不可用；脚本条件和生命周期需按完整控制流确认。
- 关联问题：[ISSUE-20260810-001](../issue/issue-20260810-001-systemd-degraded-failed-units.md)。

## 后续行动

- [ ] 读取脚本第 20–140 行，确定 `rknpu2` 解包命令的外层控制条件与 `first_boot_flag` 的作用域。
