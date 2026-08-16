---
title: "ISSUE-20260815-001 RKLLM demo 目标用户空间 ABI 不匹配"
type: issue
status: resolved
created: 2026-08-15
updated: 2026-08-15
tags: [rk3588, rkllm, cross-compile, abi, glibc]
related:
  - "[[status/current]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
  - "[[resource/airockchip-rknn-llm]]"
---

# ISSUE-20260815-001 RKLLM demo 目标用户空间 ABI 不匹配

## 现象与影响

- 首次发现时间：未知。
- 可观察现象：已上传的 `llm_demo` 在 R1 由动态加载器拒绝，尚未进入程序参数检查、RKLLM 初始化或 NPU 调用。
- 影响范围：当前 Arch GCC 16 交叉编译产物不能在 R1 Ubuntu 22.04 运行；上传的独立候选目录不影响系统文件。
- 发生频率：首次复现。

## 环境与复现

- 环境基线链接：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- 最近变更：以 Arch `aarch64-linux-gnu-g++` GCC 16.1.0 构建并上传 `llm_demo` 与 SDK Runtime；两端 SHA-256 一致。
- 最小复现步骤：在 R1 的 `/userdata/rkllm-api-demo` 执行 `LD_LIBRARY_PATH=./lib ./llm_demo`，故意不传模型参数。
- 预期结果：动态加载成功后打印 Usage，并因参数不足退出 1。
- 实际结果：动态加载器在进入 `main()` 前报告 ABI 版本缺失。

## 原始证据

```text
./llm_demo: /lib/aarch64-linux-gnu/libc.so.6: version `GLIBC_2.38' not found (required by ./llm_demo)
./llm_demo: /lib/aarch64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.32' not found (required by ./llm_demo)
```

R1 已验证为 GLIBC 2.35；此前只核对了 `librkllmrt.so` 的 ABI 需求（最高 GLIBC 2.29、GLIBCXX 3.4.26），未核对由 GCC 16 构建的 executable 自身需求。

## 关联知识与实验

- 相关知识点：CPU 架构匹配不等于 Linux 用户空间 ABI 匹配；动态加载在 `main()` 之前解析 executable 与其依赖所需的符号版本。
- 验证实验：[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。

## 假设与验证

| 假设 | 依据 | 最小验证方法 | 结果 | 状态 |
| --- | --- | --- | --- | --- |
| H1：SDK Runtime 自身超过 R1 ABI | runtime 是新引入的专用库 | 读取其 `version-info` 并比较 R1 库版本 | Runtime 最高仅需 GLIBC 2.29 / GLIBCXX 3.4.26，R1 已满足 | 排除为当前直接失败点 |
| H2：GCC 16 交叉编译出的 `llm_demo` 使用了比 R1 更新的用户空间 ABI | loader 错误直接点名 `./llm_demo` 所需 GLIBC 2.38 / GLIBCXX 3.4.32 | 以兼容目标 sysroot 或目标侧工具链重新构建并重试无参数启动 | 待验证 | 当前根因 |
| H3：RKNPU 0.8.2 与 RKLLM Runtime 不兼容 | 驱动版本未有官方配对说明 | 需先越过 loader ABI 阻塞，再观察 `rkllm_init()` 行为 | 尚未达到该阶段 | 保留 |

## 根因

当前失败的直接根因已确认：`llm_demo` 的构建工具链/默认用户空间 ABI 高于 R1 Ubuntu 22.04 所提供版本。AArch64 架构正确、专用 Runtime 的直接 ABI 也满足，仍不足以保证 executable 可被目标动态加载器启动。

## 解决或绕过方法

待验证的正确方向：使用与 R1 glibc 2.35、其 libstdc++ ABI 相容的目标 sysroot 或在 R1 可用的本机构建环境重新构建 demo。2026-08-15 已确认 R1 具有 `/usr/bin/cmake`、`/usr/bin/make`、`/usr/bin/gcc` 与 `/usr/bin/g++`，故优先尝试目标侧重编译。不要通过替换 R1 的 glibc、libstdc++ 或升级系统基础库绕过；这会影响整套镜像且不在本项目当前范围。

## 回归验证

将 `llm_demo.cpp` 与 `rkllm.h` 上传至已有独立目录后，R1 使用本机 `g++ -std=c++11 -I. llm_demo.cpp -L./lib -lrkllmrt -o llm_demo-r1` 生成新 binary。`file` 将其识别为 AArch64 动态 PIE；在同一目录以 `LD_LIBRARY_PATH=./lib ./llm_demo-r1` 运行，实际输出：

```text
Usage: ./llm_demo-r1 model_path max_new_tokens max_context_len
```

这证明目标加载器已加载 `librkllmrt.so` 并进入 `main()`；无参数的退出非零符合 demo 设计。原交叉编译二进制 `llm_demo` 保留在同一隔离目录作失败证据，未替换系统文件。问题标记为 `resolved`。

## 经验与后续行动

- 可复用的排障方法：交叉编译后分别检查 runtime 和 executable 的 ELF version requirements，并在目标机做无参数动态加载测试。
- [x] 使用 R1 原生 `g++` 重建并验证无参数加载。
