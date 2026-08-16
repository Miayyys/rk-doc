---
title: "ISSUE-20260811-001 上游 U-Boot 构建缺少 swig"
type: issue
status: resolved
created: 2026-08-11
updated: 2026-08-11
tags: [rk3588, uboot, build, host-tools]
related:
  - "[[experiment/exp-20260809-004-acquire-upstream-uboot-source]]"
  - "[[status/current]]"
---

# ISSUE-20260811-001 上游 U-Boot 构建缺少 swig

## 现象与影响

- 首次发现时间：未知。
- 可观察现象：安装 `swig` 后，构建已进入 `PYMOD rebuild`，但编译 `pylibfdt` 生成的包装 C 文件时继续失败，报 Python 2 风格 API 未声明。
- 影响范围：Arch 主机上的上游 U-Boot EVB 构建不能完成；未修改上游源码，未连接、写入或烧录 R1。
- 发生频率：仅复现一次。

## 环境与复现

- 环境基线链接：[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- 最近变更：已在隔离输出目录生成 `evb-rk3588_defconfig` 的 `.config`；第一次完整构建先因找不到 `swig` 停止，学习者安装该工具后再次构建。
- 最小复现步骤：在上游源码目录执行：

```fish
make O=/home/loser/Study/rk3588/build/uboot-evb-rk3588 CROSS_COMPILE=aarch64-linux-gnu- -j(nproc)
```

- 预期结果：主机工具和 AArch64 目标继续编译，产物仅写入隔离 `build/` 目录。
- 实际结果：主机侧 `pylibfdt` 重建阶段在使用 `/usr/include/python3.14` 编译 `libfdt_wrap.c` 时，`PyInt_AsLong` 与 `PyString_FromString` 未声明，make 退出。

## 原始证据

```text
HOSTLD  scripts/dtc/fdtoverlay
error: command 'swig' failed: No such file or directory
make[3]: *** [.../scripts/dtc/pylibfdt/Makefile:33：rebuild] 错误 1
make[2]: *** [.../scripts/Makefile.build:497：scripts/dtc/pylibfdt] 错误 2
make[1]: *** [.../Makefile:2403：scripts_dtc] 错误 2
make: *** [Makefile:189：__sub-make] 错误 2

PYMOD   rebuild
scripts/dtc/pylibfdt/libfdt_wrap.c:5618:20: error: implicit declaration of function ‘PyInt_AsLong’; did you mean ‘PyLong_AsLong’?
scripts/dtc/pylibfdt/libfdt_wrap.c:6679:19: error: implicit declaration of function ‘PyString_FromString’; did you mean ‘PyLong_FromString’?
... '-I/usr/include/python3.14' ... '-c', 'scripts/dtc/pylibfdt/libfdt_wrap.c' ... returned non-zero exit status 1.
```

## 关联知识与实验

- 相关知识点：`-j(nproc)` 只改变 make 的并行任务数；`scripts/dtc/pylibfdt` 属于主机构建工具，不是 R1 上运行的程序。`PyInt_*` 与 `PyString_*` 是 Python 2 API 名称，Python 3 使用不同 API。
- 验证实验：[EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：`swig` 不在主机 `PATH` 中 | 首次错误明确为找不到该命令 | 安装后重试已进入 `PYMOD rebuild` | 已不再是当前阻塞 | 确认（间接） |
| H2：当前配置要求构建 Python `pylibfdt` 绑定 | 失败路径为 `scripts/dtc/pylibfdt/Makefile` | 阅读 Makefile 的 `pymod` 命令 | `$(PYTHON3) setup.py ... build_ext --inplace` | 确认 |
| H3：旧 typemap 与 SWIG 4.5 生成行为组合后不兼容 Python 3.14 | 模板含 `PyInt_*`、`PyString_*`；编译器报未声明 | 检查生成文件中的兼容别名 | 未匹配到两个别名；Python 3.14.6、SWIG 4.5.0 | 确认 |
| H4：最小活跃修复范围为两处 | `PyString_AsString` 在 Python 3 条件分支的 `else` 中 | 盘点接口模板所有旧 API 及其上下文 | 两处无条件用法：`PyString_FromString`、`PyInt_AsLong`；一处仅 Python 2 分支 | 确认 |

## 根因

当前直接根因已确认：上游 `scripts/dtc/pylibfdt/libfdt.i_shipped` 无条件调用 `PyString_FromString` 和 `PyInt_AsLong`，SWIG 4.5.0 生成的 `libfdt_wrap.c` 未定义二者的兼容别名，而 Python 3.14.6 头文件也不提供它们。于是生成的 C 文件无法编译。模板中的 `PyString_AsString` 位于 Python 3 条件分支的 `else`，当前不参与编译。这个结论仅解释当前工具组合的失败；尚未证明换用旧 SWIG/Python 会成功，也未决定最终采用补丁、上游修复或隔离环境。

## 解决或绕过方法

已安装 `swig` 后，缺少命令的阻塞已越过。学习者已报告创建本地实验分支 `study/pylibfdt-py3-swig45`；未保存该操作的 `git status` 原始输出。学习者已在该分支应用并展示两行最小补丁：无条件 `PyString_FromString` 改为 `PyUnicode_FromString`，`PyInt_AsLong` 改为 `PyLong_AsLong`；展示的 diff 未含额外文件或行。尚未验证重建结果，也未替换 Python、跳过 `pylibfdt` 或改动开发板。

## 回归验证

已完成本问题的回归验证：应用两行补丁后，同一完整构建不再在 `pylibfdt` 报 `PyInt_AsLong` 或 `PyString_FromString` 未声明，而是继续进入 `BINMAN .binman_stamp`。构建随后因独立的外部启动载荷缺失停止，见[ISSUE-20260811-002](issue-20260811-002-uboot-missing-external-boot-blobs.md)。

## 经验与后续行动

- 可复用的排障方法：先按失败路径区分主机工具、交叉工具链和目标代码；再用一个只读命令验证错误中点名的可执行文件。
- [x] 使用同一隔离输出目录重新构建，确认两行补丁越过 `pylibfdt`；不提交、不烧录。
