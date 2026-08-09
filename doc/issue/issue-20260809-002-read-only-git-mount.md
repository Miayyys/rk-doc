---
title: "ISSUE-20260809-002 只读 .git 挂载阻止 Git 初始化"
type: issue
status: resolved
created: 2026-08-09
updated: 2026-08-09
tags: [git, workspace, environment]
related:
  - "[[status/current]]"
  - "[[environment/software]]"
---

# ISSUE-20260809-002 只读 `.git` 挂载阻止 Git 初始化

## 现象与影响

- 首次发现时间：未知；2026-08-09T13:38:37+08:00 完成记录。
- 可观察现象：仓库根目录的 `.git/` 为空目录，但 `git rev-parse` 不能识别为仓库，且无法删除后重建。
- 影响范围：当前目录无法完成标准 `git init`，因而不能进行首次提交或使用普通 Git 状态检查。
- 发生频率：首次检查时可稳定复现；当前已不能复现“不是 Git 仓库”的现象。

## 环境与复现

- 环境基线链接：[软件环境基线](../environment/software.md)。
- 最近变更：学习者请求在当前目录初始化 Git。
- 最小复现步骤：执行 `git rev-parse --is-inside-work-tree`，随后尝试 `rmdir .git`。
- 预期结果：空占位目录可移除，`git init` 创建可写的 Git 元数据。
- 实际结果：Git 报告“not a git repository”；`rmdir` 报告“Device or resource busy”。

## 原始证据

```text
$ findmnt -T .git -o TARGET,SOURCE,FSTYPE,OPTIONS
TARGET                        SOURCE FSTYPE OPTIONS
/home/loser/Study/rk3588/.git tmpfs  tmpfs  ro,nosuid,nodev,relatime,mode=555,uid=1000,gid=1000,inode64,huge=advise

$ mountpoint .git
.git is a mountpoint
```

命令由 Agent 执行；其余终端输出见本问题的复现步骤。未执行 `sudo umount`，以免未经授权改变工作区挂载状态。

## 关联知识与实验

- 相关知识点：Git 仓库的标准元数据目录为工作树根目录中的可写 `.git/`。
- 验证实验：本问题中的最小复现。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：`.git` 是只读挂载点，而非未完成初始化留下的普通目录 | `rmdir` 报“Device or resource busy” | 用 `findmnt -T .git` 和 `mountpoint .git` 检查 | `tmpfs`、`ro`，且确认是挂载点 | 确认 |
| H2：后续由外部环境向 Agent 暴露了有效 Git 元数据 | 挂载来源从空 `tmpfs` 变为宿主 Btrfs 路径 | 重新运行 `findmnt`、`git rev-parse` 和 `git status` | Git 已识别当前工作树；具体操作来源未知 | 确认 |

## 根因

首次检查时的直接阻塞原因已确认：`.git` 被挂载为权限 `555` 的只读空 `tmpfs`，不能被移除或写入。随后该挂载显示为宿主 Btrfs 中的 `.git` 路径，当前仓库已可被 Git 识别；挂载变化由谁执行、具体事件时间均未知。

## 解决或绕过方法

当前不再需要绕过：标准 Git 元数据已存在，当前项目已初始化。Agent 的普通只读检查不能写入 `.git`，但在学习者批准 Git 元数据写入后，已成功完成暂存和提交。

## 回归验证

2026-08-09T13:42:45+08:00 记录回归检查；事件实际发生时间未知：

```text
$ git rev-parse --is-inside-work-tree
true

$ git status --short --branch
## No commits yet on master
?? .gitignore
?? AGENTS.md
?? doc/
```

仓库识别通过，当前分支为 `master`，尚无提交，项目文件均为未跟踪状态。

后续于 2026-08-09T13:49:17+08:00 创建首次提交，并验证提交对象：

```text
2ed79a18c4054a87b86b7e1aa8bc62a26db3165d
docs: initialize RK3588 learning repository
```

首次提交包含 `.gitignore`、`AGENTS.md` 和 `doc/` 下 39 个必要文件；Obsidian 的个人工作区状态继续由 `.gitignore` 排除。

## 经验与后续行动

- 可复用的排障方法：遇到空目录却无法初始化时，依次检查 `git rev-parse`、`mountpoint`、`findmnt` 和权限，而不是强制删除。
- [x] 验证当前项目已经是有效 Git 工作树。
- [x] 检查忽略规则、提交内容和 Git 用户身份，并创建首次提交。
