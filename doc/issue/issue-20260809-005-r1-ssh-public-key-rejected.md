---
title: "ISSUE-20260809-005 R1 SSH 公钥认证被拒绝"
type: issue
status: resolved
created: 2026-08-09
updated: 2026-08-09
tags: [rk3588, r1, ssh, networking]
related:
  - "[[experiment/exp-20260807-002-boot-linux-via-debug-uart]]"
  - "[[status/current]]"
---

# ISSUE-20260809-005 R1 SSH 公钥认证被拒绝

## 现象与影响

- 首次发现时间：未知。
- 可观察现象：主机以 root 身份连接 `10.42.0.192` 时，SSH 返回 `Permission denied (publickey,password)`。
- 影响范围：不能经 SSH/SCP 传输运行时 FDT；仍可使用 Debug UART 操作板端。
- 发生频率：两次不同客户端身份选择方式均复现。

## 环境与复现

- 环境基线：R1 Ubuntu 22.04、`ssh` 服务 active；主机与 R1 位于 `10.42.0.0/24` 直连网段。
- 最近变更：在 R1 `/root/.ssh/authorized_keys` 新增主机 Ed25519 公钥，权限为目录 700、文件 600。
- 最小复现：主机执行 `ssh -i ~/.ssh/id_ed25519 -o IdentitiesOnly=yes -o ConnectTimeout=5 -o PasswordAuthentication=no root@10.42.0.192 'id -un; hostname'`。
- 预期结果：输出 `root` 与 `R1`。
- 实际结果：服务器拒绝认证。

## 原始证据

```text
root@10.42.0.192: Permission denied (publickey,password).
```

两端 `ssh-keygen -lf` 均报告同一 Ed25519 SHA-256 指纹；完整证据见[实验](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 关联知识与实验

- 相关知识点：SSH 客户端身份选择、`AuthorizedKeysFile` 与 SSH 服务端严格权限检查。
- 验证实验：[EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：客户端选错私钥 | 默认客户端连接被拒绝 | 用 `-i` 与 `IdentitiesOnly=yes` 显式选择私钥重试 | 仍被拒绝 | 排除 |
| H2：板端公钥不是主机私钥对应公钥 | 认证被拒绝 | 比较双方 `ssh-keygen -lf` 指纹 | 指纹完全相同 | 排除 |
| H3：sshd 不读取默认授权路径 | 公钥正确且客户端已明确提供 | 在 R1 查询 `sshd -T` 的 `authorizedkeysfile` | 输出包含 `.ssh/authorized_keys`，对 root 即目标文件 | 排除 |
| H4：路径上存在不安全所有权 | sshd 可能因 StrictModes 拒绝密钥文件 | 列出路径属主并读取 `strictmodes` | `/root` 为 700 但属 `youyeetoo:youyeetoo`，且 `strictmodes yes` | 确认 |

## 根因

`/root` 的所有者为 `youyeetoo:youyeetoo`，而非 `root:root`，而 sshd 的有效配置为 `strictmodes yes`。sshd 因而不信任该目录下 root 的授权公钥路径，即使 `.ssh`、`authorized_keys` 和密钥内容本身均正确。

## 解决或绕过方法

已执行修复：`chown root:root /root` 将 root 家目录的属主恢复为 root，复查显示模式仍为 700。未递归变更内容、未关闭 `StrictModes`、未重启 sshd。

## 回归验证

主机执行 `ssh -o ConnectTimeout=5 -o PreferredAuthentications=publickey -o PasswordAuthentication=no root@10.42.0.192 'id -un; hostname'`，输出：

```text
root
R1
```

该命令与修复前的失败命令相同，修复后成功，回归通过。

## 2026-08-15 镜像更新后的复发

为上传 RKLLM 候选包而从主机测试 SSH 时，客户端改为请求密码。经 Debug UART 只读检查确认：`/root/.ssh/authorized_keys` 不存在，sshd 仍为 `permitrootlogin yes`、`pubkeyauthentication yes`、`strictmodes yes`，且 `/root` 的实际模式/属主为 `700 youyeetoo:youyeetoo`。主机待授权的 Ed25519 公钥指纹为 `SHA256:3MXA9RlxfRuO7mouBBDWxc3qh777QKVbH+6CnO1OTN0`。

这与先前 H4 根因完全相同，但发生在镜像更新后，是新的配置状态而非旧修复的回归失败。问题重新设为 `active`，待完成最小修复和 SSH 回归验证。

随后学习者将主机 `~/.ssh/id_ed25519.pub` 的完整 OpenSSH 公钥写入新的 `/root/.ssh/authorized_keys`，并设置授权文件模式为 600、相关路径属主为 `root:root`。首次 Base64 传输所得文件不被 `ssh-keygen` 识别为公钥，未作为有效授权状态；改为直接写入以 `ssh-ed25519` 开头的单行公钥后，主机执行以下回归验证成功：

```text
root
R1
target-absent
```

该命令使用 `-i ~/.ssh/id_ed25519 -o IdentitiesOnly=yes -o PasswordAuthentication=no` 连接 `root@10.42.0.193`，证明公钥认证恢复且候选上传目标仍为空。问题恢复为 `resolved`。

## 经验与后续行动

- 可复用的排障方法：先分别验证客户端私钥选择、公钥内容对应关系、服务端授权路径，再读取服务端日志，不直接改认证配置。
- 可复用的结论：`authorized_keys` 的文件本身权限正确不足以保证登录；`StrictModes` 还会检查其父目录，尤其是账户家目录的属主。
