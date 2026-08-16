---
title: "EXP-20260812-003 检查运行内核源码入口"
type: experiment
status: verified
created: 2026-08-12
updated: 2026-08-12
tags: [rk3588, r1, linux, kernel, driver, source]
related:
  - "[[experiment/exp-20260812-001-locate-running-kernel-config]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[status/current]]"
---

# EXP-20260812-003 检查运行内核源码入口

## 目标

确认 R1 当前运行的厂商 Linux 是否安装可直接阅读的同版本内核 headers 或 source，以决定后续驱动学习从本机树还是外部对应源码开始。

## 环境与前置条件

- 执行端：R1 目标 Linux 的 root Shell。
- 当前内核配置已可从 `/proc/config.gz` 读取；见[EXP-20260812-001](exp-20260812-001-locate-running-kernel-config.md)。
- 本实验不编译模块，也不加载、卸载或修改驱动。

## 风险与恢复

- 影响范围：仅读取内核版本和两个常见目录路径。
- 备份：不需要。
- 恢复方法：不修改系统，无恢复操作。

## 步骤与证据

### 步骤 1：检查同版本 build/source 链接

目的：固定运行内核版本，并检查发行镜像是否在 `/lib/modules/<版本>/` 中提供构建目录或源码目录。预期目录存在或报告不存在；任一种输出都不代表内核故障。

```sh
uname -r
ls -ld /lib/modules/$(uname -r)/build /lib/modules/$(uname -r)/source 2>&1
```

实际输出（学习者提供）：

```text
5.10.110
ls: cannot access '/lib/modules/5.10.110/build': No such file or directory
ls: cannot access '/lib/modules/5.10.110/source': No such file or directory
```

观察：当前运行内核版本为 `5.10.110`，其常见本机 headers/build 入口和 source 入口均不存在。因此不能假设目标镜像中有完整、可直接阅读或可用于外部模块构建的同版本内核树。这不否定 `/proc/config.gz` 中的配置真实性，也不说明当前内核无法运行；它只限制了当前镜像能提供的源码级学习入口。


### 步骤 2：检查同版本 modules 目录

目的：确认是否存在模块文件和依赖元数据，即使 `build`/`source` 缺失时也可能仍保留这些内容。

```sh
ls -la /lib/modules/$(uname -r) 2>&1
```

实际输出（学习者提供）：

```text
ls: cannot access '/lib/modules/5.10.110': No such file or directory
```

观察：同版本 modules 目录整体不存在。因此此镜像既没有 `build`/`source`，也没有模块文件或模块依赖元数据；已观察到的 GMAC、PHY 和 GPIO 支持应以内建代码或其他非模块机制提供。这个结论与先前 `/proc/config.gz` 中相关选项均为 `=y` 相容，但不能据此推断所有驱动均内建。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 运行内核版本 | 输出版本字符串 | `5.10.110` | 已确认 |
| `build` 入口 | 存在或不存在 | 不存在 | 本机 headers/build 入口缺失 |
| `source` 入口 | 存在或不存在 | 不存在 | 本机源码入口缺失 |
| modules 目录 | 存在或不存在 | 不存在 | 本机没有模块文件或模块元数据入口 |

## 结论

当前厂商 Ubuntu 镜像未提供与运行内核 `5.10.110` 对应的 build、source 或 modules 目录。后续源码级阅读和模块构建不能直接依赖目标机文件系统；应取得能够对应当前镜像的厂商内核源码，或明确把上游源码只作为通用学习材料。刚取得的 `R1/` 官方文档索引仓库不含内核源码，但提供 Ubuntu 源码编译入口链接，见[资料档案](../resource/youyeetoo-r1-documentation-repository.md)。

## 关联知识与问题

- 支持的知识点：运行内核配置、设备树与驱动绑定可从运行时证据建立，但源码级理解还需匹配的源码树。
- 关联问题：无。

## 后续行动

- [x] 检查运行版本及常见 `build`/`source` 入口。
- [x] 检查 `/lib/modules/5.10.110`；目录整体不存在。
