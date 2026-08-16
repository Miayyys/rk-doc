---
title: "airockchip rknn-llm 官方 SDK 候选"
type: resource
status: verified
created: 2026-08-15
updated: 2026-08-15
tags: [rk3588, rknpu, rkllm, llm, sdk]
related:
  - "[[status/current]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
---

# airockchip rknn-llm 官方 SDK 候选

## 身份与来源

- 发布组织：airockchip。
- 上游地址：<https://github.com/airockchip/rknn-llm>。
- 查阅日期：2026-08-15。
- 本地位置：Arch 主机 `src/rknn-llm/`。
- 获取方式：浅克隆上游 `main`；工作树干净，提交为 `878f9361fd3afa7e167b7079918918f78d2c1c2a`（`release v1.3.0`，作者时间 `2026-06-17T17:27:54+08:00`），`git rev-parse --is-shallow-repository` 输出 `true`。
- 许可证：仓库含 `LICENSE`，具体条款尚未阅读。

## 与项目的关系

上游 README 说明：主机端 `RKLLM-Toolkit` 将训练模型转换为 `.rkllm`，板端以 `RKLLM Runtime` 的 C/C++ API 在 Rockchip NPU 上推理；支持平台包含 RK3588。它是本项目“R1 NPU 实际运行 LLM”所需的候选软件栈，而不是普通 RKNN 图像模型工具的替代品。

README 当前标注最新版本为 `v1.3.0`，并提供 Linux aarch64 运行时、示例和经转换模型的下载入口。其仓库 `rknpu-driver/` 目录还包含 `rknpu_driver_0.9.8_20241009.tar.bz2`。本板当前实际驱动为 `0.8.2`（2022-08-29），二者版本不同；资料未在已查页面给出可直接适用于本板的最低驱动版本或兼容矩阵。因此不能直接把最新 runtime、模型或驱动视作当前系统可用组合。

## 适用范围与验证状态

- 已验证：上游明确支持 RK3588 和 `.rkllm` 模型格式。
- 已验证：Linux runtime 提供 `librkllm_api/aarch64/librkllmrt.so`、`armhf/librkllmrt.so` 与 `include/rkllm.h`。纯文本 `rkllm_api_demo` 提供 Linux/Android 构建脚本、CMake 配置、`llm_demo.cpp` 与模型导出脚本；具体 API、依赖和兼容要求尚未读取。
- 已验证：v1.3.0 demo 声明 Toolkit/Runtime 均至少为 1.3.0、Python 至少为 3.9；目标输入为 `.rkllm`。Linux CMake 会把 `librkllmrt.so` 与 `llm_demo` 一起安装，运行时通过 `LD_LIBRARY_PATH=./lib` 找库。
- 已验证：官方 `build-linux.sh` 硬编码特定的 `aarch64-none-linux-gnu` 工具链路径；它与本机已验证的 `aarch64-linux-gnu-gcc` 不同。后续应使用独立 build 目录和明确的现有编译器路径，不修改上游脚本。
- 已验证：主机有 `/usr/bin/cmake`、`aarch64-linux-gnu-g++` 和 `aarch64-linux-gnu-strip`；编译器报告 target triple `aarch64-linux-gnu`、版本 GCC 16.1.0。其与目标系统的运行时 ABI 相容性仍须由实际构建和板端运行验证。
- 已验证：AArch64 `librkllmrt.so` 的直接 ELF 依赖为 GNU/OpenMP/POSIX 标准库，不含 `librknnrt.so` 或 DRM/RKNPU 专用库；最高版本需求为 `GLIBC_2.29`、`GLIBCXX_3.4.26`、`CXXABI_1.3.11`。这不排除运行时按需加载额外组件。
- 已验证：本次源码同时含 `Readme.md`（1.2.x）和 `README.md`（>=1.3.0）两份 API demo 文档；后续只以固定 v1.3.0 commit 的 `README.md` 为依据。
- 已验证：上游 tag 实际使用 `release-v*` 前缀，而非 `v*`；远端列出 `release-v1.2.0`、`release-v1.2.1`、`release-v1.2.1b1`、`release-v1.2.2`、`release-v1.2.3` 等版本。`release-v1.2.1b1` 指向 `d8a9f6a9cce06922bf61ea9151d72fbf55dd55bb`，其名称与首次模型初始化日志的 Toolkit `1.2.1b1` 相同；但该 tag 的 DeepSeek demo 文档要求 `rkllm-toolkit==1.2.0`，因此 tag 名称不能证明该模型由此版本转换或与其 Runtime 兼容。
- 已验证：主机已将该 tag 以浅克隆固定到 `src/rknn-llm-release-v1.2.1b1/`；`git describe --tags --exact-match` 输出 `release-v1.2.1b1`，`git rev-parse HEAD` 输出 `d8a9f6a9cce06922bf61ea9151d72fbf55dd55bb`。
- 已验证（静态）：该 tag 的 AArch64 `librkllmrt.so` 为 stripped AArch64 shared object；可提取到 `0.9.7`、`Warning: Your rknpu driver version is too low`、`Current driver version ... recommend ... >=` 和 `Mismatch driver version ... requires driver version >= ... incompatible!` 等字符串，同时标识自身为 `rkllm-runtime version: 1.2.1b1`。这些是强烈的版本门槛线索，但未在 R1 上实际加载，不能单独替代运行时验证。
- 待验证：SDK 的 `runtime >= 1.3.0` 要求不自动推出内核 RKNPU 驱动最低版本；当前 R1 的 0.8.2 是否可用仍未知。
- 已验证：本板具备 RKNPU 内核驱动、`/dev/dri/renderD129` 和旧版 RKNN 运行时，但未发现 RKLLM runtime 或模型。
- 待确认：SDK 版本/commit、运行时和驱动配对要求、4 GB RAM 下的最小可演示模型、模型下载大小/校验、主机转换环境和板端部署步骤。

## 参考

- 上游 README：<https://github.com/airockchip/rknn-llm>。
- 上游驱动目录：<https://github.com/airockchip/rknn-llm/tree/main/rknpu-driver>。
