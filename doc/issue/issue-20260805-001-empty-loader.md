---
title: "ISSUE-20260805-001 Loader 文件状态与来源不明"
type: issue
status: archived
created: 2026-08-05
updated: 2026-08-05
tags: [rk3588, loader]
related:
  - "[[status/current]]"
  - "[[resource/resource-index]]"
---

# ISSUE-20260805-001 Loader 文件状态与来源不明

## 现象与影响

- **已验证（首次观察）**：仓库根目录的 `rk3588_spl_loader.bin` 存在，大小为 0 字节；`file` 将其识别为 `empty`。
- **已验证（后续观察）**：同日校验时该路径已不存在，`stat` 返回 `No such file or directory`。
- **用户提供**：文件由学习者主动删除；具体删除时间未知。Agent 未执行恢复、创建或删除操作。
- 无论是空文件还是缺失状态，它都不能被视为可用或可烧录的 Loader。

## 原始证据

执行端：Arch Linux 主机；当前目录：仓库根目录。

```bash
file rk3588_spl_loader.bin
stat -c 'name=%n size=%s bytes' rk3588_spl_loader.bin
```

```text
rk3588_spl_loader.bin: empty
name=rk3588_spl_loader.bin size=0 bytes
```

两条检查命令退出码均为 0；这只表示检查成功，不表示 Loader 有效。

后续校验：

```bash
stat -c '%n %s bytes' rk3588_spl_loader.bin
```

```text
stat: cannot statx 'rk3588_spl_loader.bin': No such file or directory
```

此次 `stat` 退出码为 1。

## 假设与验证

| 假设 | 依据 | 验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| 文件曾是尚未填充的占位符 | 首次观察时大小为 0 | 询问来源并查找生成流程 | 待验证 | 保留 |
| 获取或复制过程失败 | 首次观察时大小为 0 | 与原始来源的大小和 SHA-256 对比 | 待验证 | 保留 |
| 文件由学习者主动删除 | 用户确认 | 用户说明 | 已确认 | 确认 |

## 根因

后续文件缺失的原因已确认：学习者主动删除。最初文件为何是 0 字节仍未知，但该文件不再使用，因此本问题归档而不标记为已解决。

## 安全限制与下一步

不要从不明来源随意补回文件。未来若重新引入 Loader，必须作为新的资源记录其来源、目标板型、预期大小和校验值。
