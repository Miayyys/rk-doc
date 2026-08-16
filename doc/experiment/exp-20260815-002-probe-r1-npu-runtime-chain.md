---
title: "EXP-20260815-002 探测 R1 NPU 运行时链路"
type: experiment
status: active
created: 2026-08-15
updated: 2026-08-16
tags: [rk3588, npu, rknpu, rknn, llm]
related:
  - "[[status/current]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
  - "[[issue/issue-20260810-001-systemd-degraded-failed-units]]"
  - "[[issue/issue-20260815-001-rkllm-demo-target-abi-mismatch]]"
  - "[[resource/deepseek-r1-distill-qwen-1-5b-w8a8-rk3588]]"
---

# EXP-20260815-002 探测 R1 NPU 运行时链路

## 目标

直接确认 NPU LLM 项目最低链路中的内核驱动、设备节点和用户态 Rockchip 运行时是否存在；不把失败的厂商服务当作预先必须解决的问题。

## 环境与前置条件

- 执行端：R1 Debug UART 的 root Shell。
- 硬件及版本：youyeetoo R1 V2，RK3588S，4 GB RAM、32 GB eMMC。
- 软件：Ubuntu 22.04，Linux `5.10.110 #4`。
- 操作前状态：系统可从 eMMC 启动；`rockchip.service` 失败，但尚未重新读取其完整日志。

## 风险与恢复

- 影响范围：只读读取内核环形缓冲、设备节点和文件名。
- 备份：不需要。
- 恢复方法：不需要。

## 步骤与证据

### 步骤 1：检查驱动、节点和用户态候选

目的：判断 NPU 链路是否至少具备内核端和 RKNN 用户态入口。预期驱动日志出现 RKNPU；设备接口可能是传统 `/dev/rknpu*` 或 DRM 节点；文件搜索可能找到 RKNN/RKLLM 运行时。

```sh
# R1 Debug UART 的 root Shell
printf '%s\n' '== NPU kernel log =='
dmesg | grep -Ei 'rknpu|rk-npu' | tail -n 50
printf '\n== NPU device node ==\n'
ls -l /dev/rknpu* 2>&1
printf '\n== RKNN / RKLLM runtime files ==\n'
find /usr/local /usr/lib /opt -type f \
  \( -iname '*rkllm*' -o -iname '*rknn*' -o -iname '*rknpu*' \) \
  2>/dev/null | head -n 100
```

实际输出；退出码未记录：

```text
== NPU kernel log ==
[    3.281437] RKNPU fdab0000.npu: Adding to iommu group 0
[    3.281593] RKNPU fdab0000.npu: RKNPU: rknpu iommu is enabled, using iommu mode
...
[    3.293809] [drm] Initialized rknpu 0.8.2 20220829 for fdab0000.npu on minor 1
...
[    3.325495] RKNPU fdab0000.npu: failed to find power_model node
[    3.325517] RKNPU fdab0000.npu: RKNPU: failed to initialize power model

== NPU device node ==
ls: cannot access '/dev/rknpu*': No such file or directory

== RKNN / RKLLM runtime files ==
/usr/lib/librknnrt.so
/usr/lib/systemd/system/rknn_server.service
```

观察：**已验证**内核识别并初始化 RKNPU，启用了 IOMMU，且以 DRM minor 1 注册。未找到传统 `/dev/rknpu*`，但该日志形式说明下一步应查看 `/dev/dri/`，不能把缺少该旧接口等同于 NPU 不可用。用户态已存在 `librknnrt.so` 和 `rknn_server.service`；尚未发现或验证 RKLLM 运行时、模型、示例程序或实际 NPU 推理。`power_model` 失败的影响尚未验证，当前不阻塞实际推理链的核查。

### 步骤 2：确认 DRM 节点与 RKNN 服务

目的：核查内核导出的 DRM 节点，以及镜像是否已实际启动厂商 RKNN 用户态服务。预期存在一个或多个 `renderD*` 节点；服务若存在，读取其状态与启动定义而不重启。

```sh
# R1 Debug UART 的 root Shell
printf '%s\n' '== DRM nodes =='
ls -l /dev/dri 2>&1
printf '\n== RKNN server state ==\n'
systemctl status rknn_server.service --no-pager -l
printf '\n== RKNN server definition ==\n'
systemctl cat rknn_server.service
```

实际输出；退出码未记录：

```text
== DRM nodes ==
card0  card1  renderD128  renderD129

== RKNN server state ==
● rknn_server.service - start rknn_server service
     Loaded: loaded (/lib/systemd/system/rknn_server.service; enabled; vendor preset: enabled)
     Active: active (running)
   Main PID: 622 (rknn_server)
...
start rknn server, version:1.3.0 (121b661 build: 2022-04-29 11:12:02)
I NPUTransfer: Starting NPU Transfer Server, Transfer version 2.1.0

== RKNN server definition ==
[Service]
Type=simple
Restart=always
RestartSec=1
ExecStart=rknn_server
```

观察：**已验证**系统导出了两个 DRM render 节点，且 `rknn_server` 已启用并处于 `active (running)`。服务版本/构建时间为 `1.3.0` / 2022-04-29，启动定义没有参数。仅凭服务运行不能证明它已完成一轮模型推理，也不能证明它支持 RKLLM；下一步须通过 sysfs 与进程文件描述符确认实际 RKNPU 节点。

### 步骤 3：映射 NPU 的 DRM 节点与空闲服务进程

目的：排除显示 DRM 节点与 NPU DRM 节点混用，并观察空闲的 `rknn_server` 是否已打开 NPU。预期一个 render 节点的 OF compatible 为 RKNPU；空闲服务可能没有 DRM 文件描述符。

```sh
# R1 Debug UART 的 root Shell
for node in /sys/class/drm/renderD*
do
    printf '\n== %s ==\n' "$(basename "$node")"
    readlink -f "$node/device/driver"
    cat "$node/device/uevent"
done

pid=$(systemctl show -p MainPID --value rknn_server.service)
readlink -f "/proc/$pid/exe"
ls -l "/proc/$pid/fd" | grep '/dev/dri' || true
```

实际输出；退出码未记录：

```text
== renderD128 ==
/sys/bus/platform/drivers/rockchip-drm
OF_NAME=display-subsystem
OF_COMPATIBLE_0=rockchip,display-subsystem

== renderD129 ==
/sys/bus/platform/drivers/RKNPU
DRIVER=RKNPU
OF_NAME=npu
OF_FULLNAME=/npu@fdab0000
OF_COMPATIBLE_0=rockchip,rk3588-rknpu

== rknn_server executable ==
/usr/bin/rknn_server

== rknn_server DRM file descriptors ==
```

观察：**已验证**`/dev/dri/renderD129` 是 RKNPU 设备的用户态入口，`renderD128` 是显示子系统。`rknn_server` 可执行文件为 `/usr/bin/rknn_server`，在本次空闲观察中未打开 `/dev/dri/*`；这与等待客户端请求相容，不能据此判定服务或 NPU 失败。

### 步骤 4：搜索本机模型与可执行示例

目的：优先复用镜像现有资产，完成一次实际 NPU 推理；只有资产缺失时才引入外部 SDK/模型。

```sh
# R1 Debug UART 的 root Shell
printf '%s\n' '== RKNN models =='
find /usr /opt /oem /userdata /root /home -type f -iname '*.rknn' 2>/dev/null
printf '\n== RKNN/RKLLM executables and demos ==\n'
find /usr /opt /oem /userdata /root /home -type f \
  \( -iname '*rknn*' -o -iname '*rkllm*' -o -iname '*demo*' \) \
  -executable 2>/dev/null | head -n 150
```

实际输出；退出码未记录：

```text
== RKNN models ==

== RKNN/RKLLM executables and demos ==
/usr/bin/start_rknn.sh
/usr/bin/restart_rknn.sh
/usr/bin/rkisp_demo
/usr/bin/rknn_server
/usr/bin/rknn_camera
```

观察：搜索范围内没有 `.rknn` 模型，也没有 RKLLM 名称的可执行文件；`rknn_camera` 是相机相关程序，不能当作 LLM 项目模型。当前镜像不能直接完成现成模型推理。2026-08-15 查阅 airockchip `rknn-llm` 上游资料确认 RK3588 与 `.rkllm` 模型格式受支持，且仓库含 `0.9.8` RKNPU driver 载荷；本板实际为 `0.8.2`。已查资料未给出本板的最低驱动版本或兼容矩阵，故下一步只获取源码供主机侧核对，暂不部署 runtime、模型或驱动。

### 步骤 5：固定官方 SDK 主机源码身份

目的：把后续阅读与部署判断绑定到一个可追溯的上游版本；本步骤不编译、安装或向 R1 传输文件。

执行端：Arch 主机。学习者报告已完成克隆；随后本地只读检查得到：

```text
## main...origin/main
commit=878f9361fd3afa7e167b7079918918f78d2c1c2a
author-date=2026-06-17T17:27:54+08:00
subject=release v1.3.0
true
```

目录检查显示顶层包含 `rkllm-runtime/`、`rkllm-toolkit/`、`rknpu-driver/`，并包含 `examples/rkllm_api_demo/` 和 `examples/rkllm_server_demo/`。这证明后续所需的阅读入口已在主机就绪；不证明任何其中的 runtime 或 driver 可用于当前 R1。

进一步文件盘点的实际输出：

```text
== RKLLM Linux runtime files ==
rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so
rkllm-runtime/Linux/librkllm_api/armhf/librkllmrt.so
rkllm-runtime/Linux/librkllm_api/include/rkllm.h
== Pure-text API demo files ==
examples/rkllm_api_demo/deploy/build-android.sh
examples/rkllm_api_demo/deploy/build-linux.sh
examples/rkllm_api_demo/deploy/CMakeLists.txt
examples/rkllm_api_demo/deploy/src/llm_demo.cpp
examples/rkllm_api_demo/export/data_quant.json
examples/rkllm_api_demo/export/export_rkllm.py
examples/rkllm_api_demo/export/generate_data_quant.py
examples/rkllm_api_demo/Readme.md
examples/rkllm_api_demo/README.md
== NPU driver package files ==
rknpu-driver/rknpu_driver_0.9.8_20241009.tar.bz2
```

观察：**已验证**SDK 为 AArch64 Linux 提供 `librkllmrt.so` 及公开头文件 `rkllm.h`；纯文本 API demo 提供 Linux CMake/构建脚本、C++ 调用源文件及模型导出脚本。它们是候选部署组件，而不是现有板端组件；板端已确认的是不同名称的 `librknnrt.so`。driver 目录当前只有独立版本 `0.9.8` 的压缩包，尚未解包或使用。

### 步骤 6：阅读纯文本 API demo 的运行契约

目的：在编译和下载模型前，确定 demo 的模型格式、依赖、目标架构和“完成一次推理”所需的最小证据。

执行端：Arch 主机；只读查看 `examples/rkllm_api_demo/README.md`、`deploy/CMakeLists.txt` 和 `deploy/src/llm_demo.cpp`。完整源码以本机固定的 v1.3.0 commit 为准，以下只摘录与部署判断相关的事实。

- README 声明 demo 使用 `DeepSeek-R1-Distill-Qwen-1.5B`，要求 `rkllm-toolkit >= 1.3.0`、`rkllm-runtime >= 1.3.0`、Python >= 3.9；输入模型是 `.rkllm`，可由 Toolkit 转换或从其所列 model zoo 获取。
- Linux 运行方式为 `llm_demo model_path max_new_tokens max_context_len`；README 示例为 `2048 4096`，且建议设置 `LD_LIBRARY_PATH=./lib`。该数值只是官方示例，尚未证明适合本板 4 GB RAM。
- CMake 在 Linux 上以 `CMAKE_SYSTEM_PROCESSOR` 选择 `rkllm-runtime/Linux/librkllm_api/<arch>/librkllmrt.so`，并安装该库和 `llm_demo` 到同一部署目录；R1 的 `aarch64` 架构应对应 SDK 中的 `aarch64` 目录，具体构建脚本的交叉编译设置待读取。
- `llm_demo.cpp` 以 `rkllm_createDefaultParam()` 创建参数，写入模型路径和采样/上下文参数后调用 `rkllm_init()`；仅其返回 0 时打印 `rkllm init success`。随后以 `rkllm_run()` 处理用户 prompt，callback 在 `RKLLM_RUN_NORMAL` 输出文本、在 `RKLLM_RUN_FINISH` 换行、在错误状态报告异常。故实际调用 `rkllm_run()` 并取得回答才是一次推理证据，`init success` 本身不足以证明生成成功或 NPU 实际工作。
- demo 默认 `keep_history = 0`，即单轮对话；它支持 `clear` 清除 KV cache，也预留 LoRA、prompt cache 和 chat template 接口但默认不启用。

观察：**已验证**该 SDK 的 AArch64 runtime 与 demo 在文件结构和 CMake 路径上可对齐；**待验证**当前 R1 的旧 RKNPU 0.8.2 是否满足 Runtime >=1.3.0 的实际要求、目标模型/上下文是否能在 4 GB RAM 运行，以及 build script 是否能使用主机现有 AArch64 工具链。未编译、未下载模型、未向板端写入。

### 步骤 7：检查官方构建脚本和驱动要求的明示程度

目的：确认是否可直接使用现有主机工具链，以及上游文档是否明确给出 RKNPU 内核驱动最低版本。

执行端：Arch 主机；只读读取 `deploy/build-linux.sh`，并在 SDK Markdown 文档中搜索 `rknpu`、`driver`、`runtime`、`rk3588` 等关键词。

- 脚本固定使用 `~/opts/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-{gcc,g++,strip}`，而主机此前已验证的是 `aarch64-linux-gnu-gcc`。脚本会向 CMake 显式传入 `CMAKE_SYSTEM_PROCESSOR=aarch64`、`CMAKE_SYSTEM_NAME=Linux` 和上述编译器，并在源码 `deploy/build/` 下产物化。
- 根 README 只说明 Toolkit、Runtime 和 RKNPU kernel driver 的职责，并列 RK3588 Series 为支持平台；关键词搜索没有展示 Runtime 与内核驱动的最低配对版本或 0.8.2/0.9.8 兼容结论。
- 同时存在 `Readme.md`（1.2.x）与 `README.md`（>=1.3.0）两份大小写不同的 API demo 文档；本次固定 commit 下应以当前 `README.md` 的 v1.3.0 要求为准，不能混用旧文档的 1.2.x 示例。

结论：**已验证**官方构建脚本不能不经核对地直接运行，因为其硬编码的主机路径与现有工具链名称不同。**待验证**主机是否有匹配的 `aarch64-linux-gnu-g++`、`cmake`，以及该工具链链接 SDK 的预编译 runtime 是否成功。应在仓库顶层 `build/` 用显式 CMake 参数做独立构建，不修改上游源码或脚本；当前仍不接触开发板。

### 步骤 8：验证主机可用的 AArch64 交叉构建工具

目的：在实际配置 demo 前，确认主机具备 CMake、AArch64 C++ 编译器与 strip，且编译器的目标三元组正确。

执行端：Arch 主机。实际输出：

```text
/usr/bin/cmake
/usr/bin/aarch64-linux-gnu-g++
/usr/bin/aarch64-linux-gnu-strip
aarch64-linux-gnu
aarch64-linux-gnu-g++ (GCC) 16.1.0
```

结论：**已验证**主机有完成此 demo 所需的构建前端和 AArch64 GNU C++ 工具链；`aarch64-linux-gnu` 与 R1 的 AArch64 用户空间架构相符。它不保证与板端 Ubuntu 的 glibc 或 RKLLM runtime ABI 相容，也不证明 build 可成功，须由下一步实际链接验证。

### 步骤 9：在独立目录交叉构建纯文本 demo

目的：验证现有 Arch AArch64 工具链能否编译 `llm_demo.cpp` 并链接 SDK 的 `librkllmrt.so`，且不让上游 `make install` 写入源码树。

执行端：Arch 主机。CMake 配置指定 source `src/rknn-llm/examples/rkllm_api_demo/deploy`、build `build/rkllm-api-demo`、Linux/aarch64 目标与 `aarch64-linux-gnu-{gcc,g++}`，随后执行 `cmake --build ... --parallel 4`。

实际输出末段：

```text
-- Build files have been written to: /home/loser/Study/rk3588/build/rkllm-api-demo
[ 50%] Building CXX object CMakeFiles/llm_demo.dir/src/llm_demo.cpp.o
[100%] Linking CXX executable llm_demo
[100%] Built target llm_demo
```

配置阶段出现 CMake CMP0177 policy warning，指向上游 `CMakeLists.txt` 的 `install()` destination 路径规范化；此次未执行 install，且配置、生成、编译和链接都成功，故该 warning 不影响本次 build 结论。

结论：**已验证**主机可在隔离 `build/rkllm-api-demo/` 中生成 AArch64 `llm_demo`。这证明源码、头文件、交叉工具链和预编译 `librkllmrt.so` 可完成链接；尚未检查 ELF 依赖、安装包、板端运行时 ABI、模型或 NPU 推理。

### 步骤 10：检查生成 ELF 的动态装载合同

目的：确认 `llm_demo` 的目标架构、动态解释器、运行库依赖和库搜索路径，避免把主机构建成功误当成板端可直接运行。

执行端：Arch 主机；只读使用 `file` 与 `readelf -d` 检查 `build/rkllm-api-demo/llm_demo`。

实际输出：

```text
ELF 64-bit LSB pie executable, ARM aarch64, dynamically linked,
interpreter /lib/ld-linux-aarch64.so.1, for GNU/Linux 3.7.0
NEEDED: librkllmrt.so
NEEDED: libstdc++.so.6
NEEDED: libm.so.6
NEEDED: libgcc_s.so.1
NEEDED: libc.so.6
RUNPATH: /home/loser/Study/rk3588/src/rknn-llm/.../librkllm_api/aarch64:
```

观察：**已验证**`llm_demo` 是动态链接的 AArch64 PIE，最直接的专用依赖为 `librkllmrt.so`，其余为目标 Ubuntu 应提供的标准 GNU C/C++ 运行库。二进制的 `RUNPATH` 是构建主机源码中的绝对路径，在 R1 不存在；但这是 `DT_RUNPATH` 而不是不可变的嵌入库。运行时设置 `LD_LIBRARY_PATH=./lib` 时，动态加载器会先在该路径寻找 `librkllmrt.so`，与官方部署说明一致。仍需检查 runtime 本身的依赖和目标板 glibc，再组装部署目录。

### 步骤 11：检查 RKLLM Runtime 的直接 ABI 依赖

目的：确定 `librkllmrt.so` 在动态装载阶段要求哪些系统库、是否直接依赖板端的旧 `librknnrt.so`，以及它要求的最高 glibc/C++ ABI 版本。

执行端：Arch 主机；只读使用 `file`、`readelf -d` 和 `readelf --version-info` 检查 SDK AArch64 runtime。

实际结果：runtime 为动态链接的 stripped AArch64 shared object，SONAME 为 `librkllmrt.so`；`DT_NEEDED` 仅列出 `libgomp.so.1`、`libpthread.so.0`、`libstdc++.so.6`、`libdl.so.2`、`libm.so.6`、`libgcc_s.so.1` 和 `libc.so.6`，未列出 `librknnrt.so`、DRM 库或 RKNPU 专用库。版本需求中的最高项为 `GLIBC_2.29`、`GLIBCXX_3.4.26`、`CXXABI_1.3.11`。

结论：**已验证**当前 RKLLM Runtime 不会在 ELF 动态装载阶段直接使用板端的 `librknnrt.so`；其直接前置是目标系统满足上述标准 GNU 库 ABI。**待验证**R1 是否满足这些 ABI，及 runtime 是否在执行 `rkllm_init()`/`rkllm_run()` 时以 `dlopen()` 或设备文件访问引入额外依赖。

### 步骤 12：核对 R1 的 glibc 和直接运行库存在性

目的：将主机识别到的 RKLLM Runtime 标准 ABI 前置，与 R1 实际用户空间而非发行版名称进行比较。

执行端：R1 root Shell；只读读取 `ldd --version` 与动态加载器缓存。

实际输出：

```text
ldd (Ubuntu GLIBC 2.35-0ubuntu3.6) 2.35
libstdc++.so.6 => /lib/aarch64-linux-gnu/libstdc++.so.6
libgomp.so.1 => /lib/aarch64-linux-gnu/libgomp.so.1
libgcc_s.so.1 => /lib/aarch64-linux-gnu/libgcc_s.so.1
```

结论：**已验证**R1 glibc 2.35 高于 Runtime 所需的 GLIBC 2.29，且所需的 AArch64 `libstdc++`、`libgomp`、`libgcc_s` 均可被动态加载器定位。`GLIBCXX_3.4.26` 与 `CXXABI_1.3.11` 的精确符号版本尚待读取；在其确认前，不宣称 Runtime 可在板端成功装载或初始化。

补充验证：在 `/lib/aarch64-linux-gnu/libstdc++.so.6` 中精确搜索后，分别输出 `GLIBCXX_3.4.26` 与 `CXXABI_1.3.11`。

结论更新：**已验证**R1 满足 `librkllmrt.so` 所声明的全部直接标准 ABI 前置：AArch64、GLIBC >=2.29、GLIBCXX >=3.4.26、CXXABI >=1.3.11，以及 `libgomp`/`libgcc_s`/`libstdc++` 存在性。它仍不证明 runtime 实际 `dlopen()` 的依赖、RKNPU 驱动兼容性或模型推理可用。

### 步骤 13：组装可传输的 RKLLM demo 候选包

目的：将已构建的 AArch64 executable 和 SDK AArch64 Runtime 复制到仓库 `build/` 下的独立目录，固定待部署文件的身份；不修改 SDK 源码或 R1。

执行端：Arch 主机。使用 `cmake --install build/rkllm-api-demo --prefix /home/loser/Study/rk3588/build/rkllm-api-demo/package`。

实际结果：CMake 安装了 `package/llm_demo` 和 `package/lib/librkllmrt.so`，并报告将 executable 的非工具链 runtime path 设为空。文件检查：

| 文件 | 大小 | SHA-256 | 识别结果 |
| --- | ---: | --- | --- |
| `llm_demo` | 75000 B | `681289d7d54decf994746af1f76b1c4f4b323e707c15ac2b859425a7f40c2047` | AArch64 动态 PIE |
| `lib/librkllmrt.so` | 7617472 B | `6a9e4fc5324c68921c3a900340361e107af7599fe34dc8fa7759b2c5ae22a6e6` | AArch64 动态 shared object |

结论：**已验证**候选包仅含 demo 和其专用 runtime，且 installation 清除了此前指向主机源码的绝对 RUNPATH。因此板端运行必须显式以 `LD_LIBRARY_PATH=./lib` 提供 `librkllmrt.so`；此包尚未传输到 R1，也不含模型。

### 步骤 14：确认 R1 候选上传目标

目的：在传输任何文件前，精确确认独立数据分区、可用空间以及目标目录是否已存在，避免覆盖系统或未知数据。

执行端：R1 root Shell；只读检查 `/userdata` 的挂载、空间和 `/userdata/rkllm-api-demo` 路径。

实际输出：

```text
/dev/mmcblk0p8 /userdata
/dev/mmcblk0p8   15G   36K   14G   1% /userdata
ls: cannot access '/userdata/rkllm-api-demo': No such file or directory
drwxr-xr-x 4 root root 4096 ... /userdata
```

结论：**已验证**`/userdata` 为独立 eMMC p8 数据分区，约有 14 GiB 可用空间，目标 `/userdata/rkllm-api-demo` 不存在。后续上传约 8 MiB 的候选包可限制在新建目录中，不覆盖 rootfs、板端现有 RKNN 运行库或其他用户数据；实际传输前仍须确认更新后 R1 的当前 IPv4 地址。

### 步骤 15：读取 R1 当前以太网地址

目的：精确指定 SSH/SCP 上传目标，不沿用更新前的地址假设。

执行端：R1 root Shell；只读读取 `ip -br addr show dev eth0`。

实际输出：

```text
eth0  UP  10.42.0.193/24 10.42.0.192/24 fe80::9ff4:a6ba:cee7:2d4f/64
```

结论：**已验证**同一个 `eth0` 当前同时持有 `10.42.0.193/24` 和 `10.42.0.192/24`；Linux 允许一个接口具备多个 IPv4 地址。**推测**：前者可能是当前优先的地址，后者可能来自先前手工或旧连接配置；原因未确认且当前不影响只读连通性测试。先从主机测试 `.193` 的 SSH 身份和目标目录，再决定传输，避免无根据地改变网络配置。

### 步骤 16：定位更新后 SSH 公钥认证阻塞

目的：在向空的 `/userdata/rkllm-api-demo` 传输候选包前，恢复已知主机公钥的安全登录通道。

执行端：R1 Debug UART root Shell 和 Arch 主机；读取当前 root 目录、授权文件、sshd 有效配置以及主机公钥指纹。

实际结果：SSH 到 `10.42.0.193` 请求密码；板端报告 `authorized_keys` 不存在，`permitrootlogin yes`、`pubkeyauthentication yes`、`strictmodes yes`，`/root` 为模式 700、属主/组 `youyeetoo:youyeetoo`。主机 `id_ed25519.pub` 指纹为 `SHA256:3MXA9RlxfRuO7mouBBDWxc3qh777QKVbH+6CnO1OTN0`。

结论：**已验证**这与已知的 StrictModes 根因一致：root 家目录属主不安全，且新镜像没有 root 授权文件。后续精确修复目标为 `/root` 的属主和新建的 `/root/.ssh/authorized_keys`；不改变 sshd 配置、不关闭 StrictModes、不使用未知密码。

修复与回归：将 `/root` 恢复为 `root:root`、创建模式 700 的 `.ssh`，并直接写入主机 `id_ed25519.pub` 的单行 OpenSSH 公钥；首次 Base64 写入结果不是有效公钥，已在确认无效后替换。主机以显式私钥、`IdentitiesOnly=yes`、禁用密码认证连接 `root@10.42.0.193`，实际输出：

```text
root
R1
target-absent
```

结论更新：**已验证**SSH Ed25519 公钥认证已恢复，且连接后仍确认候选上传目录不存在。下一步可受限上传仅含 `llm_demo` 和 `librkllmrt.so` 的包；上传后应先无参数执行以验证动态加载，不初始化模型或 NPU。

### 步骤 17：上传候选包并执行无参数动态加载测试

目的：验证两端文件完整性，并在不提供模型、不调用 `rkllm_init()` 的条件下检查 R1 动态加载器能否启动 demo。

执行端：Arch 主机通过 SSH/SCP 上传至已验证为空的 `/userdata/rkllm-api-demo`；随后 R1 通过 SSH 执行 `LD_LIBRARY_PATH=./lib ./llm_demo`。

实际结果：SCP 传输完成，板端 SHA-256 与主机一致：`llm_demo` 为 `681289d7d54decf994746af1f76b1c4f4b323e707c15ac2b859425a7f40c2047`，`librkllmrt.so` 为 `6a9e4fc5324c68921c3a900340361e107af7599fe34dc8fa7759b2c5ae22a6e6`。无参数运行没有打印 Usage，而是失败于动态加载器：

```text
./llm_demo: /lib/aarch64-linux-gnu/libc.so.6: version `GLIBC_2.38' not found (required by ./llm_demo)
./llm_demo: /lib/aarch64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.32' not found (required by ./llm_demo)
```

结论：文件传输完整，但 GCC 16 交叉编译的 executable 自身需要高于 R1 的 GLIBC 2.35 和 libstdc++ ABI。程序尚未到达 Usage、RKLLM Runtime 初始化或 NPU；建立[ISSUE-20260815-001](../issue/issue-20260815-001-rkllm-demo-target-abi-mismatch.md)跟踪。候选目录保留为失败证据，不覆盖系统库。

### 步骤 18：检查 R1 原生重编译条件

目的：在不替换系统基础库的前提下，寻找避开主机交叉 sysroot ABI 不匹配的最短路径。

执行端：R1 root Shell；只读检查 `cmake`、`make`、`gcc`、`g++` 的 PATH 入口。

实际输出：

```text
cmake  /usr/bin/cmake
make   /usr/bin/make
gcc    /usr/bin/gcc
g++    /usr/bin/g++
```

结论：**已验证**R1 自带完整的 C/C++/CMake/Make 原生构建环境。下一步可将公开的 demo 源码和 `rkllm.h` 传入已有候选目录，并以 R1 `g++` 链接其内的 `lib/librkllmrt.so`；这应使新 executable 使用目标用户空间 ABI。该假设仍需实际编译和 loader 测试验证。

### 步骤 19：在 R1 原生重建并回归动态加载

目的：验证本机构建能否避开 GCC 16 交叉产物的 GLIBC/GLIBCXX 不匹配，而不替换任何系统基础库。

执行端：Arch 主机上传 SDK 的 `llm_demo.cpp` 与 `rkllm.h` 到已有 `/userdata/rkllm-api-demo`；R1 root Shell 使用 `g++ -std=c++11 -I. llm_demo.cpp -L./lib -lrkllmrt -o llm_demo-r1` 构建。

实际结果：`file llm_demo-r1` 识别为 AArch64 动态 PIE；以 `LD_LIBRARY_PATH=./lib ./llm_demo-r1` 执行后输出：

```text
Usage: ./llm_demo-r1 model_path max_new_tokens max_context_len
```

结论：**已验证**R1 本机构建的 demo 可以加载 SDK Runtime 并进入 `main()`，从而解决 GCC 16 交叉产物的用户空间 ABI 阻塞。无参数 Usage 没有调用 `rkllm_init()`、没有模型、也没有 NPU 执行证据；下一步须选择、获取并校验适合 RK3588/4 GB 的 `.rkllm` 模型。

### 步骤 20：记录模型候选与首次运行前的资源基线

目的：在传输模型前建立可复核的文件完整性基线，并确认 R1 存储/内存的当前约束；不加载模型、不改动板端状态。

执行端：Arch 主机；第二条命令经 SSH 在 R1 root Shell 只读执行。

```fish
set model_path ~/Downloads/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm
stat -c 'size=%s bytes; type=%F' $model_path
sha256sum $model_path

ssh -i ~/.ssh/id_ed25519 -o IdentitiesOnly=yes root@10.42.0.193 \
  'free -h; df -h /userdata'
```

实际输出：

```text
size=2040247614 bytes; type=一般文件
85123bc6796760c9e670d6676a7d3e9527d1847406807441976fe1206b04115b  /home/loser/Downloads/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm

Mem:           3.8Gi       630Mi       2.4Gi       6.0Mi       802Mi       3.2Gi
Swap:             0B          0B          0B
/dev/mmcblk0p8   15G  7.5M   14G   1% /userdata
```

观察：**已验证**主机已有 2,040,247,614 字节的 `.rkllm` 候选，且 R1 的 `/userdata` 有约 14 GiB 可用空间。R1 总 RAM 为 3.8 GiB、无 swap，当前可用内存为 3.2 GiB；这不是模型可成功初始化的保证。模型的精确下载来源、运行时/驱动匹配和实际内存需求仍待验证。传输时必须使用该 SHA-256 在两端核对，再以保守上下文参数首次执行。

### 步骤 21：首次模型传输前的远端 Shell 语法失败

目的：创建受限模型目录并阻止同名文件覆盖后传输模型。

实际结果：远端命令错误地使用了 Fish 的 `and` 连接词；SSH 的远端默认 Shell 是 Bash，输出 `bash: line 1: and: command not found`。随后 `scp` 无法打开不存在的 `/userdata/rkllm-api-demo/models/`，最后 `sha256sum` 报目标文件不存在。

结论：**已验证**本次没有创建目标目录，也没有写入模型文件；最后的 `sha256sum` “No such file or directory” 是该结论的直接证据。该失败与模型内容、R1 NPU、SSH 身份验证或磁盘空间无关。修正命令时，主机 Fish 可以使用 `and`，但经 SSH 传给远端 Bash 的逻辑连接必须使用 `&&`。

### 步骤 22：传输模型并核对两端完整性

目的：仅将已校验的模型复制到已有项目目录，并以 SHA-256 排除传输损坏；不执行模型。

执行端：Arch 主机通过 SSH/SCP；目标为 R1 的 `/userdata/rkllm-api-demo/models/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm`。

实际输出：

```text
85123bc6796760c9e670d6676a7d3e9527d1847406807441976fe1206b04115b  /userdata/rkllm-api-demo/models/DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm
```

结论：**已验证**板端文件的 SHA-256 与主机基线 `85123bc6796760c9e670d6676a7d3e9527d1847406807441976fe1206b04115b` 一致。模型现已位于受限项目目录；该校验只证明两端字节相同，不证明文件来源、格式语义、运行时兼容性、`rkllm_init()` 或 NPU 推理成功。

### 步骤 23：首次 RKLLM 生成尝试

目的：在保守 `max_new_tokens=64`、`max_context_len=512` 参数下执行一次模型初始化后的短 prompt，验证首 token 生成路径。

执行端：R1，通过 SSH 交互运行 `llm_demo-r1`；环境变量为 `LD_LIBRARY_PATH=./lib` 与 `RKLLM_LOG_LEVEL=1`。学习者在 `user:` 输入 `ok`。

实际输出节选：

```text
user: ok
E rkllm: matmul(w8a8) run failedrobot:
E rkllm: matmul(w8a8) run failed
E rkllm: matmul(w8a8) run failed
E rkllm: matmul(w8a8) run failed
```

观察：**已验证**demo 已到达 `user:` 交互点，表明它未在无参数检查、动态加载或立即初始化失败处分支退出；但在生成路径出现 W8A8 matmul 错误，未取得模型回答。完整初始化头、内核日志、退出状态与失败后内存状态未保存，故根因未知。详见[ISSUE-20260815-002](../issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed.md)。

### 步骤 24：读取 W8A8 失败后的内存与内核侧信息

目的：以只读检查寻找 OOM killer、RKNPU/IOMMU fault 或其他内核侧报错，避免在没有线索时替换软件组件。

执行端：R1 root Shell，经 SSH 运行 `free -h`，以及以 `rknpu|npu|oom|out of memory|killed process|iommu|fault|error` 过滤的 `dmesg`。

实际结果：内存为总 3.8 GiB、used 631 MiB、available 3.2 GiB、swap 0；过滤日志未含 OOM、Killed process、IOMMU fault 或新 RKNPU 执行错误。所列 RKNPU 内容均为启动约 3 秒时的已知初始化日志。

结论：**已验证**本次采集未发现常见内核层内存耗尽或 NPU fault 的痕迹；但该结果不证明推理时资源充分，也不能排除用户态/NPU 专用分配失败。下一步应先获得 Runtime 的初始化版本头，再判断 Runtime、驱动与模型的配对假设。

### 步骤 25：确认 RKLLM Runtime 的最低驱动要求

目的：从同一次实际启动的 Runtime 日志取得其对 RKNPU driver 的直接要求，避免以模型名或猜测版本作为兼容结论。

实际输出：

```text
W rkllm: Warning: Your rknpu driver version is too low, please upgrade to 0.9.7
I rkllm: rkllm-runtime version: 1.3.0, rknpu driver version: 0.8.2, platform: RK3588
I rkllm: rkllm-toolkit version: 1.2.1b1, max_context_limit: 4096, npu_core_num: 3, target_platform: RK3588, model_dtype: W8A8
rkllm init success
```

结论：**已验证**Runtime 1.3.0 对本次实际组合明确报告 RKNPU driver 版本过低并要求 >=0.9.7；当前 0.8.2 是 W8A8 matmul 失败的直接版本阻塞条件。模型的 Toolkit 为 1.2.1b1、目标 RK3588、W8A8；它成功初始化，但生成仍失败。后续必须先确认匹配 R1 5.10.110 的驱动升级路径，不能直接安装任意 0.9.7+ 二进制。

### 步骤 26：识别 SDK 驱动载荷形态

目的：在不解压、不编译、不传输的条件下，判断 SDK `rknpu_driver_0.9.8_20241009.tar.bz2` 是否能直接作为 R1 升级输入。

执行端：Arch 主机；学习者以 `tar -tjf` 只读列出归档条目。

实际输出节选：

```text
drivers/rknpu/
drivers/rknpu/rknpu_gem.c
drivers/rknpu/rknpu_drv.c
drivers/rknpu/rknpu_job.c
drivers/rknpu/Makefile
drivers/rknpu/Kconfig
drivers/rknpu/include/rknpu_drv.h
```

结论：**已验证**归档提供的是 Linux 内核中 `drivers/rknpu/` 的源码子树，含 `.c`、`Kconfig` 和 `Makefile`，没有已加载的 `.ko`、完整 Linux 源码树或可烧录镜像证据。它需要与匹配的 R1 Linux 5.10.110 内核源码、配置及板级设备树集成/构建；不得单独复制到 R1 作为升级操作。

### 步骤 27：验证 RKNPU 的内建构建方式与本机构建入口

目的：用 SDK 驱动 Makefile 和运行内核配置验证“单独编译 `.ko`”是否是当前系统的可行路径。

执行端：Arch 主机只读读取归档内 `drivers/rknpu/Makefile`；R1 root Shell 只读读取 `/proc/config.gz` 并检查 modules/build 路径。

实际输出：

```make
obj-$(CONFIG_ROCKCHIP_RKNPU) += rknpu.o
ccflags-y += -I$(srctree)/$(src)/include
ccflags-y += -I$(src)/include
```

```text
CONFIG_ROCKCHIP_RKNPU=y
CONFIG_ROCKCHIP_RKNPU_DEBUG_FS=y
CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y
ls: cannot access '/lib/modules/5.10.110': No such file or directory
ls: cannot access '/lib/modules/5.10.110/build': No such file or directory
```

结论：**已验证**SDK Makefile 使用内核 Kbuild 的 `obj-$(CONFIG_...)` 规则和 `srctree`，不是外部模块工程；R1 的 RKNPU 配置为 `=y`，代表它已内建进当前 Linux 镜像，而非可卸载/重载的模块。并且目标缺少匹配的 modules/build 树。故当前不能、也不应单独编译和加载第二个 RKNPU `.ko`；修复路径必须从匹配的 R1 内核源码、配置、设备树和完整内核构建/恢复方案开始。

### 步骤 28：取得并初步对照厂商 Linux 5.10 源码

目的：获得可追溯的 R1 厂商完整内核候选，并仅比较 RKNPU 的构建结构，不构建或部署。

执行端：Arch 主机。学习者以浅克隆取得 `src/youyeetoo-r1-linux-kernel-5-10`；Git 输出为 `master...origin/master`、无短状态项，HEAD `82c69382f596e98f0458ae929966bcde28483af1`、作者时间 `2024-05-12T11:10:47+08:00`、主题 `r1-kernel v0.0.0`。

实际代码证据：树中存在 `drivers/rknpu/`；其 Makefile 使用 `obj-$(CONFIG_ROCKCHIP_RKNPU) += rknpu.o`，Kconfig 提供 `CONFIG_ROCKCHIP_RKNPU`、`CONFIG_ROCKCHIP_RKNPU_DRM_GEM` 等选项，驱动代码含 DRM GEM 与 DMA heap 分支。

结论：**已验证**厂商候选与当前 R1 的 `CONFIG_ROCKCHIP_RKNPU=y`、DRM render-node 设计在构建结构上相符，故它比孤立 SDK 驱动包更接近可重建的基线。**待确认**：它是否精确对应当前 eMMC 的 Linux 5.10.110 #4，以及树内 RKNPU 版本是否已达到 Runtime 所需的 >=0.9.7。

### 步骤 29：定位厂商树 RKNPU 的版本机制

目的：确认厂商树如何向用户态导出 RKNPU driver 版本，避免仅凭仓库日期或文件结构推断版本。

执行端：Arch 主机，只读搜索 `drivers/rknpu/`。

实际代码证据：`rknpu_drv.c` 的 `rknpu_get_drv_version()` 以 `DRIVER_MAJOR`、`DRIVER_MINOR` 和 `DRIVER_PATCHLEVEL` 生成版本码；`MODULE_VERSION()` 使用同一组三元组。`rknpu_debugger.c` 的 `rknpu_version_show()` 输出同一组三元组。

结论：**已验证**该树的精确 RKNPU 软件版本由上述三个宏决定，而不是硬件寄存器 `RKNPU_GET_HW_VERSION`。宏定义值尚未取得，因此尚不能声称厂商树已满足 >=0.9.7；下一步只读定位其定义。

### 步骤 30：确认厂商树 RKNPU 软件版本

目的：以版本宏的实际定义值判断厂商候选是否达到 RKLLM Runtime 1.3.0 明示的最低版本；不构建、修改或部署内核。

执行端：Arch 主机，只读搜索 `drivers/rknpu/rknpu_drv.c`。

实际代码证据：

```c
#define DRIVER_DESC "RKNPU driver"
#define DRIVER_DATE "20220829"
#define DRIVER_MAJOR 0
#define DRIVER_MINOR 8
#define DRIVER_PATCHLEVEL 2
```

结论：**已验证**厂商候选树的 RKNPU 软件版本为 `0.8.2`，与当前 R1 Runtime 打印的 `rknpu driver version: 0.8.2` 一致，且低于 Runtime 要求的 `>=0.9.7`。因此它不能单独作为本问题的驱动升级来源；“直接重编译该公开厂商树即可满足 Runtime”这一候选路径被排除。该版本一致性仍不能证明它与当前 eMMC 的完整内核、设备树或固件精确对应。

### 步骤 31：检查旧版 RKLLM Runtime 的 Git tag 入口

目的：优先寻找不改内核的用户态版本配对路径；只检查 `rknn-llm` 上游是否以 `v1.2*` 命名发布过 tag，不下载、切换或部署任何版本。

执行端：Arch 主机，在已固定的 `src/rknn-llm` 仓库运行 `git remote -v` 和 `git ls-remote --tags origin 'refs/tags/v1.2*'`。

实际结果：`origin` 的 fetch/push URL 均为 `https://github.com/airockchip/rknn-llm.git`；`ls-remote` 正常结束但没有输出匹配 `refs/tags/v1.2*` 的 tag。

结论：**已验证**该特定查询没有匹配结果。它只能说明 tag 不以 `v1.2` 直接开头，**不能**说明上游没有 v1.2 系列；下一步应列出全部 tag 以确认实际命名。

### 步骤 32：确认上游 v1.2 系列的实际 tag 命名

目的：纠正上一步对 tag 前缀的过窄假设，并定位与模型 Toolkit 名称对应的源码候选；只读查询，不下载、切换或部署。

执行端：Arch 主机，运行 `git -C $sdk_src ls-remote --tags origin | tail -n 80`。

实际输出节选：

```text
cb5b341364311065fd19eddd631a79a9f0c5afe1 refs/tags/release-v1.2.0
d546a0f975d62469cf1306f98103c737ff933506 refs/tags/release-v1.2.1
d8a9f6a9cce06922bf61ea9151d72fbf55dd55bb refs/tags/release-v1.2.1b1
04b51a3c10bf4416f3815b630a35fc35e20a38d1 refs/tags/release-v1.2.2
f7df8e5de9b40dc7aaa66194cf14d4895aaf159a refs/tags/release-v1.2.3
```

结论：**已验证**上游实际使用 `release-v*` 前缀，故步骤 31 的 `v1.2*` 模式没有输出是查询模式不匹配，不是“没有旧版 tag”。`release-v1.2.1b1` 的名称恰好与模型日志的 Toolkit `1.2.1b1` 一致，成为首个优先阅读的用户态候选。**待验证**：该 tag 的 Runtime 是否支持 RKNPU `0.8.2`、能否加载当前模型、以及是否会完成首 token 生成。

### 步骤 33：固定 Toolkit 同名的历史 SDK 候选

目的：取得与模型日志 Toolkit 名称相同的上游历史源码，供后续只读检查其 Runtime/驱动要求；保持 v1.3.0 源码不变，不向 R1 写入文件。

执行端：Arch 主机。学习者以 `--depth 1 --branch release-v1.2.1b1` 克隆到 `src/rknn-llm-release-v1.2.1b1/`，再运行 `git describe --tags --exact-match` 和 `git rev-parse HEAD`。

实际输出：

```text
release-v1.2.1b1
d8a9f6a9cce06922bf61ea9151d72fbf55dd55bb
```

结论：**已验证**本地候选精确固定为 `release-v1.2.1b1`，不是分支头或相近版本。它与模型日志的 Toolkit 名称相同，但尚未阅读其 Runtime 或兼容要求，不能据此推断 RKNPU `0.8.2` 可用。

### 步骤 34：阅读历史 tag 的文档化要求

目的：检查 `release-v1.2.1b1` 是否公开给出 RK3588/RKNPU 驱动版本配对，并避免把 Git tag 名称误当模型转换版本。

执行端：Arch 主机，只读搜索该 tag 中全部 Markdown 的 `rknpu`、`driver version`、版本号和 `Requirements`。

实际结果：根 README 只说明 RKNPU kernel driver 负责与硬件交互，未给出最低 RKNPU 版本；`examples/DeepSeek-R1-Distill-Qwen-1.5B_Demo/Readme.md` 的 Requirements 为 `rkllm-toolkit==1.2.0`。搜索输出没有 `0.8.2`、`0.9.7` 或最低驱动版本说明。

结论：**已验证**该 tag 的 Markdown 文档没有公开给出 RKNPU `0.8.2` 的兼容性证据。并且其 DeepSeek demo 记载的 Toolkit 为 `1.2.0`，故 `release-v1.2.1b1` 这个 tag 名称与当前模型日志同名，不能推出二者精确配对。下一步只读检查该 tag 的 Runtime 二进制是否内嵌最低驱动版本提示。

### 步骤 35：静态检查历史 Runtime 的驱动版本门槛线索

目的：在不运行、复制或替换板端库的条件下，检查 `release-v1.2.1b1` Runtime 是否含有与 RKNPU 版本检查有关的明文。

执行端：Arch 主机。对 `rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so` 运行 `file`，再以 `strings -a | rg` 搜索 RKNPU、`0.8.2`、`0.9.7`、升级和驱动版本提示。

实际输出节选：

```text
ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked, stripped
0.9.7
W rkllm: Warning: Your rknpu driver version is too low, please upgrade to %s
I rkllm: rkllm-runtime version: %s, rknpu driver version: %s, platform: %s
Current driver version: %d.%d.%d, recommend to upgrade the driver to the new version: >= %d.%d.%d
Mismatch driver version, %s requires driver version >= %d.%d.%d, but you have driver version: %d.%d.%d which is incompatible!
```

搜索退出码为 0。

结论：**已验证（静态）**历史 Runtime 1.2.1b1 同样内嵌 RKNPU 最低版本检查与 `0.9.7` 字面量，和当前 1.3.0 Runtime 的低版本警告高度一致。因此“降到该 tag 即自动兼容 0.8.2”没有证据支持，反而有较强反证。由于尚未加载该库，不能声称其对 R1 的实际阈值已由运行时证实；是否值得做隔离的用户态 A/B 测试须权衡其剩余信息价值。

### 步骤 36：初步比较 RKNPU 0.8.2 与 SDK 0.9.8 源码

目的：判断把 SDK 的 RKNPU 0.9.8 子树替入 R1 公开 5.10 树是否近似于小补丁升级，还是需要跨子系统移植；只解压到被 Git 忽略的 `build/local/` 并做文本 diff，不构建、不改源码。

执行端：Arch 主机。学习者将 `rknpu_driver_0.9.8_20241009.tar.bz2` 解到 `build/local/rknpu-driver-0.9.8/`，再对两个 `drivers/rknpu/` 目录运行 `diff -ru`。

实际证据：0.9.8 独有 `include/rknpu_devfreq.h`。`rknpu_drv.h` 将 driver date/version 从 `20220829`/`0.8.2` 改为 `20240828`/`0.9.8`，并新增 `linux/irq.h`、`rockchip_system_monitor.h`、`rockchip_ipa.h` 依赖；同时重构 IRQ/统计配置、reset 存储方式、IOMMU 多 domain 与 SG cache、NBUF、DRM fake device、session 和延迟 power-put 等数据结构/接口。

结论：**已验证**0.9.8 与 R1 0.8.2 的差异跨越多个驱动子系统，不是仅替换版本宏或少数文件即可安全完成的升级。**待验证**旧 R1 5.10 树是否已有新增 Rockchip 专用头文件及其所需实现；在该依赖检查前，不应尝试把 0.9.8 目录覆盖进厂商树。

### 步骤 37：检查新增 Rockchip 头文件是否存在

目的：验证 0.9.8 新增的两个直接 include 是否在公开 R1 5.10 源码树中存在；只读文件清单搜索，不推断函数实现或配置状态。

执行端：Arch 主机。学习者以 `rg --files | rg 'rockchip_(system_monitor|ipa)\\.h$'` 搜索 `src/youyeetoo-r1-linux-kernel-5-10`。

实际输出：

```text
include/soc/rockchip/rockchip_system_monitor.h
include/soc/rockchip/rockchip_ipa.h
search exit=0
```

结论：**已验证**旧 R1 树提供这两个 0.9.8 的直接头文件依赖，因而“头文件完全缺失”的移植阻塞已排除。**仍待验证**：0.9.8 实际调用的函数是否在这些头文件中声明、实现是否进入当前配置，以及其他跨子系统 API 是否兼容。

### 步骤 38：列出 0.9.8 对新增 Rockchip 接口的调用

目的：将“新增头文件存在”进一步收敛为有限的待对照符号集合，避免无目标地阅读整个 0.9.8 驱动。

执行端：Arch 主机。学习者对解压的 0.9.8 `drivers/rknpu/` 使用 `rg -o --no-filename 'rockchip_(system_monitor|ipa)_[A-Za-z0-9_]+' | sort -u`。

实际输出：

```text
rockchip_ipa_get_static_power
rockchip_ipa_power_model_init
rockchip_system_monitor_register
rockchip_system_monitor_unregister
```

结论：**已验证**0.9.8 对这两个新增 Rockchip 框架的直接调用可先收敛为四个符号：IPA 的功耗模型初始化/静态功耗读取，以及 system monitor 的注册/注销。下一步逐项在旧 R1 树中对照声明、实现和配置守卫；这不覆盖 0.9.8 的其他内核 API 变化。

### 步骤 39：对照四个 Rockchip API 的声明、实现与旧 RKNPU 使用

目的：确认 0.9.8 的四个直接 framework 调用在旧 R1 5.10 树中是否真实可用，并分辨配置关闭时的编译/运行语义。

执行端：Arch 主机。学习者同时搜索 `include/` 和 `drivers/` 中的四个符号。

实际证据：`drivers/soc/rockchip/rockchip_system_monitor.c` 定义并导出 register/unregister；`drivers/soc/rockchip/rockchip_ipa.c` 定义并导出 `power_model_init`/`get_static_power`。两份头文件分别以 `IS_REACHABLE(CONFIG_ROCKCHIP_SYSTEM_MONITOR)`、`IS_ENABLED(CONFIG_ROCKCHIP_IPA)` 提供真实原型或降级 inline stub。旧 `drivers/rknpu/rknpu_drv.c` 已在功耗模型、devfreq/system monitor 路径中调用全部四个符号；RKVENC、RKVDEC、GPU、CPU/DMC 等现有驱动也使用同一框架。

结论：**已验证**这四项不是 0.9.8 对 R1 公开树的新外部 API 阻塞：旧 RKNPU 已依赖相同接口，树中也有实现。配置关闭时，system monitor 返回 `ERR_PTR(-ENOTSUPP)`，IPA 初始化返回 `ERR_PTR(-ENOTSUPP)`、静态功耗返回 0；旧 RKNPU 已以 `IS_ERR` / `IS_ERR_OR_NULL` 处理相应失败。**待验证**当前运行内核是否实际启用 `CONFIG_ROCKCHIP_SYSTEM_MONITOR` 与 `CONFIG_ROCKCHIP_IPA`；这不代表 0.9.8 的 IOMMU、reset、DRM 等其他变更已兼容。

### 步骤 40：确认运行内核启用 IPA 与 system monitor

目的：从实际运行的 R1 内核配置确认旧 RKNPU 所用的功耗/监控框架是否编入，而不是仅从公开源码的 fallback 判断。

执行端：Arch 主机经 SSH 在 R1 root Shell 只读读取 `/proc/config.gz`。

实际输出：

```text
CONFIG_ROCKCHIP_IPA=y
CONFIG_ROCKCHIP_SYSTEM_MONITOR=y
remote exit=0
```

结论：**已验证**当前运行 R1 内核同时内建 IPA 和 Rockchip system monitor，因此旧 RKNPU 的这四项调用会使用真实实现而非配置关闭时的降级 stub。0.9.8 在这两个框架上的依赖与当前运行内核相容；IOMMU、reset、DRM/GEM、设备树资源和其余 API 变化仍待检查。

### 步骤 41：取得 0.8.2 与 0.9.8 的紧凑文件差异清单

目的：先确定升级范围和优先阅读顺序，避免直接淹没在全文 diff 中；只比较目录，不改动来源或构建。

执行端：Arch 主机。学习者对 R1 公开树和 `build/local/rknpu-driver-0.9.8/drivers/rknpu/` 运行 `diff -qr`。

实际结果：新增 `rknpu_devfreq.c`/`include/rknpu_devfreq.h` 和 `rknpu_iommu.c`/`include/rknpu_iommu.h`；`rknpu_drv.c`、`rknpu_gem.c`、`rknpu_job.c`、`rknpu_mem.c`、`rknpu_mm.c`、`rknpu_reset.c`、`rknpu_debugger.c`、多个头文件及 Makefile 均不同。`Kconfig`、`rknpu_fence.c` 和 `include/rknpu_fence.h` 未出现在差异清单，表示这次目录比较未发现它们不同。`diff` 退出码为 1，符合“存在差异”的预期。

结论：**已验证**0.9.8 是一次覆盖核心数据路径的驱动升级，不能按“小补丁”理解；新增的独立 IOMMU/devfreq 实现需要特别核对。**已验证**Kconfig 在此比较中未变，未见新增顶层 RKNPU 配置符号；但新文件是否纳入最终对象仍由 Makefile 和已有内核配置决定，下一步只读比较 Makefile。

### 步骤 42：确认新增对象的 Makefile 编译条件

目的：从 0.9.8 的 Kbuild 规则判断新增 IOMMU/devfreq 实现会在什么条件下进入 RKNPU built-in 对象。

执行端：Arch 主机，对两版 `drivers/rknpu/Makefile` 运行 unified diff。

实际输出：

```make
+rknpu-y += rknpu_iommu.o
+rknpu-$(CONFIG_PM_DEVFREQ) += rknpu_devfreq.o
```

`diff` 退出码为 1，符合存在差异的预期。

结论：**已验证**0.9.8 的 `rknpu_iommu.o` 无条件进入 RKNPU 对象；`rknpu_devfreq.o` 只有在已有通用配置 `CONFIG_PM_DEVFREQ=y` 时才进入。下一步从运行 R1 的 `/proc/config.gz` 核对该配置；无论其值如何，IOMMU 文件都必须继续做 API 兼容性检查。

### 步骤 43：确认运行内核启用 PM_DEVFREQ

目的：从运行中 R1 的真实配置确认 0.9.8 新增 `rknpu_devfreq.o` 会不会参与 built-in RKNPU 构建。

执行端：Arch 主机经 SSH 在 R1 root Shell 只读读取 `/proc/config.gz`。

实际输出：

```text
CONFIG_PM_DEVFREQ=y
remote exit=0
```

结论：**已验证**运行 R1 已启用通用 `PM_DEVFREQ`，因此 0.9.8 的 `rknpu_devfreq.o` 会和无条件的 `rknpu_iommu.o` 一同进入该驱动的构建。这个配置前置不是当前阻塞；下一步优先核对新增 IOMMU 文件使用的标准内核 API。

### 步骤 44：列出新增 IOMMU 文件的候选接口名

目的：从 `rknpu_iommu.c` 中分离需要旧内核提供的标准 IOMMU API 与仅属于新驱动的内部字段/辅助名称。

执行端：Arch 主机。学习者以 `rg -o --no-filename '\\biommu_[A-Za-z0-9_]+' | sort -u` 搜索新增文件。

实际输出包含 `iommu_attach_device`、`iommu_detach_device`、`iommu_dma_map_sg`、`iommu_domain_alloc/free`、`iommu_get_dma_cookie`、`iommu_get_domain_for_dev`、`iommu_map_sg`、`iommu_setup_dma_ops`、`iommu_unmap` 等，以及 `iommu_domains`、`iommu_domain_id`、`iommu_data` 等名称。

结论：**已验证**其中前一组是需在旧内核对照的标准 API 候选；后一组含新 RKNPU 的字段、内部函数或变量名，不能仅按名称误判为外部依赖。下一步只对标准 API 查 R1 5.10 的声明与实现。

### 步骤 45：对照 IOMMU domain 核心 API 的旧内核原型

目的：先以小范围检查最易受内核版本影响的 IOMMU domain 分配、释放和设备域获取接口，避免一次输出大量 IOMMU 实现细节。

执行端：Arch 主机，只读搜索 R1 树 `include/linux/iommu.h`。

实际输出节选：

```c
extern struct iommu_domain *iommu_domain_alloc(struct bus_type *bus);
extern void iommu_domain_free(struct iommu_domain *domain);
extern struct iommu_domain *iommu_get_domain_for_dev(struct device *dev);
```

同一头文件还显示配置关闭时的 inline fallback。

结论：**已验证**旧 R1 5.10 提供与 0.9.8 所需形式相符的 domain alloc/free 和 `get_domain_for_dev` 原型。`iommu_get_dma_cookie`、`iommu_setup_dma_ops` 未在本头文件的本次结果中出现，不能据此判为缺失；它们可能来自 DMA-IOMMU 专用头文件，下一步应先读取新 `rknpu_iommu.c` 的 include 清单。

### 步骤 46：定位新版 IOMMU 文件的直接头文件依赖

目的：从实际 include 链定位其余 DMA-IOMMU API 的声明来源，避免把未直接包含 `linux/iommu.h` 误判为接口缺失。

执行端：Arch 主机，只读搜索 0.9.8 `rknpu_iommu.c`。

实际输出：

```c
#include <linux/dma-map-ops.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include "rknpu_iommu.h"
```

结论：**已验证**该 `.c` 文件通过 `linux/dma-map-ops.h` 和私有 `rknpu_iommu.h` 组织 IOMMU/DMA 依赖，未直接 include `linux/iommu.h`。这不证明任何 API 存在或缺失；下一步只读取私有头的 include 与声明，再决定对照哪份旧内核头文件。

### 步骤 47：定位私有 IOMMU 头的标准接口入口

目的：确认 0.9.8 私有 IOMMU 声明依赖的标准内核头，缩小旧 R1 5.10 的兼容性对照范围。

执行端：Arch 主机，只读搜索 0.9.8 `include/rknpu_iommu.h`。

实际输出：

```c
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/iommu.h>
#include <linux/iova.h>
#include <linux/version.h>
#include <linux/dma-iommu.h>
#include "rknpu_drv.h"
```

结论：**已验证**私有头明确以 `linux/iommu.h` 提供通用 IOMMU 接口，并以 `linux/dma-iommu.h` 引入 DMA-IOMMU 接口；因此下一步应只读对照旧 R1 的 `include/linux/dma-iommu.h` 是否存在并声明新版实际使用的 DMA cookie/映射 API。`iova.h`、mutex、seq_file 和版本条件也可能带来后续兼容性检查，但尚未构成缺失结论。

### 步骤 48：对照 DMA cookie 与 DMA ops 的旧内核原型

目的：确认新版 IOMMU 路径所需的 DMA-IOMMU 声明在旧 R1 5.10 中存在，且参数形式相符。

执行端：Arch 主机，只读搜索 R1 树 `include/linux/dma-iommu.h`。

实际输出节选：

```c
int iommu_get_dma_cookie(struct iommu_domain *domain);
void iommu_setup_dma_ops(struct device *dev, u64 dma_base, u64 size);
```

同一头还提供配置关闭时的 inline fallback：`iommu_get_dma_cookie()` 返回 `-ENODEV`，`iommu_setup_dma_ops()` 为空实现。

结论：**已验证**旧 R1 5.10 提供与 0.9.8 使用形式相符的 DMA cookie 获取和 DMA ops 设置原型。是否实际走到正常实现仍取决于旧内核的 IOMMU/DMA 配置和新版调用方式；下一步应读取新版这两个函数的调用点与传参，避免仅凭同名原型判定整体兼容。

### 步骤 49：阅读新版 DMA domain 迁移调用点

目的：确认 0.9.8 的 DMA-IOMMU API 在什么运行时流程中使用，而不把“原型存在”误写为“升级可行”。

执行端：Arch 主机，只读搜索 0.9.8 `rknpu_iommu.c` 的调用上下文。

实际输出节选：

```c
// init domain iova_cookie
iommu_get_dma_cookie(dst_domain);

iommu_detach_device(src_domain, rknpu_dev->dev);
ret = iommu_attach_device(dst_domain, rknpu_dev->dev);
if (ret) {
        /* ... */
}

// set domain type to dma domain
dst_domain->type |= __IOMMU_DOMAIN_DMA_API;
// iommu dma init domain
iommu_setup_dma_ops(rknpu_dev->dev, 0, dma_limit);
```

结论：**已验证**新版代码的意图是创建目标 domain 后，为它准备 IOVA/DMA 状态，将 NPU 从源 domain 迁移到目标 domain，再将 NPU 设备的 DMA 映射操作设为该 DMA domain。`iommu_attach_device()` 的返回值被处理；本次节选中 `iommu_get_dma_cookie()` 的返回值未检查。直接操作 `dst_domain->type` 与 domain 迁移都属于更深的内核行为依赖，故“原型匹配”不足以证明可安全替换。下一步应读取该调用点所属函数的前部，确认 source/target domain 的取得方式和错误回滚路径。

### 步骤 50：阅读 DMA domain 迁移的状态与回滚分支

目的：确认新版驱动如何保持其内部 domain 状态与内核实际绑定状态一致，并区分两个目标 domain 分支的失败处理。

执行端：Arch 主机，只读读取 0.9.8 `rknpu_iommu.c`。

实际输出节选：

```c
src_domain_id = rknpu_dev->iommu_domain_id;
if (domain_id == src_domain_id)
        return 0;

src_domain = iommu_get_domain_for_dev(rknpu_dev->dev);
if (src_domain != rknpu_dev->iommu_domains[src_domain_id])
        return -EINVAL;

dst_domain = rknpu_dev->iommu_domains[domain_id];
if (dst_domain != NULL) {
        iommu_detach_device(src_domain, rknpu_dev->dev);
        ret = iommu_attach_device(dst_domain, rknpu_dev->dev);
        if (ret) {
                iommu_attach_device(src_domain, rknpu_dev->dev);
                return ret;
        }
        rknpu_dev->iommu_domain_id = domain_id;
} else {
        dst_domain = iommu_domain_alloc(bus);
        /* 初始化 cookie、detach、attach；attach 失败时释放 dst_domain */
}
```

结论：**已验证**驱动先以 `iommu_get_domain_for_dev()` 校验内核实际绑定值是否等于自身缓存的 `iommu_domains[src_domain_id]`，不一致即拒绝迁移。目标 domain 已存在时，attach 失败会尝试重新 attach 源 domain；新建目标 domain 的 attach 失败路径在本次输出中只释放新 domain，未显示重新 attach 源 domain。后者是否安全依赖 IOMMU attach 失败语义及后续代码，当前仅标为**待验证的回滚差异**，不能直接定性为 bug。下一步应读取紧随其后的“reset default iommu domain”部分，确认迁移后的 domain 状态收尾。

### 步骤 51：定位相对源码路径失败的原因

目的：区分“源码或文件缺失”与“主机当前工作目录导致的相对路径解析错误”，避免对源码状态作出错误结论。

执行端：Arch 主机；当前目录为 `src/youyeetoo-r1-linux-kernel-5-10`。

实际输出：

```text
sed：无法读取 build/local/rknpu-driver-0.9.8/drivers/rknpu/rknpu_iommu.c：没有那个文件或目录
```

结论：**已验证**`$new_rknpu` 保存的是相对仓库根目录的 `build/local/...` 路径；从内核源码子目录执行时，它被解析为 `src/youyeetoo-r1-linux-kernel-5-10/build/local/...`，因此失败。该输出不证明 0.9.8 文件缺失。后续本步骤统一使用仓库绝对路径；不改变任何源码或板端状态。

### 步骤 52：阅读 DMA domain 迁移的默认 domain 与并发控制

目的：确认迁移收尾如何影响 IOMMU group 默认 domain，以及切换操作如何避免与正在使用的 domain 冲突。

执行端：Arch 主机，以绝对路径只读读取 0.9.8 `rknpu_iommu.c`。

实际输出节选：

```c
// reset default iommu domain
rknpu_dev->iommu_group->default_domain = dst_domain;

if (domain_id == rknpu_dev->iommu_domain_id) {
        atomic_inc(&rknpu_dev->iommu_domain_refcount);
        /* ... */
}

if (atomic_read(&rknpu_dev->iommu_domain_refcount) == 0) {
        ret = rknpu_iommu_switch_domain(rknpu_dev, domain_id);
        /* ... */
}
```

结论：**已验证**每次成功迁移后，0.9.8 直接将 group 的 `default_domain` 指向目标 domain。`rknpu_iommu_domain_get_and_switch()` 用 `domain_lock` 保护检查与切换：已经是目标 domain 时增加引用计数；仅在引用计数为 0 时才尝试实际切换。完整等待/超时和对应 put 路径尚未阅读。直接访问 `iommu_group->default_domain` 是结构体字段 ABI 依赖，下一步应优先检查旧 R1 5.10 的 `struct iommu_group` 是否含该字段及其类型，不能由函数原型替代。

### 步骤 53：对照旧 R1 的 IOMMU group 结构体可见性

目的：核对新版直接访问 `iommu_group->default_domain` 是否能由旧 R1 的公共内核头支持。

执行端：Arch 主机，只读搜索旧 R1 内核树。

实际输出节选：

```c
/* drivers/iommu/iommu.c */
struct iommu_group {
        /* ... */
        struct iommu_domain *default_domain;
        struct iommu_domain *domain;
        /* ... */
};

/* include/linux/iommu.h */
struct iommu_group {};
```

结论：**已验证**旧 R1 树中 `default_domain` 的完整结构体定义位于 IOMMU core 私有实现 `drivers/iommu/iommu.c`，而公共 `include/linux/iommu.h` 的本次输出只见不含字段的空 fallback 定义。0.9.8 RKNPU 并未包含 IOMMU core 私有实现，因而不能依赖该私有定义访问字段。**强结论**：就当前旧 R1 源树的公共接口而言，直接替换 0.9.8 RKNPU 会遇到 `iommu_group->default_domain` 结构体可见性/布局不兼容，不能作为无适配的升级方案。该空 fallback 对应的精确配置条件仍待读出，但不会把 IOMMU core 私有字段变成稳定的外部驱动 API。

### 步骤 54：确认 IOMMU group 空结构体的配置守卫

目的：确定旧 R1 公共头中空 `iommu_group` 是否由通用 IOMMU API 配置关闭触发。

执行端：Arch 主机，只读读取旧 R1 `include/linux/iommu.h`。

实际输出：

```c
#else /* CONFIG_IOMMU_API */

struct iommu_ops {};
struct iommu_group {};
```

结论：**已验证**空 group 位于 `CONFIG_IOMMU_API` 的关闭分支。新版 0.9.8 所依赖的通用 IOMMU domain/DMA-IOMMU API 因而有明确的配置前置；运行板虽有 Rockchip IOMMU 启动日志，但这不能自动证明 `CONFIG_IOMMU_API=y`。同时，即使开启该 API，旧树也不应假定对外暴露 IOMMU core 私有 `default_domain` 布局。下一步必须在 R1 运行内核的 `/proc/config.gz` 只读确认 `CONFIG_IOMMU_API`、`CONFIG_IOMMU_SUPPORT` 与 `CONFIG_ROCKCHIP_IOMMU` 的实际值。

### 步骤 55：验证运行 R1 的通用 IOMMU 配置

目的：核对运行板是否满足新版通用 IOMMU API 的配置前置，避免把源码 fallback 分支当成实际内核配置。

执行端：R1 目标 Linux，root Shell，只读读取 `/proc/config.gz`。

实际输出：

```text
CONFIG_IOMMU_API=y
CONFIG_IOMMU_SUPPORT=y
CONFIG_ROCKCHIP_IOMMU=y
```

结论：**已验证**运行 R1 已启用通用 IOMMU API、IOMMU 支持和 Rockchip IOMMU；`CONFIG_IOMMU_API` 关闭不是当前阻塞。此前在源码头中观察到的空 group 仅是关闭配置的 fallback，不能代表运行配置。仍待验证的是启用 API 分支中公共头对 `struct iommu_group` 的可见性；若仅是前向声明，0.9.8 对 `default_domain` 的直接字段访问仍无法由旧 R1 公共接口编译。

### 步骤 56：确认启用 IOMMU API 分支中的 group 声明

目的：确认旧 R1 公共头在实际启用的通用 IOMMU API 分支中，是否向外部驱动公开 `iommu_group` 的字段布局。

执行端：Arch 主机，只读读取旧 R1 `include/linux/iommu.h`。

实际输出：

```c
struct iommu_ops;
struct iommu_group;
struct bus_type;
struct device;
struct iommu_domain;
```

结论：**已验证**启用 `CONFIG_IOMMU_API` 时，旧 R1 公共头仅对 `struct iommu_group` 作前向声明，即它是不透明类型，不公开 `default_domain` 字段。结合步骤 53 的 IOMMU core 私有完整定义，0.9.8 的 `rknpu_dev->iommu_group->default_domain` 无法按旧 R1 树原样编译。至此已获得足够证据：SDK 的 0.9.8 `drivers/rknpu/` 不能直接替换进该旧 R1 5.10 源树；安全路径应转为寻找明确适配 R1 的完整 BSP/内核或供应商维护的适配补丁，而非继续猜测性移植。

### 步骤 57：定位官方 R1 完整 BSP 的文档入口

目的：从厂商本地文档索引确认完整 R1 Ubuntu BSP 的来源入口，不下载、不构建，避免把独立内核仓库或 SDK 驱动子树当成可直接部署的升级载荷。

执行端：Arch 主机，只读搜索本地 `R1/README.md`。

实际输出节选：

```text
Ubuntu source code compilation *Compile complete firmware* *Step-by-step compilation*
https://wiki.youyeetoo.com/en/r1/Ucompile
```

补充资料记载：官方 R1 Ubuntu 页面列有获取 Ubuntu 源码、构建 OS 镜像和构建选项章节；官方 Debian 编译页展示了完整 R1 SDK 的归档、repo 同步、`kernel/` 子目录与 `rk3588s-yyt.img` 产物形式。Debian 页不能证明 Ubuntu SDK 或 RKNPU 版本。

结论：**已验证**存在 R1 官方 Ubuntu 完整 BSP 的文档入口，且厂商发布完整 SDK 而非仅驱动子树。当前尚未得到 Ubuntu SDK 文件名、下载链接、版本或 RKNPU 版本，不能下载或将它视为当前问题的修复方案。下一步由学习者在浏览器打开 Ubuntu 源码页，复制其“Get Ubuntu Source Code”部分的文件名、日期和链接以建立候选身份。

### 步骤 58：排除误认的 Debian SDK 链接

目的：避免将厂商 Debian SDK 当作 Ubuntu BSP 候选，防止后续下载或构建时偏离当前系统目标。

执行端：资料核对；学习者提供 Google Drive 链接，结合官方 Debian 编译页进行只读比对。

实际信息：

```text
https://drive.google.com/drive/folders/1AYpMrvScgpaZQYcPmMsULS1woYz5tKPk?usp=drive_link
```

结论：**已验证**该链接与官方 `Dcompile` 页面所列 “R1-Debian Source Download” 一致。因此它是 Debian SDK 候选，不是已确认的 Ubuntu SDK；不能把它作为当前 Ubuntu/RKNPU 升级路径的修复载荷。下一步仍是从 `Ucompile` 页面取得 Ubuntu 源码下载项的独立身份信息。

### 步骤 59：更正 Debian 标注 SDK 与 Ubuntu 页面之间的关系

目的：核对步骤 58 的“不是 Ubuntu SDK”表述是否会错误排除厂商 Ubuntu 页面实际推荐的源码入口。

执行端：资料核对；学习者在浏览器打开 `Ucompile` 页面并提供页面截图。未下载、未构建、未改动板端或主机源码。

实际观察：截图的面包屑为 `r1 / Ucompile`，页面标题为“Ubuntu编译步骤”。其“解压源码”段落提示将源码下载到 x86 Ubuntu 主机；该段列出的两个下载项却均命名为 “R1-Debian Source”，其中官方链接为：

```text
http://dd.youyeetoo.cn:5000/sharing/KOnmowBRq
```

另一个为与步骤 58 相同的 Baidu 网盘来源。

结论：**已验证**厂商 Ubuntu 编译页本身引用了该 Debian 标注的源码入口。因此步骤 58 中“不能作为当前 Ubuntu/RKNPU 升级路径的修复载荷”仍成立（尚无版本、板型、RKNPU 或构建产物证据），但“不是 Ubuntu SDK”表述过强，现予以更正：它是 Ubuntu 页面也引用的 R1 Linux BSP 候选。它究竟是多 rootfs 共用 BSP，还是页面文案/链接复用错误，仍属**待确认**。唯一下一步改为读取同页后续“自动编译”或“分步编译”段中的 `BoardConfig-*.mk` 文件名和 kernel 构建命令；这能判断 Ubuntu 页面是否使用独立配置，而无需下载。

### 步骤 60：读取 Ubuntu 页面给出的内核构建入口

目的：确认 Ubuntu 页面是否给出完整的 R1 内核配置和产物目标，而非只引用一个不明来源的 SDK。

执行端：资料核对；学习者继续阅读 `Ucompile` 页面并抄录“单独编译 kernel”命令。未下载、未构建、未改动任何镜像。

实际信息：

```bash
cd kernel
make CROSS_COMPILE=../prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu- ARCH=arm64 rockchip_linux_defconfig rk3588_linux.config
make CROSS_COMPILE=../prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu- ARCH=arm64 rk3588s-yyt.img
```

结论：**资料记载**该入口预期源码含 `kernel/`、随 SDK 提供的 AArch64 GCC 10.3 工具链、`rockchip_linux_defconfig`、`rk3588_linux.config` 和 R1/Yyt 专用 `rk3588s-yyt.img` 目标。它没有使用 `BoardConfig-*.mk` 包装配置；前一条“寻找 BoardConfig 名称”的下一步不适用。该信息显著增强了“这是可构建完整 BSP 候选”的判断，但仍**不能证明**下载包中的 RKNPU 已达 0.9.7、产物适配 R1 V2，或可安全刷入当前 eMMC。

下一步：下载源码包到主机后只读检查 `kernel/drivers/rknpu/include/rknpu_drv.h` 的版本宏和 `kernel/Makefile`/目标存在性；在确认版本前不编译或烧录。

### 步骤 61：恢复本地 R1 内核源码工作树并核对其构建基线

目的：在不下载分卷 SDK 的前提下，确认已克隆的 R1 内核树是否仍可作为自维护内核的编译基线。

执行端：Arch 主机。先只读检查工作树；确认 Git 对象完整后，由学习者恢复当前 `HEAD` 缺失的受跟踪文件。未访问开发板，未构建或烧录。

实际结果：初始工作树仅剩 `.git` 和少量点文件，Git 报告 84,052 个受跟踪文件删除。只读检查显示未启用 sparse checkout、`git fsck --no-dangling --no-reflogs` 无输出，且 `HEAD` 含 `drivers/rknpu/include/rknpu_drv.h`、`arch/arm64/configs/rockchip_linux_defconfig` 与 `rk3588_linux.config`。学习者执行：

```fish
git -C ~/Study/rk3588/src/youyeetoo-r1-linux-kernel-5-10 restore --source=HEAD --worktree -- .
```

终端输出：

```text
刷新索引: 100% (84058/84058), 完成.
## master...origin/master
```

恢复后的只读核对：提交为 `82c69382f596e98f0458ae929966bcde28483af1`（2024-05-12）；页面所列两个配置输入都存在。RKNPU 宏仍为 `0.8.2`。`rk3588s-yyt.dts` 存在，但在该独立内核树的 `Makefile`、`arch/` 与 `scripts/` 中未搜到 `rk3588s-yyt.img` 字符串。

结论：**已验证**本地树可恢复为干净、可阅读的 R1 内核源码基线；它可用于先建立独立 out-of-tree 构建流程。它不是 NPU 问题的直接修复版，因为内含 RKNPU 仍为 0.8.2。厂商页面的 `rk3588s-yyt.img` 很可能依赖完整 SDK 的外层打包脚本，或页面命令与独立公开树不完全对应；这在实际内核构建成功前均属**待确认**。

### 步骤 62：比较官方 Rockchip BSP 分支的 RKNPU 版本

目的：不下载完整内核树，先确认 Rockchip 的较新 5.10、6.1、6.6 BSP 分支是否能提供超过 0.8.2 的 RKNPU 驱动，以及选择哪个分支可最小化 R1 移植范围。

执行端：Arch 主机。学习者先以 `git ls-remote --heads` 只读取分支引用，再以 `curl` 分别读取三个分支的 `drivers/rknpu/include/rknpu_drv.h`；未克隆完整树、未构建、未改动开发板。

实际输出：

```text
== develop-5.10 ==
#define DRIVER_DATE "20240828"
#define DRIVER_MAJOR 0
#define DRIVER_MINOR 9
#define DRIVER_PATCHLEVEL 8
== develop-6.1 ==
#define DRIVER_DATE "20240828"
#define DRIVER_MAJOR 0
#define DRIVER_MINOR 9
#define DRIVER_PATCHLEVEL 8
== develop-6.6 ==
#define DRIVER_DATE "20240828"
#define DRIVER_MAJOR 0
#define DRIVER_MINOR 9
#define DRIVER_PATCHLEVEL 8
```

结论：**已验证**三个官方 BSP 分支均含 RKNPU `0.9.8`，满足当前 RKLLM Runtime 明示的 `>=0.9.7` 前置；没有证据显示 6.1 或 6.6 因 RKNPU 版本而优于 5.10。选择 `develop-5.10` 作为下一候选是**工程判断**：它与 R1 当前内核、已恢复的 R1 板级树同属 5.10，预计 DTS、厂商外设和启动镜像的迁移差异最小。该判断不等同于“可直接刷入”；R1 DTS、配置、构建产物和启动链仍待逐项验证。

下一步：只读取并记录 `develop-5.10` 的精确 commit；随后再按需使用 partial/sparse clone 获取该分支，避免完整下载。

### 步骤 63：固定 Rockchip `develop-5.10` 候选提交

目的：为后续按需获取官方 5.10 BSP 的内容建立可复现身份，避免引用会继续移动的分支名。

执行端：Arch 主机，只读取远程引用。

实际输出：

```text
bfa51d2ab08140d1309afc9a9fe0fc2878cee35a	refs/heads/develop-5.10
```

结论：**已验证**截至本次查询，官方 `rockchip-linux/kernel` 的 `develop-5.10` 指向 `bfa51d2ab08140d1309afc9a9fe0fc2878cee35a`。该身份对应步骤 62 已验证的 RKNPU 0.9.8 候选；尚未克隆，未取得 R1 DTS 迁移所需文件。

### 步骤 64：按需获取官方 5.10 BSP 的 RKNPU、DTS 与配置目录

目的：在网络受限条件下，以可复现 commit 获取 RKNPU 0.9.8 与 R1 DTS 迁移比较所需的最小源码范围。

执行端：Arch 主机。学习者以 `--filter=blob:none --no-checkout` 建立 shallow partial clone，设置 sparse-checkout 仅包含 `drivers/rknpu`、`arch/arm64/boot/dts/rockchip` 和 `arch/arm64/configs`，再检出步骤 63 的固定 commit。未修改既有 R1 源码、未构建、未访问开发板。

实际输出：

```text
## HEAD（非分支）
22M    /home/loser/Study/rk3588/src/rockchip-linux-kernel-develop-5-10
```

结论：**已验证**partial/sparse 工作树建立成功，当前为固定 commit 的 detached HEAD；22 MB 表明尚未下载完整内核树。该目录是后续对照和移植的官方 5.10 BSP 参考，不能直接作为 R1 启动镜像。

下一步：只列出该树是否已有 Yyt/R1 DTS，以及最接近的 RK3588 EVB DTS 名称；据此决定复用或迁移本地 `rk3588s-yyt.dts`。

### 步骤 65：确认官方 5.10 BSP 的 R1 设备树基础

目的：确认升级候选是否已有与当前 R1 运行时设备树身份对应的 DTS，避免以不必要的 EVB1 或其他板型作为迁移基线。

执行端：Arch 主机，只检查已 sparse-checkout 的 DTS 文件名。

实际输出：

```text
absent:  rk3588s-yyt.dts
present: rk3588s-evb4-lp4x-v10.dts
absent:  rk3588-evb1-v10.dts
```

结论：**已验证**官方 `develop-5.10` 已包含 `rk3588s-evb4-lp4x-v10.dts`；它与本板运行时 `/proc/device-tree/compatible` 中的首项 `rockchip,rk3588s-evb4-lp4x-v10` 对应。上游没有 Yyt 命名 DTS，这不表示无法适配：本地 R1 5.10 树的 `rk3588s-yyt.dts` 可作为厂商板级差异来源。下一步应比较两份 DTS 的差异规模和关键节点，不能因 compatible 同名就直接使用上游 DTS 构建或烧录。

### 步骤 66：收敛 R1 与官方 EVB4 顶层 DTS 的差异边界

目的：确定 R1 厂商顶层 DTS 与官方相同 EVB4 LP4X V10 DTS 是否已有大量板级节点分叉，还是只通过不同的基础 include 选择板级/系统配置。

执行端：Arch 主机。学习者对本地 R1 `rk3588s-yyt.dts` 与固定官方 `develop-5.10` 的 `rk3588s-evb4-lp4x-v10.dts` 执行只读统一 diff；未复制文件、未构建、未访问或改动开发板。

实际输出：

```diff
@@ -6,8 +6,8 @@

 /dts-v1/;

-#include "rk3588s-evb4-lp4x.dtsi"
-#include "rk3588-android.dtsi"
+#include "rk3588s-evb4-lp4x-yyt.dtsi"
+#include "rk3588-linux.dtsi"

 / {
  model = "Rockchip RK3588S EVB4 LP4X V10 Board";
```

结论：**已验证**两份顶层 DTS 在本次完整 diff 中只有一个 hunk，根节点 `model` 后的正文相同；差异集中于两条 include：官方候选采用 `rk3588s-evb4-lp4x.dtsi` 与 `rk3588-android.dtsi`，R1 厂商树采用 `rk3588s-evb4-lp4x-yyt.dtsi` 与 `rk3588-linux.dtsi`。这将 R1 迁移的首个检查范围收敛到这四份基础 `.dtsi`，但尚不能推出它们内容兼容，更不能直接用官方 DTS 构建或烧录。

下一步：只核对四份 `.dtsi` 在本地 R1 树和官方候选树中的存在性；依据结果再决定是比较同名 Linux include，还是定位并移植 Yyt 板级差异。

### 步骤 67：确认 R1 专有基础 DTS 的唯一性

目的：判断步骤 66 的四份 include 中，哪些是两棵树共有的基础文件，哪些是 R1 厂商树独有的板级差异来源。

执行端：Arch 主机。学习者仅以 `test -f` 检查两棵源码树中四个文件的存在性；未读取或修改文件内容，未构建、未烧录。

实际输出：

```text
rk3588s-evb4-lp4x-yyt.dtsi         r1=yes  upstream=no
rk3588s-evb4-lp4x.dtsi             r1=yes  upstream=yes
rk3588-linux.dtsi                  r1=yes  upstream=yes
rk3588-android.dtsi                r1=yes  upstream=yes
```

结论：**已验证**`rk3588s-evb4-lp4x-yyt.dtsi` 是本地 R1 厂商树独有的文件；其余三份基础 `.dtsi` 在两棵树中均存在。顶层差异因此不再需要以“Android 与 Linux include 不同”作为迁移解释：R1 专有板级差异首先应从 Yyt 文件相对同树标准 `rk3588s-evb4-lp4x.dtsi` 的实际 diff 中定位。尚未验证同名文件在两棵树中的内容是否完全相同。

下一步：只统计并查看 `rk3588s-evb4-lp4x.dtsi` 与 `rk3588s-evb4-lp4x-yyt.dtsi` 的 diff hunk 标题；据此筛出 R1 实际改变的节点，不构建、不烧录。

### 步骤 68：量化 R1 专有基础 DTS 的差异规模

目的：在阅读具体属性前先量化 Yyt 板级文件相对同树标准 EVB4 基础文件的差异范围，决定后续采用分类阅读而不是无差别逐行阅读。

执行端：Arch 主机。学习者生成主机侧可再生文件 `build/local/r1-yyt-vs-evb4.dtsi.diff`，再统计并列出 unified diff 的 hunk 标记；命令仅读取源码，不构建、不烧录。

实际输出：

```text
26
3:@@ -6,9 +6,14 @@
20:@@ -29,7 +34,7 @@
29:@@ -46,35 +51,49 @@
...
787:@@ -1155,47 +1071,53 @@
851:@@ -1223,10 +1145,4 @@
```

为避免重复保存长输出，中间 22 个 hunk 标记已在学习者终端显示，此处省略；完整可再生 diff 位于 `build/local/r1-yyt-vs-evb4.dtsi.diff`。

结论：**已验证**Yyt 文件相对标准 EVB4 基础文件包含 26 个差异块，不能按“仅一个 GPIO 修正”处理；但差异仍被严格限定在一个 R1 专有板级 include 文件内。下一步应先从 diff 中提取新增/删除的节点标题和 `status` 变更，按供电、存储、网络、显示、USB、音频等类别归类，而不是逐块解释全部属性。

下一步：只从保存的 diff 中提取结构节点行和 `status` 属性变更的前 120 行，确认改动涉及哪些硬件类别；不构建、不烧录。

### 步骤 69：建立 R1 专有 DTS 的初步硬件差异地图

目的：从 26 个差异块中先识别被修改的硬件类别，优先保护启动、网络和 NPU 所依赖的基础节点，不陷入全部属性的逐行阅读。

执行端：Arch 主机。学习者从 `build/local/r1-yyt-vs-evb4.dtsi.diff` 只提取新增/删除的结构节点与 `status` 行；未修改源码、未构建、未烧录。

实际输出（节选）：

```text
+        fan_ctrl{//0c7
+        usb30_power_ver1{//1c7
+        usb30_power{//0 B0
+&backlight1 {
+&can2 {
+&u2phy3_host {
+&usbhost_dwc3_0 {
+&sata2{
+&pcie2x1l1 {
+&pwm7{
+&pwm12 {
+&route_dp0 {
+&uart9 {
```

同一输出还显示多处显示/DSI/DP/HDMI 路由、I²C7、USB host、PCIe、SATA、PWM、CAN、UART 和厂商 GPIO 节点的状态变化；未出现 RKNPU/NPU 节点。若干新增行写作 `status = "disalbed"`，该拼写不是标准 `"disabled"`；但因为 Linux 只把 `"okay"`/`"ok"` 视为可用，当前只能推测其效果仍为不启用，不能据此直接修正或删除厂商配置。

结论：**已验证**Yyt 文件包含真实板级外设与显示路由差异，不能直接被标准 EVB4 文件取代；在已提取的结构节点中没有直接 NPU 配置差异。下一步应只核对 eMMC/SD 控制器、GMAC 网口和 NPU 标签是否出现在 Yyt 文件中；这三个类别最直接影响可启动性、当前 SSH 联通与 LLM 推理基础，仍不构建、不烧录。

下一步：在 Yyt 文件中只列出 `sdhci`、`sdmmc`、`gmac0`、`gmac1`、`rknpu`、`npu` 标签或节点引用的行号；不读取完整文件、不构建、不烧录。

### 步骤 70：确认 R1 DTS 移植所需的依赖粒度

目的：验证“只复制 R1 专有 Yyt 文件”是否足以在官方 5.10 BSP 中复用 R1 板级配置，避免把依赖于旧基础树的 include 直接放入官方树。

执行端：Arch 主机。学习者以 `cmp -s` 逐一比较本地 R1 树和固定官方候选中的三个同名基础 `.dtsi`；未修改、构建或烧录。

实际输出：

```text
rk3588s-evb4-lp4x.dtsi: different
rk3588-linux.dtsi: different
rk3588-android.dtsi: different
```

结论：**已验证**三个同名基础 include 在两棵树中均有内容差异。因此不能把 `rk3588s-evb4-lp4x-yyt.dtsi` 单独复制到官方树后假定其仍与上游基础文件兼容。后续直接移植的正确范围是 R1 DTS 的完整 include 依赖包，并且必须放在独立工作树/新 DTB 名称中进行编译验证；不能覆盖官方 EVB4 文件，也不能在未验证的情况下烧录。

下一步：只读取 R1 `rk3588s-yyt.dts` 和其三个直接 include 的首段 `#include`，确定完整依赖包的第一层清单；不复制、不构建、不烧录。

### 步骤 71：读取 R1 DTS 移植包的第一层 include

目的：识别 R1 顶层 DTS 实际采用的板级基础，而不是假定它以标准 EVB4 文件为父级。

执行端：Arch 主机。学习者仅提取 R1 顶层 DTS、Yyt 板级文件、标准 EVB4 文件和 Linux 文件中的 `#include` 行；未修改、构建或烧录。

实际输出：

```text
== rk3588s-yyt.dts ==
9:#include "rk3588s-evb4-lp4x-yyt.dtsi"
10:#include "rk3588-linux.dtsi"
== rk3588s-evb4-lp4x-yyt.dtsi ==
7:#include "dt-bindings/usb/pd.h"
8:#include "rk3588s.dtsi"
9:#include "rk3588s-evb-yyt.dtsi"
10:#include "rk3588-rk806-single.dtsi"
13:#include "rk3588s-os04a10-camera.dtsi"
14:#include "rk3588s-lcd-yyt.dtsi"
== rk3588s-evb4-lp4x.dtsi ==
7:#include "dt-bindings/usb/pd.h"
8:#include "rk3588s.dtsi"
9:#include "rk3588s-evb.dtsi"
10:#include "rk3588-rk806-single.dtsi"
```

`rk3588-linux.dtsi` 没有直接 C-preprocessor include。

结论：**已验证**R1 顶层选择的 Yyt 文件并不 include 标准 `rk3588s-evb4-lp4x.dtsi`；它改用 `rk3588s-evb-yyt.dtsi`，并额外引入 OS04A10 相机与 Yyt LCD 描述。因此 R1 是一套替代性的板级 DTS 组合，不是把标准 EVB4 文件加以覆盖。下一步以 C 预处理器递归生成依赖清单，作为直接移植到独立工作树的精确输入来源；不手工猜测文件闭包。

下一步：以主机 C 预处理器只生成 `rk3588s-yyt.dts` 的递归头文件依赖清单，保存到 `build/local/`；不复制、不构建、不烧录。

### 步骤 72：生成 R1 DTS 的递归移植输入清单

目的：得到 R1 顶层 DTS 实际递归依赖的板级源码文件闭包，作为在官方 5.10 独立工作树中直接移植的精确输入，而不是手工挑选文件。

执行端：Arch 主机。学习者以 `gcc -E -M -nostdinc -x assembler-with-cpp` 和 R1 DTS、DTS 根目录、内核 include 三个 include 路径生成依赖到 `build/local/r1-dts-dependencies.mk`，再筛选 `.dts`/`.dtsi` 后缀；未复制、构建或烧录。

实际输出：

```text
rk3588-linux.dtsi
rk3588-rk806-single.dtsi
rk3588s.dtsi
rk3588s-evb4-lp4x-yyt.dtsi
rk3588s-evb-yyt.dtsi
rk3588s-lcd-yyt.dtsi
rk3588s-os04a10-camera.dtsi
rk3588s-pinctrl.dtsi
rk3588s-yyt.dts
rockchip-pinconf.dtsi
```

结论：**已验证**R1 顶层 DTS 的递归 `.dts`/`.dtsi` 闭包在本次预处理条件下为 10 个文件；`dt-bindings` 头文件由内核 include 路径提供，不属于板级 DTS 复制清单。下一步创建固定官方 5.10 commit 的独立、初始不检出的 sparse Git worktree；后续复制和编译试验只在该工作树进行，原官方参考树与 R1 基线不被覆盖。

下一步：创建固定官方 commit 的独立 sparse Git worktree（初始不检出），只包含后续 DTS/驱动/配置对照目录；不复制 DTS、不构建、不烧录。

### 步骤 73：创建隔离的 R1 DTS 移植工作树

目的：为 R1 DTS 直接移植提供可独立丢弃的官方 0.9.8 内核工作空间，避免覆盖原官方参考树或本地 R1 5.10 基线。

执行端：Arch 主机。学习者从固定 commit 的 partial clone 以 `git worktree add --no-checkout -b study/r1-dts-port` 创建新工作树，再在该工作树初始化 sparse checkout，仅检出 DTS、配置和 RKNPU 对照目录；未复制 DTS、未构建、未烧录。

实际输出：

```text
## study/r1-dts-port
```

结论：**已验证**`src/rockchip-linux-kernel-r1-dts-port` 已作为分支 `study/r1-dts-port` 建立，初始状态干净。后续 R1 DTS 文件只可复制到此工作树；若验证失败可删除该 worktree/分支，不影响原官方候选或开发板。

下一步：把步骤 72 的 10 个 R1 DTS 输入逐个复制到此隔离工作树并逐个字节比较；不构建、不烧录。

### 步骤 74：将 R1 DTS 依赖包复制到隔离工作树

目的：把步骤 72 确认的 R1 DTS 闭包直接带入官方 0.9.8 内核候选的隔离工作树，为后续独立 DTB 编译做准备。

执行端：Arch 主机。学习者逐个 `cp` 10 个 R1 DTS 输入到 `study/r1-dts-port` 的对应 DTS 目录，并在每次复制后以 `cmp -s` 核验；未构建、未烧录。

实际保留输出：

```text
verified: rockchip-pinconf.dtsi
 M arch/arm64/boot/dts/rockchip/rk3588-linux.dtsi
 M arch/arm64/boot/dts/rockchip/rk3588-rk806-single.dtsi
 M arch/arm64/boot/dts/rockchip/rk3588s-pinctrl.dtsi
 M arch/arm64/boot/dts/rockchip/rk3588s.dtsi
 M arch/arm64/boot/dts/rockchip/rockchip-pinconf.dtsi
?? arch/arm64/boot/dts/rockchip/rk3588s-evb-yyt.dtsi
?? arch/arm64/boot/dts/rockchip/rk3588s-evb4-lp4x-yyt.dtsi
?? arch/arm64/boot/dts/rockchip/rk3588s-lcd-yyt.dtsi
?? arch/arm64/boot/dts/rockchip/rk3588s-os04a10-camera.dtsi
?? arch/arm64/boot/dts/rockchip/rk3588s-yyt.dts
```

学习者消息仅保留最后一个 `verified:` 行；但 Git 状态恰好列出 5 个预期被替换的已有文件与 5 个预期新增的 Yyt/R1 文件。除 `rockchip-pinconf.dtsi` 外的逐文件 `cmp` 成功行未在本记录中保留，故不能把全部字节核验写为已独立留证。

结论：**已验证**完整 R1 DTS 依赖包已复制到隔离工作树，且文件集合与预处理器闭包相符。下一步只读取该树 DTS Makefile 中的 RK3588 DTB 列表，确定增加 `rk3588s-yyt.dtb` 的最小构建入口；尚未编译，更未烧录。

下一步：只检索隔离工作树 DTS Makefile 中已有的 RK3588S DTB 条目；不修改、不构建、不烧录。

### 步骤 75：注册隔离工作树中的 R1 DTB 构建目标

目的：使官方 5.10 内核的 DTS 构建规则能以独立目标名处理已移植的 `rk3588s-yyt.dts`，同时保留原有 EVB4 DTB 目标以便对照和回退。

执行端：Arch 主机。先只读检索 DTS Makefile，确认现有 `rk3588s-evb4-lp4x-v10.dtb` 与 `-linux` 条目；随后仅在隔离 `study/r1-dts-port` 工作树增加：

```make
dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3588s-yyt.dtb
```

实际 diff：

```diff
 dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3588s-evb4-lp4x-v10.dtb
 dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3588s-evb4-lp4x-v10-linux.dtb
+dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3588s-yyt.dtb
 dtb-$(CONFIG_ARCH_ROCKCHIP) += rk3588s-evb8-lp4x-v10.dtb
```

`git diff --check -- arch/arm64/boot/dts/rockchip/Makefile` 无输出。对整个移植工作树运行同一检查时，只报告 R1 原始导入文件 `rockchip-pinconf.dtsi` 的 `new blank line at EOF`；为保持来源文件逐字忠实，本步骤未改动该文件。

结论：**已验证**隔离工作树已具备独立 `rk3588s-yyt.dtb` 构建目标，且没有覆盖标准 EVB4 目标。下一步先读取官方候选树的顶层构建输入是否足以只构建 DTB；当前 sparse checkout 尚未包含完整内核构建依赖，不能直接假定 `make` 可运行。

下一步：只检查隔离工作树当前 sparse-checkout 路径和内核 DTB 构建所需的顶层 `Makefile`、`scripts/`、`include/`、`arch/arm64/Makefile` 是否存在；不构建、不烧录。

### 步骤 76：核对隔离工作树的完整内核 DTB 构建前置

目的：确认当前 partial/sparse 工作树能否直接调用内核 `make` 构建 DTB，避免因缺少通用构建目录而将下载/配置问题误判为 DTS 移植问题。

执行端：Arch 主机。学习者读取 sparse-checkout 清单，并检查顶层 `Makefile`、`scripts/`、`include/`、`arch/arm64/Makefile` 的存在性；未修改、构建或烧录。

实际输出：

```text
== sparse paths ==
arch/arm64/boot/dts/rockchip
arch/arm64/configs
drivers/rknpu
== DTB build prerequisites ==
present: Makefile
absent:  scripts
absent:  include
present: arch/arm64/Makefile
```

结论：**已验证**当前 sparse checkout 不具备直接执行完整内核 `make` 的条件；`scripts/` 和 `include/` 未检出。此时扩展至完整内核树会带来不必要网络下载，也不能提高 DTS 移植结论的质量。下一步使用主机 GCC 预处理器加 `dtc`，对隔离树中的 R1 DTS 做最小 DTB 生成验证；DT bindings 临时从已验证存在的 R1 源码 `include/` 提供。该验证只证明 DTS 预处理/编译闭包可用，不等同于内核可启动。

下一步：在主机对隔离树 `rk3588s-yyt.dts` 执行预处理和 `dtc` 编译，生成 `build/local/` DTB 并检查文件类型与哈希；不构建内核、不烧录。

### 步骤 77：最小编译隔离工作树中的 R1 DTB

目的：验证完整 R1 DTS 依赖包在隔离官方 5.10 工作树中能通过 C 预处理和 DTC 生成二进制设备树，先排除 include/标签/语法闭包错误。

执行端：Arch 主机。学习者使用 `gcc -E -P -nostdinc -undef -D__DTS__ -x assembler-with-cpp`，优先从隔离工作树解析 DTS、从本地 R1 源码 `include/` 提供 dt-bindings，随后调用 `dtc -I dts -O dtb`。生成物位于 `build/local/r1-dts-port/`；未构建内核、未烧录。

实际输出（警告节选）：

```text
Warning (alias_paths): /aliases: aliases property name must include only lowercase and '-'
```

命令继续输出：

```text
.../rk3588s-yyt-port.dtb: Device Tree Blob version 17, size=149607,
boot CPU=0, string block size=7319, DT structure block size=142232
dc3d5eafef03768c43a564a511caf6abf6a676ca063edfcd3042b981256b331d  .../rk3588s-yyt-port.dtb
```

结论：**已验证**隔离树中的 R1 DTS 可生成有效 FDT v17 DTB，大小 149,607 bytes，SHA-256 为 `dc3d5eafef03768c43a564a511caf6abf6a676ca063edfcd3042b981256b331d`。`alias_paths` 是 DTC 警告，未阻止输出生成；它反映来源 DTS 中存在不满足 DTC 严格命名建议的 alias 属性，当前不修改，且不能据此判断是否影响运行时。该结果仅验证 DTS 预处理/编译闭包，**不**证明官方 0.9.8 内核可启动、eMMC/网口可用或 NPU 可推理。

下一步：只反编译生成 DTB 并检索 `mmc@fe2e0000`、`ethernet@fe1c0000`、`npu@fdab0000` 三个节点的上下文，验证关键硬件节点仍被包含；不构建内核、不烧录。

### 步骤 78：验证隔离 R1 DTB 的关键启动、网络与 NPU 节点

目的：在进入完整内核构建前，确认移植生成的 DTB 至少保留当前项目不可缺少的 eMMC 启动控制器、GMAC 网口和 RKNPU 节点，并且没有将它们禁用。

执行端：Arch 主机。学习者以 `fdtget -t s` 直接读取步骤 77 生成 DTB 的 `compatible` 与 `status`；未构建内核、未烧录。

实际输出：

```text
== /mmc@fe2e0000 ==
compatible: rockchip,rk3588-dwcmshc rockchip,dwcmshc-sdhci
status: okay
== /ethernet@fe1c0000 ==
compatible: rockchip,rk3588-gmac snps,dwmac-4.20a
status: okay
== /npu@fdab0000 ==
compatible: rockchip,rk3588-rknpu
status: okay
```

结论：**已验证**隔离生成 DTB 保留并启用 eMMC、GMAC 和 RKNPU 三个关键节点，且 compatible 与当前 R1 运行时已观察的驱动绑定方向一致。这是静态设备树验证，不证明实际硬件初始化、网络通信、NPU 执行或内核启动成功。下一步为完整官方 5.10 内核构建准备源码：解除隔离工作树的 sparse checkout 以取得构建所需文件；该操作将产生较大主机网络下载，但不接触开发板。

下一步：在确认主机磁盘空间后，解除隔离 worktree 的 sparse checkout 并记录目录大小和 Git 状态；不构建、不烧录。

### 步骤 79：为完整官方 5.10 内核构建展开隔离源码树

目的：取得完整官方 `develop-5.10` 源码，以支持后续 Kconfig、DTS 和内核映像构建，同时保留隔离工作树中的 R1 DTS 移植改动。

执行端：Arch 主机。学习者解除 `study/r1-dts-port` 的 sparse checkout；随后检查 `scripts/`、`include/`、`drivers/`、`kernel/`、`init/` 和目录大小，并查看 Git 状态；未执行内核构建、未烧录。

实际输出：

```text
present: scripts
present: include
present: drivers
present: kernel
present: init
致命错误：这个工作区不是稀疏的
sparse checkout disabled
1.4G    /home/loser/Study/rk3588/src/rockchip-linux-kernel-r1-dts-port
```

学习者此前的 Git 状态仍显示 `study/r1-dts-port` 分支及 R1 DTS 的 5 项修改、5 项新增；本次命令未报告覆盖或丢失这些文件。

结论：**已验证**隔离工作树已拥有完整官方 5.10 源码，大小 1.4 GiB，且 sparse checkout 已禁用。`git sparse-checkout list` 的“不是稀疏的”是预期状态提示；后接的 fallback 输出 `sparse checkout disabled` 用于明确结果，不是新的失败。下一步只检查该树的 RK3588 Linux 配置输入和关键 RKNPU/启动配置符号；不配置、不编译、不烧录。

下一步：只读取 `rockchip_linux_defconfig` 与 `rk3588_linux.config` 的位置、内容和 RKNPU/eMMC/GMAC 关键符号；不配置、不编译、不烧录。

### 步骤 80：确认官方 ARM64 RK3588 Linux 配置基线

目的：确定完整官方 5.10 树是否已有 NPU、eMMC 与网络相关配置输入，并区分主 defconfig 和可选 RK3588 附加片段，避免把无关 Wi-Fi/GPU 选项混入首个启动验证配置。

执行端：Arch 主机。学习者仅搜索并读取 `rockchip_linux_defconfig`、`rk3588_linux.config` 中的关键符号；未生成 `.config`、未编译、未烧录。

实际输出（ARM64 相关部分）：

```text
== arch/arm64/configs/rockchip_linux_defconfig ==
CONFIG_ARCH_ROCKCHIP=y
CONFIG_STMMAC_ETH=y
CONFIG_MMC_SDHCI=y
CONFIG_MMC_DW=y
CONFIG_MMC_DW_ROCKCHIP=y
CONFIG_ROCKCHIP_RKNPU=y

== arch/arm64/configs/rk3588_linux.config ==
# CONFIG_BCMDHD_SDIO=y is not set
CONFIG_BCMDHD_PCIE=y
CONFIG_MALI_CSF_SUPPORT=y
```

输出还匹配到 `arch/arm/configs/rockchip_linux_defconfig`，但后续构建固定 `ARCH=arm64`，该 32 位配置不参与本次配置。

结论：**已验证**官方 ARM64 `rockchip_linux_defconfig` 显式启用 RKNPU、STMMAC 核心和 Rockchip eMMC 控制器。`rk3588_linux.config` 是位于 `arch/arm64/configs/` 的三行附加片段，涉及 PCIe Broadcom Wi-Fi 和 Mali CSF，而非当前 NPU/eMMC 首次启动验证的必要前置，暂不合入。defconfig 未显式列出 `CONFIG_DWMAC_ROCKCHIP`，但其最终值需经 Kconfig 依赖解析后的 `.config` 确认，不能由缺少一行直接下结论。

下一步：在独立 `build/` 输出目录执行仅配置的 `rockchip_linux_defconfig`，再读取解析后 `.config` 的 NPU/eMMC/GMAC 符号；不编译内核、不烧录。

### 步骤 81：生成并验证官方 5.10 的 R1 移植配置

目的：让 Kconfig 解析官方 ARM64 `rockchip_linux_defconfig` 的依赖，确认首次 R1 移植构建同时启用 RKNPU、其 DRM GEM ABI、eMMC、GMAC/Realtek PHY 与 DRM。

执行端：Arch 主机。学习者在新的 `build/kernel-r1-dts-port` 输出目录执行 `make -C ... O=... ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- rockchip_linux_defconfig`，随后只读取输出 `.config` 的关键符号；未编译内核、未烧录。

实际输出：

```text
#
# configuration written to .config
#
CONFIG_STMMAC_ETH=y
CONFIG_DWMAC_ROCKCHIP=y
CONFIG_REALTEK_PHY=y
CONFIG_DRM=y
CONFIG_MMC_SDHCI=y
CONFIG_MMC_DW=y
CONFIG_MMC_DW_ROCKCHIP=y
CONFIG_ROCKCHIP_RKNPU=y
CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y
```

结论：**已验证**Kconfig 最终配置启用当前 R1 启动、网口和 NPU 所需的关键符号；先前 defconfig 中未显式列出的 `DWMAC_ROCKCHIP` 已解析为 `y`。`rk3588_linux.config` 的 Wi-Fi/Mali 附加项未参与本次输出目录，首个 NPU 目标不依赖它们。下一步使用内核 Kbuild 仅构建 `rk3588s-yyt.dtb`，验证移植 DTS 能通过实际内核构建流程；不构建 Image、不烧录。

下一步：在该输出目录使用 Kbuild 仅构建 `rk3588s-yyt.dtb`，检查产物类型与哈希；不构建内核 Image、不烧录。

### 步骤 82：首次调用 Kbuild 的 R1 DTB 目标

目的：使用实际内核 Kbuild/DTC 流程构建已注册的 R1 DTB，验证 Makefile 目标解析方式。

执行端：Arch 主机。学习者在已配置的输出目录调用裸目标 `rk3588s-yyt.dtb`；未构建 Image、未烧录。

实际输出：

```text
UPD     include/config/kernel.release
make[2]: *** 没有规则可制作目标“arch/arm64/boot/dts/rk3588s-yyt.dtb”。 停止。
make[1]: *** ...：rk3588s-yyt.dtb] 错误 2
```

结论：**已验证**Kbuild 已进入 ARM64 DTB 目标解析，但裸文件名被解析为 `arch/arm64/boot/dts/rk3588s-yyt.dtb`，省略了实际的 `rockchip/` 子目录。这是构建目标路径错误，不是 DTS 预处理、DTC、RKNPU 或 Kconfig 错误。下一步仅改用完整目标路径 `arch/arm64/boot/dts/rockchip/rk3588s-yyt.dtb` 重试；不改源码、不构建 Image、不烧录。

下一步：以完整 `rockchip/` 相对路径调用 Kbuild 构建 R1 DTB，检查产物类型和哈希；不构建 Image、不烧录。

### 步骤 83：修正 Kbuild DTB 目标的路径层级

目的：验证 Kbuild 对 ARM64 DTB 目标路径的自动前缀规则，避免将 `arch/arm64/boot/dts/` 重复写入目标名。

执行端：Arch 主机。学习者以 `arch/arm64/boot/dts/rockchip/rk3588s-yyt.dtb` 作为 make 目标重试；未构建 Image、未烧录。

实际输出：

```text
make[2]: *** 没有规则可制作目标
“arch/arm64/boot/dts/arch/arm64/boot/dts/rockchip/rk3588s-yyt.dtb”。 停止。
```

结论：**已验证**Kbuild 前端会自动添加 `arch/arm64/boot/dts/` 前缀：裸文件名缺少 `rockchip/` 子目录，完整路径则使此前缀重复。正确目标应是相对此目录的 `rockchip/rk3588s-yyt.dtb`。本次同样未进入 DTS 编译，不涉及源码、配置、驱动或硬件错误。

下一步：以 `rockchip/rk3588s-yyt.dtb` 作为 Kbuild 目标构建 R1 DTB，检查产物类型和哈希；不构建 Image、不烧录。

### 步骤 84：通过官方 Kbuild 成功构建 R1 DTB

目的：以完整官方 5.10 构建系统、RKNPU 0.9.8 配置和隔离移植的 R1 DTS 生成 DTB，验证 DTS 已通过真实内核构建规则。

执行端：Arch 主机。学习者以目录相对 Kbuild 目标 `rockchip/rk3588s-yyt.dtb` 构建，未构建 Image、未烧录。

实际输出：

```text
.../rk3588s-yyt.dtb: Device Tree Blob version 17, size=233459,
boot CPU=0, string block size=20211, DT structure block size=213192
a76fbacb8ea37230be1dfb4d08762017b09cfa8a36516b0f668057f55268949d  .../rk3588s-yyt.dtb
```

结论：**已验证**隔离 R1 DTS 已通过官方 `develop-5.10` 的实际 Kbuild/DTC 流程，生成 FDT v17、233,459 bytes 的 DTB，SHA-256 为 `a76fbacb8ea37230be1dfb4d08762017b09cfa8a36516b0f668057f55268949d`。其大小大于步骤 77 的手动 `dtc` 产物（149,607 bytes）；这种差异可能来自 Kbuild 按配置加入的符号/元数据，尚待读取验证，不能直接假定二者等价或任一错误。下一步只检查 Kbuild DTB 是否含 `__symbols__` 及关键节点状态；不构建 Image、不烧录。

下一步：只读取 Kbuild 输出 `.config` 的设备树符号相关选项、Kbuild DTB 根节点子节点和 eMMC/GMAC/RKNPU 的 `compatible`/`status`；不构建 Image、不烧录。

### 步骤 85：解释 Kbuild DTB 符号表并复核关键节点

目的：确认 Kbuild DTB 相比手动 `dtc` 产物增大的原因，并再次检查 R1 启动与 NPU 所需的三个基础节点。

执行端：Arch 主机。学习者只读取独立输出目录 `.config` 与已生成的 DTB；未改动源码、未构建 Image、未烧录。

实际输出：

```text
== DTS symbol options ==
CONFIG_DTC_SYMBOLS=y
# CONFIG_OF_OVERLAY is not set
== root special nodes ==
__symbols__
== /mmc@fe2e0000 ==
compatible: rockchip,rk3588-dwcmshc rockchip,dwcmshc-sdhci
status: okay
== /ethernet@fe1c0000 ==
compatible: rockchip,rk3588-gmac snps,dwmac-4.20a
status: okay
== /npu@fdab0000 ==
compatible: rockchip,rk3588-rknpu
status: okay
```

结论：**已验证**此次 Kbuild DTB 启用了 `CONFIG_DTC_SYMBOLS`，并含有根节点 `__symbols__`；这解释了其比步骤 77 手动 DTB 更大的主要元数据来源。`CONFIG_OF_OVERLAY` 未启用，当前没有 overlay 功能作为这一事实的前置。Kbuild 产物中的 eMMC、GMAC、RKNPU 均仍具预期 compatible 且 `status = "okay"`。这证明 DTS 与该官方构建配置在静态层面一致，**不证明**内核 Image 已成功构建、该 DTB 可启动或 NPU 已恢复推理。

下一步：只在主机的隔离输出目录构建 `Image`；构建完成后检查 ELF/镜像类型、大小、哈希和 RKNPU 0.9.8 版本字符串。仍不封装 FIT、不写入板子。

### 步骤 86：首次构建官方 5.10 的 R1 ARM64 内核映像

目的：验证隔离的官方 `develop-5.10` 源码、R1 DTS 移植与已解析配置可共同完成 Linux ARM64 内核映像构建。

执行端：Arch 主机。学习者在 `build/kernel-r1-dts-port` 输出目录执行 `make ... -j(nproc) Image`；未封装 FIT、未写入开发板。

实际输出末尾：

```text
LD      vmlinux
SORTTAB vmlinux
SYSMAP  System.map
OBJCOPY arch/arm64/boot/Image
make: 离开目录“/home/loser/Study/rk3588/src/rockchip-linux-kernel-r1-dts-port”
```

结论：**已验证**该隔离树已成功链接 `vmlinux`、生成 `System.map`，并导出 ARM64 裸内核映像 `arch/arm64/boot/Image`。这是一次完整的主机侧内核构建成功，不是板端可启动性、FIT 封装完整性或 NPU 推理成功证据。下一步只读取 `Image`/`vmlinux` 的身份、大小和哈希，并从 `vmlinux` 核对 RKNPU 0.9.8 版本字符串；不封装 FIT、不烧录。

下一步：只读取 `Image`/`vmlinux` 的身份、大小和 SHA-256，并核对 RKNPU driver 版本字符串；不封装 FIT、不写入板子。

### 步骤 87：核验构建产物与内建 RKNPU 版本

目的：区分可由 bootloader 使用的裸 `Image` 和供分析的 `vmlinux`，并确认本次构建实际编入 RKNPU 0.9.8。

执行端：Arch 主机。学习者只读取隔离输出目录的产物属性、哈希与字符串；未封装 FIT、未写入开发板。

实际输出：

```text
.../arch/arm64/boot/Image: Linux kernel ARM64 boot executable Image, little-endian, 4K pages
.../vmlinux: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV), statically linked, BuildID[sha1]=9da81f27dc6617d3889263bb1ecda942a292354c, with debug_info, not stripped
Image size=37675520 bytes
vmlinux size=402889456 bytes
e39e443ccff8b670ece4caa3149a6c0272b34c723fca7fe5492e8fff918726d3  Image
22fb59c90c0cdb6f3e4d36d38290deabfbc99af836f03cfb81bec3f4768f2b2d  vmlinux
0.9.8
RKNPU driver
20240828
```

结论：**已验证**`Image` 为 37,675,520-byte ARM64 Linux boot Image，`vmlinux` 为带 debug 信息的 402,889,456-byte AArch64 ELF；二者 SHA-256 已保存。`vmlinux` 内含 `RKNPU driver`、`0.9.8` 与日期 `20240828` 字符串，因此本次构建确实使用官方候选的 RKNPU 0.9.8，而非旧 R1 0.8.2。它仍只是主机产物：现有 R1 启动链使用厂商 FIT，不能把裸 `Image` 直接写入 eMMC 任一分区。

下一步：只读读取当前板端 `boot` 分区的文件类型、大小与头部，并在主机确认是否存在 `mkimage`/`dumpimage`；据此确定现有 FIT 封装与验证入口。不得写分区、不得烧录。

### 步骤 88：识别当前 R1 boot 分区的 FIT 控制树

目的：确认当前板端实际 boot 分区和开头封装格式，防止把新建的裸 `Image` 错写为可启动载荷。

执行端：R1 root Shell。学习者通过 GPT `PARTLABEL=boot` 自动定位分区后，仅读取文件类型、长度和前 64 字节；未写入 eMMC。

实际输出：

```text
boot device: /dev/mmcblk0p3
/dev/mmcblk0p3: Device Tree Blob version 17, size=1536, boot CPU=0,
string block size=190, DT structure block size=1004
67108864
d0 0d fe ed 00 00 06 00 ...
```

结论：**已验证**当前启动使用的 `boot` 分区为 `/dev/mmcblk0p3`，容量 67,108,864 B（64 MiB）。其首 4 字节为 FDT/FIT 魔数 `d0 0d fe ed`，`file -s` 将开头的控制树识别为 1,536 B DTB。结合此前 U-Boot 的 FIT 启动日志，可确定这个开头是 FIT 的控制 FDT，而非 Linux 运行时 DTB 或可直接替换的裸内核；实际 kernel、DTB 等 FIT 子镜像应由该控制树引用。下一步仍需确认主机 `mkimage`/`dumpimage` 工具可用，再读取 FIT 结构与签名信息；不提取、不修改、不写入分区。

下一步：只确认主机的 `mkimage`/`dumpimage` 是否存在；若存在，再从只读副本列出 FIT 的 images/configurations、哈希和签名信息；不写入板子。

### 步骤 89：确认主机缺少 FIT 检查工具并定位 Arch 包

目的：为只读检查当前 boot FIT 准备 U-Boot 标准工具，不将缺少命令误解为内核构建或板端问题。

执行端：Arch 主机。学习者确认 `mkimage` 与 `dumpimage` 都不在 `PATH`；随后以只读 `pacman -Si uboot-tools` 查询仓库元数据，未安装软件。

实际输出：

```text
mkimage: missing
dumpimage: missing
Repository      : extra
Name            : uboot-tools
Version         : 2026.07-1
Description     : U-Boot bootloader utility tools
Download Size   : 235.64 KiB
Installed Size  : 874.63 KiB
Validated By    : SHA-256 Sum  Signature
```

结论：**已验证**主机当前缺少 FIT 专用检查命令；Arch `extra/uboot-tools` 包可用，版本 `2026.07-1`，将提供 U-Boot 实用工具。安装它只改变主机工具环境，不接触 R1 eMMC、FIT 或源码树。安装后先以 `command -v` 和 `-V` 确认，再进行 FIT 的只读解析。

下一步：安装 Arch `uboot-tools`，确认 `mkimage`/`dumpimage` 的路径和版本；不读取或写入板子。

### 步骤 90：准备 FIT 只读解析工具

目的：确认主机已具备 U-Boot FIT 的标准检查工具，为后续解析当前 boot 分区副本做准备。

执行端：Arch 主机。学习者安装 `uboot-tools` 后执行版本查询；未读取或写入板子。

实际输出：

```text
mkimage version 2026.07
dumpimage version 2026.07
```

结论：**已验证**主机的 `mkimage`/`dumpimage` 已可用，均为 2026.07。下一步将只读复制当前 R1 `/dev/mmcblk0p3` 到被 Git 忽略的 `build/local/` 分析区，先核对完整 64 MiB 长度和 SHA-256；不会修改板端分区。

下一步：只读复制当前 R1 `boot` 分区到主机分析区，核验 64 MiB 长度和 SHA-256；不解析、不写入板子。

### 步骤 91：保存并校验当前 R1 boot FIT 分区副本

目的：将当前官方镜像的 boot 载荷固定为主机侧分析输入，避免后续解析重复读取板端，更避免在没有可验证载荷前写入分区。

执行端：Arch 主机经 SSH 从 R1 仅读取 `/dev/mmcblk0p3` 的 16 个 4 MiB 块，并写入 Git 忽略的 `build/local/r1-20260816/`；未写入 R1。

实际输出：

```text
path=build/local/r1-20260816/r1-boot-p3.img; size=67108864 bytes; type=一般文件
e983740d4df29d51fa58dea9d504d536b87c8b205935ec2ceb8d64e679cd833b  build/local/r1-20260816/r1-boot-p3.img
```

结论：**已验证**当前 boot 分区副本长度精确为 67,108,864 B，与步骤 88 的分区容量一致；SHA-256 为 `e983740d4df29d51fa58dea9d504d536b87c8b205935ec2ceb8d64e679cd833b`。该文件是之后 FIT 结构分析的唯一输入；本步骤没有解析、修改或写入开发板。

下一步：仅以 `dumpimage -l` 列出该本地 FIT 副本的 images/configurations、载荷大小、哈希和签名信息；不提取载荷、不写入板子。

### 步骤 92：列出当前 boot FIT 的载荷与配置

目的：以 U-Boot 标准 FIT 工具确定当前启动配置引用的 kernel、FDT、resource 以及完整性/签名声明，为后续仅主机侧重建可行性分析建立基线。

执行端：Arch 主机。学习者对步骤 91 的本地副本执行 `dumpimage -l`；未提取载荷、未写入 R1。

实际输出要点：

```text
FIT description: U-Boot FIT source file for arm
Created:         Sun Sep 29 11:10:28 2024
Image 0 (fdt):      147826 B, sha256 abd1c6c3...fe87546
Image 1 (kernel):   35707392 B, sha256 5e8fc7f4...49ceeb
Image 2 (resource): 638976 B, sha256 492cbec9...6569c6
Default Configuration: 'conf'
Configuration 0 (conf)
  Kernel: kernel
  FDT: fdt
  Sign algo: sha256,rsa2048:dev
  Sign padding: pss
  Sign value: unavailable
```

结论：**已验证**当前 FIT 的默认 `conf` 引用 `kernel` 与 `fdt`，同时容纳 `resource` 子镜像；FDT 哈希与此前已记录的 FIT FDT `abd1c6c3…fe87546` 一致。当前 kernel 为 35,707,392 B，新构建 Image 为 37,675,520 B，单看三项数据量仍小于 64 MiB 分区，但最终 FIT 控制树、外部数据布局、对齐和签名仍未核验，因此不能据此判断新载荷可写入。配置声明 `sha256,rsa2048:dev` 和 PSS 签名；`Sign value: unavailable` 仅是本次 `dumpimage -l` 的呈现，不能推断签名不存在、可跳过，或本机已具备私钥。下一步须只读查看控制树中该签名节点及 images 的属性，确认签名材料、required 策略和外部数据位置；不提取、不修改、不写入板子。

下一步：只读取 FIT 控制树中 `/images/*` 与 `/configurations/conf` 的属性名、签名子节点属性名和外部数据位置；不提取、不写入板子。

### 步骤 93：确认 FIT 使用外置载荷与独立签名节点

目的：区分 FIT 控制树本身与分区中实际大载荷的位置，并确认默认启动配置引用范围和签名节点层级。

执行端：Arch 主机。学习者仅使用 `fdtget -p/-l` 列出步骤 91 本地副本的属性名和子节点；未读取二进制载荷、未写入 R1。

实际输出要点：

```text
/images: children = fdt kernel resource
/images/fdt: properties = data-size data-position type arch compression load
/images/fdt: children = hash
/images/kernel: properties = data-size data-position type arch os compression entry load
/images/kernel: children = hash
/images/resource: properties = data-size data-position type arch compression
/images/resource: children = hash
/configurations: properties = default
/configurations: children = conf
/configurations/conf: properties = rollback-index fdt kernel multi
/configurations/conf: children = signature
```

结论：**已验证**三项 FIT image 使用 `data-position`/`data-size` 指向外置载荷；控制 FDT 不内嵌大块 `data` 属性，这与其仅 1,536 B 一致。`conf` 明确包含 `fdt`、`kernel`、`multi` 和 `rollback-index`，其中 `multi` 与此前 `resource` multi-file image 一致；其下有独立 `signature` 节点。下一步只读取这些位置/大小、rollback index 与 signature 的可读字符串属性；不读取签名 value 的二进制内容、不提取、不写入板子。

下一步：只读取各 image 的位置/大小/装载属性、`conf` 的引用和 rollback index，以及 `signature` 的属性名和文本属性；不提取、不写入板子。

### 步骤 94：读取外置载荷布局与签名覆盖范围

目的：确定当前 FIT 外置载荷的精确偏移、配置引用和签名覆盖对象，并仅做新 Image 的分区容量上界计算。

执行端：Arch 主机。学习者使用 `fdtget` 读取步骤 91 FIT 副本的文本/整数属性；随后主机仅以已知数值计算结束位置；未提取或写入 R1。

实际输出要点：

```text
/images/fdt:      data-position=0x800,     data-size=0x24172
/images/kernel:   data-position=0x24a00,   data-size=0x220da00
/images/resource: data-position=0x2232400, data-size=0x9c000
fdt load=0xffffff00
kernel load=entry=0xffffff01
conf: kernel=kernel, fdt=fdt, multi=resource, rollback-index=0
signature:
  algo=sha256,rsa2048
  key-name-hint=dev
  padding=pss
  required=(absent)
  sign-images=fdt kernel multi

old-end=0x22ce400 (36496384 bytes)
new-kernel-end=0x2412c00 (37825536 bytes)
partition=0x4000000 (67108864 bytes)
remaining-after-new=29283328 bytes
```

结论：**已验证**当前 FIT 的外置数据按 `fdt → kernel → resource` 布局，三项原始数据结束于 `0x22ce400`。若只以新 Image（37,675,520 B）替换原 kernel、保持其起点 `0x24a00`，新 kernel 结束于 `0x2412c00`，仍距 64 MiB 分区末尾有 29,283,328 B。因而**容量不是当前阻塞**；这不包含新 FIT 控制树大小、对齐策略或 resource 放置调整，不能当作可写入证明。

签名节点明确以 RSA-2048/PSS 对 `fdt kernel multi` 三项签名，密钥提示为 `dev`。其自身没有 `required` 属性；但 FIT/U-Boot 的最终验证策略还可能由 U-Boot 内嵌公钥/控制 DTB 决定，不能据此假设可绕过或不必重签。下一步只在主机检查当前 FIT signature 节点是否带二进制 `value`、其长度，并在当前 U-Boot 分区副本中查找 `dev` 公钥/required 线索；不构建新 FIT、不写入板子。

下一步：只读取当前 FIT 签名 value 的存在与长度，并从现有 U-Boot 分区只读副本检查 `dev` 公钥或 `required` 验证策略线索；不构建、不写入板子。

### 步骤 95：确认 boot FIT 未携带实际签名值并复核 U-Boot 分区

目的：区分“存在 signature 节点”与“实际携带可验证签名”，并在导出 U-Boot 前重新确认当前 GPT 的精确分区标签。

执行端：Arch 主机以 `fdtget` 只读查询本地 boot 副本；R1 root Shell 只读列出 GPT 分区。未构建、提取或写入 R1。

实际输出：

```text
FIT signature value bytes: Error at 'value': FDT_ERR_NOTFOUND
0
/dev/mmcblk0p1 4M uboot
/dev/mmcblk0p2 4M misc
/dev/mmcblk0p3 64M boot
...
```

结论：**已验证**`/configurations/conf/signature` 不存在 `value` 属性；其算法、padding、密钥提示和覆盖范围是声明，当前 boot FIT 没有内含实际 RSA 签名值。这也解释了步骤 92 `dumpimage` 显示 `Sign value: unavailable`。这不能单独证明板上 U-Boot 的验证策略：公钥及 `required` 标记可能存在于 U-Boot 内嵌控制 DTB，故不应据此尝试构造或写入新 FIT。

GPT 当前明确为 `/dev/mmcblk0p1` `4M` `uboot`、`/dev/mmcblk0p3` `64M` `boot`。下一步只读复制 4 MiB `uboot` 分区到同一主机分析目录，并记录大小/哈希；不解析、不写入板子。

下一步：只读复制当前 `/dev/mmcblk0p1` U-Boot 分区到主机分析区，核对 4 MiB 长度和 SHA-256；不解析、不写入板子。

### 步骤 96：保存并校验当前 R1 U-Boot 分区副本

目的：固定当前第二阶段 U-Boot 载荷为主机侧分析输入，以便后续只读检查其 FIT 与内嵌公钥策略。

执行端：Arch 主机经 SSH 从 R1 仅读取 `/dev/mmcblk0p1` 的 4 个 1 MiB 块，写入 Git 忽略分析目录；未写入 R1。

实际输出：

```text
path=build/local/r1-20260816/r1-uboot-p1.img; size=4194304 bytes; type=一般文件
3b09148574d57f9b76f8afb064dc21af6df2819e0c5ccf4bce18e08f56820001  build/local/r1-20260816/r1-uboot-p1.img
```

结论：**已验证**当前 U-Boot 分区副本长度精确为 4,194,304 B，与步骤 95 GPT 容量一致；SHA-256 为 `3b09148574d57f9b76f8afb064dc21af6df2819e0c5ccf4bce18e08f56820001`。之后的 U-Boot/FIT 验证策略分析仅针对该本地副本，不需要再访问板端原始分区。

下一步：仅以 `dumpimage -l` 列出本地 U-Boot 分区副本的早期 FIT image/configuration/哈希信息；不提取、不写入板子。

### 步骤 97：列出当前 U-Boot 早期 FIT 的组成

目的：确认 `/dev/mmcblk0p1` 的早期 FIT 是否包含用于检查 U-Boot 验证策略的 U-Boot FDT，并建立其 image 编号和哈希基线。

执行端：Arch 主机。学习者以 `dumpimage -l` 仅读取步骤 96 的本地副本；未提取、修改或写入 R1。

实际输出要点：

```text
FIT description: FIT Image with ATF/OP-TEE/U-Boot/MCU
Image 0 (uboot): 1297800 B, sha256 eb906a97...9e5a6a1
Image 1 (atf-1): 194076 B,  sha256 045b2cef...1eab1d
Image 2 (atf-2): 24576 B,   sha256 30812190...bb479
Image 3 (atf-3): 20480 B,   sha256 cb7bdbec...9d78a
Image 4 (optee): 461200 B,  sha256 fde08608...2ab12
Image 5 (fdt): 7829 B, Type: Flat Device Tree, sha256 c07f4a4d...6dc381
Default Configuration: 'conf'
Configuration 0 (conf): Firmware=atf-1, FDT=fdt, Loadables=uboot atf-2 atf-3 optee
Sign algo: sha256,rsa2048:dev
Sign value: unavailable
```

结论：**已验证**当前 `uboot` 分区是包含 ATF、OP-TEE、U-Boot 和独立 U-Boot DTB 的早期 FIT，配置 `conf` 选择 `atf-1` 作为 firmware、`fdt` 作为 DTB，并加载 U-Boot、其余 ATF 段与 OP-TEE。该 FDT 是 image 编号 5、大小 7,829 B、哈希 `c07f4a4d…6dc381`，可从本地主机副本安全提取以检查公钥/required 策略。早期 FIT 同样声明 `sha256,rsa2048:dev` 但无可见签名 value；这仍不等于验证策略已关闭。

下一步：只从本地 p1 副本提取 image 5 U-Boot DTB，核对其类型、大小、哈希；不写入板子。

### 步骤 98：提取并核验当前 U-Boot 控制 DTB

目的：从已固定的早期 FIT 副本中取得 U-Boot 实际使用的控制 DTB，作为公钥/required 验证策略的唯一可分析输入。

执行端：Arch 主机。学习者以 `dumpimage -T flat_dt -p 5` 从本地 p1 副本提取至 `build/local/`，并只读核对类型、大小、哈希；未写入 R1。

实际输出：

```text
Extracted Image 5 (fdt)
Data Size: 7829 Bytes
Hash value: c07f4a4d713c2dde198a1c4fc7a980a98f5dc97665e3171dc7c319d7846dc381
.../r1-uboot-control-fdt.dtb: Device Tree Blob version 17, size=7829
c07f4a4d713c2dde198a1c4fc7a980a98f5dc97665e3171dc7c319d7846dc381  r1-uboot-control-fdt.dtb
```

结论：**已验证**本地提取的 U-Boot 控制 DTB 为有效 FDT v17、7,829 B，SHA-256 与步骤 97 的 image 5 哈希精确一致。它不是 Linux 运行时 DTB，而是当前 U-Boot 的控制树；下一步仅列出根子节点与 `/signature`（若存在）的子节点和属性，检查公钥提示与 required 策略。

下一步：只读取 U-Boot 控制 DTB 的根子节点、`/signature` 子节点和各 key 节点属性名/文本属性；不修改、不写入板子。

### 步骤 99：排除当前 U-Boot 控制 DTB 中的 FIT 公钥节点

目的：检查当前第二阶段 U-Boot 是否在其控制 DTB 中嵌入了 FIT 验签公钥或 `required` 策略。

执行端：Arch 主机。学习者只列出步骤 98 控制 DTB 的根子节点与 `/signature` 子节点；未修改或写入 R1。

实际输出要点：

```text
== root children ==
aliases
firmware
...
chosen
secure-otp@fe3a0000
adc-keys
== /signature children ==
Error at '/signature': FDT_ERR_NOTFOUND
(no /signature node)
```

结论：**已验证**当前 U-Boot 控制 DTB 没有 `/signature` 节点，因此其中没有可供本次查询发现的 `key-dev` 公钥或 `required` 属性。结合步骤 95：boot FIT 的 signature 节点又没有实际 `value`，现有证据不支持“p3 boot FIT 的 `conf` 必须通过 RSA-2048 验签”这一假设。仍不能把它扩展成完整启动链永久不验签的断言：BootROM/SPL/早期 FIT 的其他策略、U-Boot 代码编译选项以及单独 image SHA-256 完整性校验仍存在。对 p3 的任何候选重建仍必须在主机先验证其 FIT 哈希与结构，且在具备恢复路径前不得写入。

下一步：只从步骤 91 本地 boot FIT 副本提取 fdt、kernel、resource 三项载荷并逐项核对其 FIT 声明的 SHA-256；不构建新 FIT、不写入板子。

### 步骤 100：首次提取 boot FIT 载荷并纠正 `dumpimage -T` 语义

目的：提取当前 boot FIT 的现有输入载荷用于主机侧重建分析，并验证 U-Boot 工具的容器类型参数语义。

执行端：Arch 主机。学习者从本地 p3 FIT 副本提取 image 0 成功；随后以 `-T kernel` 和 `-T multi` 提取 image 1/2 失败。未写入 R1。

实际输出要点：

```text
Image 0 (fdt) extracted; sha256 abd1c6c3...fe87546
dumpimage: failed to verify header of Default Image support
dumpimage: Can't extract subimage from .../r1-boot-p3.img
...
path=.../r1-boot-fdt.dtb; size=147826 bytes
abd1c6c3...fe87546  r1-boot-fdt.dtb
```

结论：**已验证**image 0 FDT 已成功提取，大小 147,826 B、SHA-256 `abd1c6c3…fe87546` 与 FIT 声明一致。image 1/2 的失败不是其载荷完整性证据：`dumpimage -T` 指定的是输入文件的整体容器格式，而本输入整体是 FIT/FDT，不能改写为子镜像的 `kernel` 或 `multi` 类型。应对三个子镜像均使用 `-T flat_dt -p <index>`；下一步仅以此正确参数提取尚不存在的 image 1/2 并哈希核对，不覆盖已成功的 FDT。

下一步：仅以 `dumpimage -T flat_dt -p 1/2` 从本地 FIT 副本提取 kernel/resource，并核对大小和 SHA-256；不构建新 FIT、不写入板子。

### 步骤 101：完整提取并校验当前 boot FIT 的三项载荷

目的：得到可用于主机侧 FIT 克隆验证的原始 FDT、kernel、resource 输入，并确认它们与当前分区控制树的哈希声明一致。

执行端：Arch 主机。学习者以正确的整体容器类型 `dumpimage -T flat_dt` 提取 image 1、2；image 0 已在步骤 100 提取。未构建新 FIT、未写入 R1。

实际输出：

```text
Image 1 (kernel):   35707392 B, sha256 5e8fc7f485e3a8ef71f73d0c69d6520bd753377a697f1ec983f8ec620449ceeb
Image 2 (resource): 638976 B,   sha256 492cbec98ecfdd2b2d66f03694127d8744ec89339716b60b9d662456bc6569c6
r1-boot-kernel.img size=35707392 bytes
r1-boot-resource.img size=638976 bytes
```

结论：**已验证**当前 boot FIT 的 fdt、kernel、resource 三项现均有本地主机副本，且 SHA-256 分别精确匹配步骤 92 的 FIT 声明：`abd1c6c3…fe87546`、`5e8fc7f4…49ceeb`、`492cbec9…6569c6`。这排除了载荷提取或 FIT 外置偏移解释错误。下一步可仅在主机用这三项旧载荷构造 FIT 克隆，以验证 `mkimage` 的外置数据布局与哈希生成，不替换 kernel、不写入板子。

下一步：创建仅使用这三项已验证旧载荷的主机侧 FIT 克隆 manifest，使用现有 `0x800` 外置数据起点和 `0x200` 对齐重建，再以 `dumpimage -l` 自检；不写入板子。

### 步骤 102：成功重建并自检旧载荷 FIT 克隆

目的：验证 `mkimage` 可在主机以当前 R1 的外置数据起点和对齐策略，重新封装已提取的旧载荷且保持各 image 哈希不变。

执行端：Arch 主机。学习者使用 `r1-boot-fit-clone.its`、`mkimage -E -p 0x800 -B 0x200` 生成克隆 FIT，并以 `dumpimage -l` 读取；未写入 R1。

实际输出要点：

```text
FIT description: R1 boot FIT clone for host-side validation only
Image 0 (fdt):      sha256 abd1c6c3...fe87546
Image 1 (kernel):   sha256 5e8fc7f4...49ceeb
Image 2 (resource): sha256 492cbec9...6569c6
Default Configuration: 'conf'
Configuration 0 (conf): Kernel=kernel, FDT=fdt
.../r1-boot-fit-clone.img; size=36496384 bytes
179b89f7ce145876219190cd78f06e76fb6ddbfff9caa4dc6f2d48052ca82319  r1-boot-fit-clone.img
```

结论：**已验证**主机 `mkimage` 能将三项已核验旧载荷重建为有效 FIT，`dumpimage` 可读，三项 SHA-256 均与当前 boot FIT 完全一致。克隆总长 36,496,384 B，恰等于步骤 94 现有三项外置数据结束位置 `0x22ce400`，表明 `-E -p 0x800 -B 0x200` 已复现该数据布局。克隆省略了原 FIT 中无 `value` 的 signature 元数据，故它不是字节级克隆或可写入候选；本步骤只证明外置封装、载荷引用和 hash 生成链路成立。

下一步：只读取克隆 FIT 的 image data-position/data-size 与 `conf` 的 `multi` 属性，确认布局与原 FIT 一致；不构建新内核 FIT、不写入板子。

### 步骤 103：核验 FIT 克隆的外置布局与配置引用

目的：确认 host-side 克隆不仅保留 image 哈希，也复现当前 R1 FIT 的外置载荷偏移、大小与 resource 引用。

执行端：Arch 主机。学习者只读取 FIT 克隆控制树的 image position/size 和配置文本属性；未构建新内核 FIT、未写入 R1。

实际输出：

```text
/images/fdt:      position=0x800,     size=0x24172
/images/kernel:   position=0x24a00,   size=0x220da00
/images/resource: position=0x2232400, size=0x9c000
conf: kernel=kernel, fdt=fdt, multi=resource
```

结论：**已验证**克隆的三项外置数据 position/size 与步骤 94 的当前 FIT 完全一致，`conf` 也仍引用 `kernel`、`fdt`、`resource`。至此，主机侧已验证“现有载荷 → 同布局可读 FIT”的完整封装链。下一步可以只在主机以新 `Image`、已构建 R1 DTB、原 resource 创建候选 FIT，再进行完整性/容量静态检查；不得写入 p3。

下一步：创建引用新 Image、新 R1 DTB 和原 resource 的主机侧候选 FIT manifest，按已验证布局封装并静态自检；不写入板子。

### 步骤 104：生成并初检 RKNPU 0.9.8 候选 boot FIT

目的：把已构建的新 Image、R1 DTS port DTB 和保持不变的厂商 resource 封装为主机候选 FIT，并验证 image 完整性和分区容量上界。

执行端：Arch 主机。学习者使用 `r1-boot-fit-npu098.its`、`mkimage -E -p 0x800 -B 0x200` 生成候选，再以 `dumpimage -l`、`stat`、`sha256sum` 检查；未写入 R1。

实际输出要点：

```text
Image 0 (fdt):      233459 B, sha256 a76fbacb8ea37230be1dfb4d08762017b09cfa8a36516b0f668057f55268949d
Image 1 (kernel):   37675520 B, sha256 e39e443ccff8b670ece4caa3149a6c0272b34c723fca7fe5492e8fff918726d3
Image 2 (resource): 638976 B, sha256 492cbec98ecfdd2b2d66f03694127d8744ec89339716b60b9d662456bc6569c6
.../r1-boot-fit-npu098.img; size=38550016 bytes
6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb  r1-boot-fit-npu098.img
remaining=28558848 bytes
```

结论：**已验证**候选 FIT 可被 `dumpimage` 正常解析；新 DTB、内核及原 resource 的哈希都与各自输入完全对应。候选总长 38,550,016 B，小于 p3 67,108,864 B，容量余量为 28,558,848 B，因此容量不是阻塞。它仍缺少板端验收：尚未核对候选的最终 data positions/config 引用，未在 U-Boot RAM 中加载，更未写入 p3。下一步只读取候选控制树的布局、`multi`、rollback index 和 signature 节点存在性；不写入板子。

下一步：只读取候选 FIT 的 image data-position/data-size、conf 的 kernel/fdt/multi/rollback-index 和 signature 节点存在性；不写入板子。

### 步骤 105：完成候选 FIT 的静态布局与配置核验

目的：确认新 DTB 尺寸变化后的外置数据位置仍连续、对齐、落在 p3 容量内，且配置引用保持预期。

执行端：Arch 主机。学习者只读取候选 FIT 控制树属性；未写入 R1。

实际输出：

```text
/images/fdt:      position=0x800,     size=0x38ff3
/images/kernel:   position=0x39800,   size=0x23ee200
/images/resource: position=0x2427a00, size=0x9c000
conf: kernel=kernel, fdt=fdt, multi=resource, rollback-index=0
signature: absent by design
candidate-end=0x24c3a00 (38550016 bytes)
```

结论：**已验证**候选 FIT 的 FDT 结束后按 `0x200` 对齐到 kernel，kernel 结束处正好是 resource 起点；最终外置数据末端为 `0x24c3a00`（38,550,016 B），小于 p3 64 MiB。`conf` 的 image 引用和 rollback index 均与原配置一致。候选没有 signature 节点，这是 manifest 的明确设计；步骤 95/99 尚未发现实际签名 value 或 U-Boot 公钥 required 策略。静态验证到此完成，但仍不构成板端可启动证据。

下一步：重启进入 U-Boot 提示符，仅运行 `help tftpboot`、`help dhcp` 与 `printenv ipaddr serverip bootfile`，确认是否可用网络将候选 FIT 载入 RAM 做非持久测试；不运行下载或启动命令，不写入板子。

### 步骤 106：确认 U-Boot RAM 网络启动命令入口

目的：确定当前 U-Boot 是否有可用于候选 FIT 非持久 RAM 测试的网络命令，并确认其网络环境尚未预设。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者仅读取命令帮助与环境变量；未执行 DHCP、TFTP、启动或写入。

实际输出：

```text
tftpbootm - tftpbootm aosp/uImage/FIT image via network using TFTP protocol
Usage:
tftpbootm [loadAddress] [[hostIPaddr:]bootfilename]

dhcp - boot image via network using DHCP/TFTP protocol
Usage:
dhcp [loadAddress] [[hostIPaddr:]bootfilename]

## Error: "ipaddr" not defined
## Error: "serverip" not defined
## Error: "bootfile" not defined
```

结论：**已验证**当前 U-Boot 提供厂商命令 `tftpbootm`，其语义是通过 TFTP 获取 AOSP/uImage/FIT 并启动；`dhcp` 也属于“DHCP/TFTP boot image”命令，而非只租约查询。两者在将来都属于临时 RAM 启动操作，不会写 eMMC，但此时仍不执行。`ipaddr`、`serverip`、`bootfile` 均未定义，说明不存在待继承的持久网络配置；后续可设置不保存的临时地址，但需先确认主机共享网卡地址、选择无冲突板端地址，并准备只读 TFTP 根目录。Arch 仓库确认 `extra/tftp-hpa` 5.2-11 可用，是否已安装待主机检查。

下一步：在主机只读读取共享网卡 `enp108s0` 的 IPv4 地址，并检查 `tftp-hpa` 是否已安装；不启动服务、不修改 U-Boot 环境、不下载或启动候选 FIT。

### 步骤 107：确认主机 TFTP RAM 测试前置条件

目的：取得 U-Boot 临时网络配置所需的主机 IPv4 地址，并确认本机已有可启动的 TFTP server 程序。

执行端：Arch 主机。学习者只读取接口地址与安装包数据库；未启动服务、未修改 U-Boot 环境、未下载或启动候选 FIT。

实际输出：

```text
enp108s0  UP  10.42.0.1/24
tftp-hpa 5.2-11
```

结论：**已验证**主机直连共享网段地址为 `10.42.0.1/24`，`tftp-hpa` server 已安装。先前 NetworkManager dnsmasq 的 DHCP 池从 `.10` 开始；因此后续可选择 `.2` 作为 U-Boot 仅 RAM 内的临时板端地址，避免与已知 DHCP 池重叠。下一步创建仅含候选 FIT 的临时 TFTP 根目录，并用前台、secure、绑定 `10.42.0.1:69` 的 server 提供下载；服务不会写入板子，停止前先由主机本地 TFTP client 回读并核验哈希。

下一步：创建临时只读 TFTP 根目录并前台启动绑定 `10.42.0.1:69` 的 `in.tftpd`；先不在 U-Boot 下载或启动。

### 步骤 108：识别临时 TFTP server 的端口冲突

目的：在未触及 U-Boot 或 R1 eMMC 的前提下，解释前台 `in.tftpd` 为什么没有保持运行，并区分网络地址问题与 UDP 端口占用。

执行端：Arch 主机。学习者创建了仅含候选 FIT 的 TFTP 根目录，核对文件哈希后尝试以前台方式启动；随后只读检查监听 socket、网卡地址和 systemd 单元。未在 R1 下载、启动或写入。

实际输出：

```text
6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb  .../r1-boot-fit-npu098.img
6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb  .../tftp-root/r1-boot-fit-npu098.img

UNCONN 0 0 0.0.0.0:69 0.0.0.0:* users:(("in.tftpd",pid=914,fd=4))
UNCONN 0 0 [::]:69    [::]:*    users:(("in.tftpd",pid=914,fd=5))
enp108s0 ... inet 10.42.0.1/24 ...
Unit tftp.service could not be found.
Unit tftp.socket could not be found.
```

结论：**已验证**候选 FIT 与准备的 TFTP 根目录文件哈希一致，主机网卡地址仍为 `10.42.0.1/24`。UDP 69 已由 PID 914 的 `in.tftpd` 监听全部 IPv4/IPv6 地址，因此新前台 server 不能绑定该端口并以退出码 71 退出。尚不知道 PID 914 的启动参数、TFTP 根目录和管理者；不能假定它提供候选文件，也不停止或替换它。systemd 中未发现名为 `tftp.service`/`tftp.socket` 的单元。

下一步：只读读取 PID 914 的命令行、工作目录和进程父级，确认它是否正是此前启动的同一 TFTP server，以及能否安全复用；仍不在 U-Boot 下载或启动。

### 步骤 109：释放未使用的旧 TFTP listener

目的：在学习者确认旧服务对应当前不使用的 i.MX6ULL 后，只释放 UDP 69，以便为 R1 建立独立的临时 TFTP server。

执行端：Arch 主机。学习者明确说明该 i.MX6ULL 当前不使用，并执行 `sudo kill -TERM 914`；随后以 `sudo ss -ulpn | rg ':69\\b|tftp' ; or true` 复核。未删除文件、未改变网络配置，未在 U-Boot 下载或启动，也未写入 R1。

实际输出：复核命令无输出。

结论：**已验证**PID 914 不再监听 UDP 69，端口已释放。`TERM` 是可恢复的进程停止操作；先前 TFTP 根目录内容未被删除。下一步启动绑定 `10.42.0.1:69`、仅暴露候选 FIT 的新前台 TFTP server，并先在主机本机回读和核验哈希。

### 步骤 110：验证 R1 候选 FIT 的主机本机 TFTP 回读

目的：在让 U-Boot 使用网络前，验证专用 TFTP server 能从其 secure 根目录读出完整候选 FIT。

执行端：Arch 主机。一个终端以前台方式运行 `in.tftpd --foreground --secure --verbose --address 10.42.0.1:69 .../tftp-root`；另一终端在临时目录执行 `tftp 10.42.0.1 -c get r1-boot-fit-npu098.img` 后计算 SHA-256。未修改 U-Boot 环境，未在 R1 下载、启动或写入。

实际输出：

```text
6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb  r1-boot-fit-npu098.img
```

学习者报告 server 前台无额外日志。

结论：**已验证**主机从 `10.42.0.1:69` 经 TFTP 回读的候选 FIT 与原候选 SHA-256 一致，因此 server 已监听、根目录与读取权限均有效。前台无日志不否定成功，以完整回读哈希为准。下一步进入 U-Boot 仅设置临时 `ipaddr=10.42.0.2`、`serverip=10.42.0.1` 并执行 ping；不保存环境、不下载或启动。

### 步骤 111：验证 U-Boot 到主机的临时网络连通性

目的：在启动候选 FIT 前，只验证 U-Boot 是否能通过 R1 千兆网口解析主机的 ARP 地址。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者临时执行 `setenv ipaddr 10.42.0.2`、`setenv serverip 10.42.0.1` 与 `ping $serverip`；未执行 `saveenv`、TFTP 下载、RAM 启动或 eMMC 写入。

实际输出：

```text
ethernet@fe1c0000 Waiting for PHY auto negotiation to complete. done
Using ethernet@fe1c0000 device

ARP Retry count exceeded; starting again
ping failed; host 10.42.0.1 is not alive
```

结论：**已验证**U-Boot 成功完成 `ethernet@fe1c0000` PHY 自协商，故物理链路和 PHY 基础初始化通过；但它没有获得 `10.42.0.1` 的 ARP 回复，当前不能从 U-Boot 获取 TFTP 文件。仅凭这一次失败不能确定请求是否到达主机、是否被主机忽略，或 U-Boot MAC/地址设置是否异常。下一步在主机只抓取 `enp108s0` 上的 ARP 流量，同时重做一次相同 U-Boot ping；不改变网络策略，不下载或启动。

### 步骤 112：抓包定位 U-Boot TFTP 前的 ARP 超时

目的：确定 U-Boot 的 ARP 请求是否到达主机，以及主机是否已发出回复，以避免把收包问题误判为 TFTP、IP 配置或防火墙问题。

执行端：Arch 主机与 R1 Debug UART。主机以 `sudo tcpdump -ni enp108s0 -e -vvv arp` 被动抓包；学习者在保持相同临时 U-Boot 地址的条件下重复 `ping $serverip`。未改网络策略，未执行 TFTP、RAM 启动或 eMMC 写入。

实际输出（节选，四轮相同）：

```text
1e:a8:e4:78:ee:77 > ff:ff:ff:ff:ff:ff ... Request who-has 10.42.0.1 tell 10.42.0.2
08:bf:b8:c2:8a:1b > 1e:a8:e4:78:ee:77 ... Reply 10.42.0.1 is-at 08:bf:b8:c2:8a:1b
...
8 packets captured
8 packets received by filter
0 packets dropped by kernel
```

结论：**已验证**U-Boot 使用 MAC `1e:a8:e4:78:ee:77` 发出 ARP 广播，主机 `08:bf:b8:c2:8a:1b` 每轮均向该 MAC 单播正确回复；但 U-Boot 仍重复 ARP 并报告超时。故当前阻塞不在 TFTP server、主机 IPv4 地址或主机未回应，而在 U-Boot 侧未成功处理返回的二层帧（具体是 MAC/DMA/驱动/环境何项尚未定位）。为避免在不服务于近期 NPU 目标的 U-Boot GMAC 排障中下钻，改用备选的 eMMC userdata→RAM 临时启动路线：先只读确认 U-Boot 有 `ext4load` 和 `bootm`，再决定是否将候选文件写入 userdata；不改 boot 分区。

### 步骤 113：确认 U-Boot 的 eMMC ext4 读取与 FIT 启动入口

目的：验证备选的 userdata→RAM 临时启动路线所需命令在当前厂商 U-Boot 中存在，并记录其参数语义。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者只读取 `help ext4load` 与 `help bootm`；未读取分区内容、未写入或启动。

实际输出要点：

```text
ext4load - load binary file from a Ext4 filesystem
Usage: ext4load <interface> [<dev[:part]> [addr [filename [bytes [pos]]]]]

bootm - boot application image from memory
... addr#<conf_uname> - configuration specification
```

结论：**已验证**当前 U-Boot 有 `ext4load` 和 FIT configuration 形式的 `bootm addr#<conf>`。这使 `/userdata`（GPT 第 8 分区）上的候选文件可作为非 boot 分区的中转：读取到 RAM 后用 `bootm` 临时启动，断电/重启仍会回到原 p3 boot FIT。下一步先只读列出 `mmc 0:8` 的根目录，确认 U-Boot 实际能读该分区的 ext4；尚不写入 userdata。

### 步骤 114：验证 U-Boot 可读取 userdata 分区

目的：确认 `mmc 0:8` 与运行 Linux 的 `/userdata` 对应，且当前 U-Boot 可实际读取其 ext4 根目录。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者执行只读 `ext4ls mmc 0:8 /`；未写入、加载或启动。

实际输出：

```text
<DIR>       4096 lost+found
<DIR>       4096 recovery
               0 .resized
<DIR>       4096 rkllm-api-demo
```

结论：**已验证**U-Boot 能读取 eMMC 第 8 分区的 ext4 根目录，并看到 Linux `/userdata` 中已有的 `rkllm-api-demo`，第 8 分区与前述 GPT/Linux 映射一致。候选 FIT 可以作为普通文件安全中转到该分区，随后经 `ext4load` 读入 RAM；这仍不等同于候选可启动。下一步正常重启至当前已知 Linux，再用 SSH 将候选复制到独立目录并做板端 SHA-256 回归；不修改 boot 分区。

### 步骤 115：将候选 FIT 作为普通 userdata 文件传入 R1

目的：为不改 boot 分区的 RAM 临时启动准备本地中转文件，并通过端到端哈希排除传输损坏。

执行端：Arch 主机与 R1 Linux root SSH。主机在 `/userdata/r1-ram-boot-test/` 新建独立目录后，以 SCP 写入 `r1-boot-fit-npu098.img`，随后主机/板端分别计算 SHA-256 并列出文件；没有写入 p1、p3、rootfs 或 U-Boot 环境。

实际输出：

```text
r1-boot-fit-npu098.img  100%  37MB  67.1MB/s
6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb  .../r1-boot-fit-npu098.img
6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb  /userdata/r1-ram-boot-test/r1-boot-fit-npu098.img
-rw-r--r-- 1 root root 37M Nov 22 04:58 /userdata/r1-ram-boot-test/r1-boot-fit-npu098.img
```

结论：**已验证**候选 FIT 已完整存入 R1 `/userdata/r1-ram-boot-test/r1-boot-fit-npu098.img`，板端与主机 SHA-256 均为 `6e07fcdc0f7ac19d36957e5aaca1973145d87d1ef9c06e7d09d6da74564fbadb`。影响范围仅为 userdata 新增一个可删除文件。下一步重启进入 U-Boot，以 `ext4load mmc 0:8 0x0a200000 ...` 读到安全的高端 DRAM 地址并 `iminfo` 解析；先不启动。

### 步骤 116：将候选 FIT 读入 U-Boot RAM

目的：验证 U-Boot 能从 userdata 完整读取候选 FIT 到选定的高端 DRAM 地址，并尝试在不启动的情况下识别该映像。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者执行 `ext4load mmc 0:8 0x0a200000 /r1-ram-boot-test/r1-boot-fit-npu098.img` 与 `iminfo 0x0a200000`；未执行 `bootm`、`saveenv` 或任何分区写入。

实际输出：

```text
38550016 bytes read in 225 ms (163.4 MiB/s)
Unknown command 'iminfo' - try 'help'
```

结论：**已验证**U-Boot 从第 8 分区读取了精确 `38,550,016` B，等于候选文件已记录长度；候选当前驻留 `0x0a200000` RAM。当前厂商 U-Boot 未编入标准 `iminfo`，这仅限制该工具层的非启动检查，不表示 FIT 解析失败。下一步用 U-Boot 已确认可用的 `fdt` 命令把该地址设为工作 FDT 并读取 `/configurations`；不启动。

### 步骤 117：解析 RAM 中候选 FIT 的 configuration

目的：在真正启动前，确认 U-Boot 的 libfdt 能解析 RAM 候选的控制树，并确认默认 configuration 指向预期的三项 image。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者执行 `fdt addr 0x0a200000` 与 `fdt print /configurations`；期间两次误输入 `pint`，均只得到 `FDT_ERR_NOTFOUND`，随后以正确路径成功读取。未执行 `bootm`、保存或写入。

实际输出：

```text
configurations {
        default = "conf";
        conf {
                description = "R1 RKNPU 0.9.8 candidate configuration";
                kernel = "kernel";
                fdt = "fdt";
                multi = "resource";
                rollback-index = <0x00000000>;
        };
};
```

结论：**已验证**U-Boot libfdt 成功解析 RAM 中候选 FIT 的控制树，默认 `conf` 和其 kernel/FDT/resource 引用与主机静态核验一致。此时仅剩临时 `bootm 0x0a200000#conf` 才能验证实际 kernel/DTB 兼容性；该命令会离开 U-Boot 并启动 RAM 中的候选，但不会写入任何 eMMC 分区。若失败或无输出，强制重启仍从原 p3 boot FIT 启动。

### 步骤 118：首次非持久 RAM 启动候选内核

目的：验证 RKNPU 0.9.8 候选 FIT 是否能在 R1 实机上越过 U-Boot 并启动 kernel/用户空间，而不改变 eMMC 启动分区。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者执行 `bootm 0x0a200000#conf`。该命令仅使用步骤 116 已载入 RAM 的候选 FIT；未执行分区写入或保存环境。

实际串口尾部：

```text
[  425.384382] Kernel Offset: disabled
[  425.384688] CPU features: 0x002,48000026,6a00aa38
[  425.385105] Memory Limit: none
[  425.387880] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b ]---
```

结论：**已验证**候选 kernel 已实际运行至少约 425 秒并进入用户空间启动阶段；内核 panic 的直接触发条件是 PID 1（init）以 exit code `0x0000000b` 退出。按 Linux 信号编码，`0x0b` 对应 SIGSEGV；这说明当前官方 Rockchip 5.10/R1 DTS 候选与沿用的现有 Ubuntu rootfs 尚未形成可稳定启动的组合。仅有串口尾部，PID 1 崩溃的具体原因、首个错误日志和是否与 rootfs/内核 ABI/设备树有关均**待验证**。由于启动来自 RAM，重启后仍会使用原 p3 FIT。下一步先恢复原系统并检查 pstore 是否保留候选启动的完整崩溃上下文；不重试候选启动。

### 步骤 119：检查候选 panic 的 pstore 留存

目的：在原系统恢复后，尝试从保留 RAM 的 pstore 获取候选内核 panic 前的完整上下文，避免先重试而丢失可能的日志。

执行端：R1 原 Linux 的 Debug UART root Shell。学习者在重启回原 p3 FIT 后执行只读 `ls -lah /sys/fs/pstore 2>&1`；未再次加载候选或写入分区。

实际输出：

```text
total 0
drwxr-x--- 2 root root 0 Nov 22 04:57 .
drwxr-xr-x 8 root root 0 Jan  1  2021 ..
```

结论：**已验证**当前 pstore 目录为空，未保留本次候选 panic 的 console 或 dmesg 记录。因此 PID 1 SIGSEGV 的原因仍未知，不能据此认定为 rootfs、设备树或内核配置问题。下一步先在主机启用串口会话日志，再机械重做已验证的 userdata→RAM 启动，以获取 panic 前完整串口上下文；仍不写 boot 分区。

### 步骤 120：带串口记录的第二次候选 RAM 启动

目的：以主机串口日志记录第二次同载荷的非持久启动，取得 PID 1 panic 前的完整上下文。

执行端：Arch 主机以 picocom `--logfile` 会话记录 Debug UART；R1 U-Boot 再次从 `/userdata/r1-ram-boot-test/r1-boot-fit-npu098.img` 读取候选并执行同一 `bootm 0x0a200000#conf`。未写 boot 分区。

目前取得的串口尾部：

```text
[   70.979429] Kernel Offset: disabled
[   70.979734] CPU features: 0x002,48000026,6a00aa38
[   70.980151] Memory Limit: none
[   70.982886] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b ]---
```

学习者报告该尾部之前主要为全零 IRQ/PMU/thermal 寄存器转储。

保存的完整串口记录为 `build/local/r1-20260816/r1-candidate-boot-attempt2.log`，大小 141,748 B，SHA-256 为 `61d90ad63ede3887a0506e1b9ad78c236eedc2b66411aa637478245e6bf16f85`。

结论：**已验证**同一候选第二次再次以 PID 1 exit code `0x0b` panic，说明问题可复现；本次约 71 秒的时间与第一次约 425 秒不同，不能把具体时间作为稳定特征。寄存器的零值转储是 panic 诊断上下文，不足以定位首因。日志文件已由主机保存，但尚未从中提取 panic 之前的异常行；下一步在原系统恢复、关闭串口会话后，检索保存日志中的 `systemd`、`segfault`、`Oops`、`Unable`、`panic` 等关键字和其上下文。

### 步骤 121：定位候选启动 Oops 的 HDMI 资源不匹配

目的：从保存的串口记录和候选 DTB/内核源码中定位 `Attempted to kill init` 之前的首个故障，区分用户空间崩溃与内核驱动 Oops。

执行端：Arch 主机。读取 `build/local/r1-20260816/r1-candidate-boot-attempt2.log` 的 Oops 上下文，并只读查询候选 DTB HDMI 节点和隔离内核源码；未再次启动或写入 R1。

实际证据：

```text
[   69.217327] dwhdmi-rockchip fde80000.hdmi: registered ddc I2C bus driver
[   69.217569] dwhdmi-rockchip fde80000.hdmi: invalid resource
[   69.217626] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000040
CPU: 4 PID: 1 Comm: swapper/0 Not tainted 5.10.252 #1
pc : __dw_hdmi_probe+0x774/0xad8
Call trace:
 __dw_hdmi_probe
 dw_hdmi_qp_bind
 dw_hdmi_rockchip_bind
 rockchip_drm_bind
 rockchip_drm_platform_probe
 rockchip_drm_init
 kernel_init
```

候选 DTB 的 `/hdmi@fde80000` 为 `status = "okay"`，`reg` 仅为 `0x0 fde80000 0x0 20000`。新内核 `dw-hdmi-qp.c` 在检测到 HDCP14 后调用 `platform_get_resource(pdev, IORESOURCE_MEM, 1)` 并立即传给 `devm_ioremap_resource()`；候选 DTS 未提供这一索引资源。

结论：**已确认**候选启动的首个致命故障是 Rockchip DRM HDMI probe 中对缺失第二个内存资源的空指针访问，发生在内核初始化线程 `swapper/0`，其后的 init panic 是连锁后果。该 R1 vendor DTS 与新 Rockchip HDMI 驱动的资源要求不兼容。为验证近期 headless NPU 目标，下一步仅在隔离候选 DTS 关闭 HDMI、HDMI PHY、HDMI 音频和 HDMI VOP 路由，生成另一个明确命名的 headless 候选；不改 NPU/eMMC/GMAC，不写 R1。

### 步骤 122：构建并静态验证 headless RKNPU 候选 FIT

目的：只绕过已确认的 HDMI Oops，保留 RKNPU 0.9.8 内核、R1 eMMC/GMAC/NPU 与原 resource image，生成可单独验证的 headless 候选。

执行端：Arch 主机。仅修改隔离工作树顶层 `rk3588s-yyt.dts`，追加 `status = "disabled"` 覆盖 `hdmi0`、`hdptxphy_hdmi0`、`hdmi0_in_vp1` 与 `hdmi0_sound`；使用既有 Kbuild 输出目录重建 DTB。原显示候选 DTB 先复制为 `build/local/r1-20260816/rk3588s-yyt-display-candidate.dtb` 并保持原 SHA-256。再以新 DTB、既有 RKNPU 0.9.8 Image 与原 resource 构建 headless FIT。未访问 R1。

实际输出要点：

```text
preserved display DTB SHA-256:
a76fbacb8ea37230be1dfb4d08762017b09cfa8a36516b0f668057f55268949d

headless DTB: 233475 B
cc3b50ecce8a9e27bda78f820fd89d2803e75174763c5142d85df9aa24609b1a

headless FIT: 38550528 B
1369e8b9f8ac7bfe966fe0c70b907e851d5b89764d505d8fcc0a1aef9bd790f3
```

DTB 静态状态为：`/hdmi@fde80000`、`/hdmiphy@fed60000`、`/hdmi0-sound` 均 `disabled`；反编译结果中 `route_hdmi0` 与 `hdmi0_in_vp1` 也为 `disabled`。`/npu@fdab0000`、`/ethernet@fe1c0000`、`/mmc@fe2e0000` 均保持 `okay`。FIT 中 FDT、kernel、resource 哈希分别为 `cc3b50ec…24609b1a`、`e39e443c…918726d3`、`492cbec9…6569c6`；外置数据终点为 `0x24c3c00`（38,550,528 B），仍小于 p3 容量并余 28,558,336 B。

结论：**已验证**headless 候选仅改变 HDMI 相关 DT 状态，保持 NPU、网口、eMMC 开启，且 FIT 可由 `dumpimage` 解析。它尚未传入或启动到 R1；下一步以 SSH 写入 userdata 的新文件名、核验板端哈希后进行相同的非持久 RAM 启动。

### 步骤 123：传输并核验 headless 候选 FIT

目的：将仅用于临时 RAM 启动的 headless FIT 作为普通文件放入 userdata，并以端到端 SHA-256 排除传输损坏；不改变实际启动分区。

执行端：Arch 主机与 R1 Linux root SSH。学习者以 SCP 写入 `/userdata/r1-ram-boot-test/r1-boot-fit-npu098-headless.img`，随后在主机和 R1 分别计算 SHA-256 并列出板端文件。未写入 p1、p3、rootfs 或 U-Boot 环境。

实际输出：

```text
r1-boot-fit-npu098-headless.img  100%  37MB  56.3MB/s
1369e8b9f8ac7bfe966fe0c70b907e851d5b89764d505d8fcc0a1aef9bd790f3  /home/loser/Study/rk3588/build/local/r1-20260816/r1-boot-fit-npu098-headless.img
1369e8b9f8ac7bfe966fe0c70b907e851d5b89764d505d8fcc0a1aef9bd790f3  /userdata/r1-ram-boot-test/r1-boot-fit-npu098-headless.img
-rw-r--r-- 1 root root 37M Nov 22 04:57 /userdata/r1-ram-boot-test/r1-boot-fit-npu098-headless.img
```

结论：**已验证**headless FIT 已完整传入 R1 userdata，板端 SHA-256 与主机构建产物一致。影响范围仅为 userdata 的一个新增文件；R1 原 p3 启动 FIT 未被修改。下一步在串口中重启、进入 U-Boot，以 `ext4load` 将这个新文件读入 RAM，并在 `bootm` 前用 `fdt print /configurations` 核验其控制树。

### 步骤 124：加载并解析 headless FIT 的 configuration

目的：在 RAM 启动前，确认 U-Boot 能完整读取 headless FIT，且默认配置实际指向预期的 kernel、FDT 与 resource。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者执行以下只读加载和控制树读取命令；未执行 `bootm`、`saveenv` 或任何分区写入。

```text
ext4load mmc 0:8 0x0a200000 /r1-ram-boot-test/r1-boot-fit-npu098-headless.img
fdt addr 0x0a200000
fdt print /configurations
```

实际输出：

```text
38550528 bytes read in 223 ms (164.9 MiB/s)
configurations {
	default = "conf";
	conf {
		description = "R1 RKNPU 0.9.8 headless candidate configuration";
		kernel = "kernel";
		fdt = "fdt";
		multi = "resource";
		rollback-index = <0x00000000>;
	};
};
```

结论：**已验证**U-Boot 读取的字节数精确等于 headless FIT 的主机文件长度，且其 libfdt 成功解析出 headless `conf`，三项 image 引用与预期相符。候选已驻留在 RAM；下一步可执行一次 `bootm 0x0a200000#conf`，这会启动 RAM 中候选但不写 eMMC。若候选失败，重启仍回到原 p3 FIT。

### 步骤 125：首次启动 headless 候选仍触发 init panic

目的：验证禁用 HDMI 是否足以让 RKNPU 0.9.8 候选越过此前的启动故障。

执行端：R1 Debug UART。学习者在步骤 124 已载入的 RAM FIT 上执行 `bootm 0x0a200000#conf`；未执行任何 eMMC 分区写入或 U-Boot 环境保存。

实际串口尾部：

```text
[  174.978981] 00000080: 00000000 00000000
[  174.979319] Kernel Offset: disabled
[  174.979625] CPU features: 0x002,48000026,6a00aa38
[  174.980042] Memory Limit: none
[  174.982791] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b ]---
```

结论：**已验证**headless 候选仍以 `Attempted to kill init! exitcode=0x0000000b` 结束，且本次约 175 秒才到达该末端症状。仅凭 panic 尾部无法判断它是否仍为 HDMI Oops，或是 HDMI 绕过后暴露了新的首个 Oops；不能将此结果视为 H3 已被否定。下一步从本次完整串口日志中定位 panic 前第一条 `Unable to handle`、`Oops` 或 Call trace，再决定下一个最小改动。

### 步骤 126：确认 headless FIT 已加载，但 HDMI 仍进入 DRM component bind

目的：从本次完整串口日志区分“加载错了 FIT/DTB”与“正确 headless DTB 仍触发 HDMI 绑定”两类假设。

执行端：Arch 主机。读取 `build/local/r1-20260816/r1-headless-boot-attempt1.log`；文件为 142,736 B，SHA-256 `cbb6661b258fd6c33f52d128605d06aac39c2d957047d77e773c773e9de4fb1a`。未修改 R1 或重建候选。

实际证据：

```text
Description:  R1 DTS port with HDMI disabled for headless NPU test
Data Size:    233475 Bytes = 228 KiB
Hash value:   cc3b50ecce8a9e27bda78f820fd89d2803e75174763c5142d85df9aa24609b1a
Verifying Hash Integrity ... sha256+ OK
Loading fdt from 0x08300000 to 0x08300000
Booting using the fdt blob at 0x08300000

dwhdmi-rockchip fde80000.hdmi: registered ddc I2C bus driver
dwhdmi-rockchip fde80000.hdmi: invalid resource
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000040
pc : __dw_hdmi_probe+0x774/0xad8
```

主机再对所加载 DTB 只读查询，`/hdmi@fde80000` 的 `status` 为 `disabled`。而新内核 `rockchip_drm_match_add()` 遍历每个已注册的 Rockchip DRM 子驱动的全部 platform device，并无针对该节点 `status` 的过滤。

结论：**已验证**U-Boot 实际加载并校验了预期的 headless FDT，故“误启动旧 FIT/DTB”被排除；但 HDMI 仍作为 platform component 进入 Rockchip DRM master bind，导致相同 Oops。**推测**：需在 U-Boot 对 FDT 的启动期 fixup 之后再读状态，或绕过整个 display-subsystem DRM master，才能确定最小可靠的 headless 绕过；当前不能仅从静态 DTB 的 HDMI `disabled` 推导运行期不会 bind。下一步在不执行 `go` 的 U-Boot `bootm` 分阶段流程中读取 `0x08300000` 的 FDT，直接观察启动期 FDT 是否被 U-Boot 改写。

### 步骤 127：厂商 U-Boot 不支持该 FIT 的分阶段 bootm 检查

目的：在不跳转内核的前提下，尝试让 U-Boot 完成 FIT 的 FDT 装载阶段，以读取启动期 FDT。

执行端：R1 Debug UART 的 U-Boot `=>` 提示符。学习者在 headless FIT 已位于 `0x0a200000` 时执行 `bootm start 0x0a200000#conf`；未执行 `bootm go`、`saveenv` 或分区写入。

实际结果：U-Boot 正确列出并 SHA-256 校验 kernel 与 headless FDT，随后显示：

```text
Loading fdt from 0x08300000 to 0xffffff00
"Synchronous Abort" handler, esr 0x96000045
...
Resetting CPU ...
```

结论：**已验证**该厂商 U-Boot 的分阶段 `bootm start` 会把 FIT FDT 的 `load = 0xffffff00` 作为实际目标地址，触发 U-Boot 自身同步异常并复位；该子命令路径不能用于检查启动期 FDT。此前直接 `bootm 0x0a200000#conf` 的整条路径不会表现为这一错误，因其日志显示 FDT 载入 `0x08300000`，两种调用方式不可混用。本次仅复位 CPU，未跳转候选内核、未写 eMMC；原 p3 将在复位后照常启动。下一步不再使用任何分阶段 `bootm` 子命令，改为在主机隔离 DTS 中移除 display-subsystem 的 DRM compatible，使 Rockchip display DRM master 根本不匹配；再静态验证后另建候选。

### 步骤 128：排除 display-less 设备树绕过

目的：验证移除 `display-subsystem` 的 `compatible` 是否能阻止 Rockchip display DRM master 建立，从而避免 HDMI component bind。

执行端：R1 Debug UART 与 Arch 主机保存的串口日志。隔离 DTS 已删除该节点的 `compatible` 并设为 `disabled`；学习者以普通 userdata 文件经 U-Boot 直接 `bootm 0x0a200000#conf` 启动。未写入 p1、p3 或保存 U-Boot 环境。

实际证据（串口日志节选）：

```text
Description:  R1 DTS port without Rockchip display DRM master
Data Size:    233459 Bytes = 228 KiB
...
Description:  R1 DTS port without Rockchip display DRM master
...
Internal error: Oops: 96000005 [#1] SMP
pc : __dw_hdmi_probe+0x774/0xad8
```

结论：**已验证**U-Boot 实际加载了 display-less FDT，但仍发生相同 HDMI Oops。因此“只从设备树移除 display-subsystem compatible 即可阻止 HDMI bind”的假设被排除；不再继续在该 DTS 层添加绕过。下一步改为仅在新的输出目录关闭 `CONFIG_DRM_ROCKCHIP`，保留 RKNPU 所需通用 DRM/GEM，再构建检查。

### 步骤 129：构建无 Rockchip display DRM 的 RKNPU 0.9.8 候选

目的：排除与 R1 DTS 不兼容的 Rockchip 显示/HDMI 驱动，同时保留 RKNPU 0.9.8 的 DRM GEM 内存接口，生成下一次非持久 RAM 启动候选。

执行端：Arch 主机。由既有 `build/kernel-r1-dts-port/.config` 复制出独立 `build/kernel-r1-nodisplay/.config`，仅通过 `scripts/config --disable DRM_ROCKCHIP` 修改，然后以 `ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-` 构建 `Image` 与 `rockchip/rk3588s-yyt.dtb`。没有访问 R1。

实际输出要点：

```text
CONFIG_DRM=y
# CONFIG_DRM_ROCKCHIP is not set
CONFIG_ROCKCHIP_RKNPU=y
CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y

Image SHA-256:
533b0b077b17474af778575f8c96cd231f7e25e53abe39ae7acc6ca4f229e32c
DTB SHA-256:
4d23f3a15f31b9444c8176acd73b73d6768445abe4144de40c13d29747a7a1c2
```

该 `Image` 长 36,788,736 B，内含 `RKNPU driver`、`0.9.8`、`20240828` 字符串；`aarch64-linux-gnu-nm` 未找到 `__dw_hdmi_probe`。DTB 中 `/hdmi@fde80000` 为 `disabled`，而 `/npu@fdab0000` 与 `/mmc@fe2e0000` 均为 `okay`。

使用上述 Image/DTB、未改变的 resource image 封装得到 `build/local/r1-20260816/r1-boot-fit-npu098-nodisplay.img`：长 37,663,232 B，SHA-256 为 `083e804f3825f1a75b5ec68a328bb8e44ba3c64ed361a52e8d5be430cc6665dc`，小于 p3 的 67,108,864 B，余 29,445,632 B。`dumpimage` 可解析 FDT/kernel/resource，`conf` 引用三者，且无 signature 节点。

结论：**已验证**该候选在主机侧完成配置、编译与 FIT 结构检查；它尚未传到或启动于 R1，不能据此声明显示 Oops 已解决。下一步仅把此文件传入 `/userdata/r1-ram-boot-test/`，核验板端 SHA-256 后执行已验证的直接 RAM 启动路径；不得使用 `bootm start` 等分阶段子命令。

### 步骤 130：定位无显示候选的 Mali GPU probe RCU stall

目的：在 HDMI Oops 不再出现后，定位候选无法继续启动的下一条首要故障，避免将末端卡顿误归因于 NPU。

执行端：R1 Debug UART。无 `CONFIG_DRM_ROCKCHIP` 的候选通过 RAM 直接启动后，不再输出 HDMI Oops，但约 112 秒后 RCU 输出卡顿任务调用栈：

```text
kbase_hwaccess_pm_powerup
kbase_backend_late_init
kbase_device_init
kbase_platform_device_probe
platform_drv_probe
```

结论：**已验证**此时阻塞在 Mali Bifrost GPU probe，而不是 RKNPU。精确的 Mali/DTS/电源兼容根因尚未调查；为保持本轮目标限定为“验证 NPU LLM”，下一候选在独立配置中关闭 `CONFIG_MALI_BIFROST`、`CONFIG_MALI_MIDGARD`、`CONFIG_MALI400`，不改变 DTS、RKNPU 或通用 DRM。

### 步骤 131：无显示、无 Mali 的 RKNPU 0.9.8 候选完成 NPU LLM 回归

目的：在不写 eMMC 的条件下，验证完整候选能启动现有 Ubuntu rootfs，并让相同 RKLLM 模型实际生成文本。

执行端：Arch 主机、R1 Debug UART 和 R1 Linux root Shell。主机构建独立输出 `build/kernel-r1-nodisplay-nogpu`，保留：

```text
CONFIG_DRM=y
CONFIG_ROCKCHIP_RKNPU=y
CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y
```

并关闭 Rockchip display DRM 与三个 Mali 配置。候选 FIT `build/local/r1-20260816/r1-boot-fit-npu098-nodisplay-nogpu.img` 的 SHA-256 为：

```text
05c2b6f67e648a6ed16ddb9d0a0475d9539910675e75626c9cd5170d0e44ac9c
```

学习者将它作为普通文件传至 `/userdata/r1-ram-boot-test/`，核验主机/板端 SHA-256 一致，再在 U-Boot 直接执行：

```text
ext4load mmc 0:8 0x0a200000 /r1-ram-boot-test/r1-boot-fit-npu098-nodisplay-nogpu.img
bootm 0x0a200000#conf
```

候选进入用户空间后，实际证据为：

```text
root@R1:~# uname -r
5.10.252

root@R1:~# readlink -f /sys/class/drm/renderD128/device/driver
/sys/bus/platform/drivers/RKNPU

root@R1:~# cat /sys/class/drm/renderD128/device/uevent
DRIVER=RKNPU
OF_FULLNAME=/npu@fdab0000
OF_COMPATIBLE_0=rockchip,rk3588-rknpu
```

随后使用原有 AArch64 `llm_demo-r1`、原有 `DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm`，并输入 `ok`：

```text
user: ok
robot: Alright,
I rkllm:  Prefill       196.36           4         49.09                    20.37
I rkllm:  Generate      125.08           1         125.08                   7.99
I rkllm:  Peak Memory Usage (MB)
I rkllm:  1673.56
```

结论：**已验证**RKNPU 0.9.8 候选从 RAM 启动后可执行该 W8A8 RKLLM 模型的真实生成路径；旧系统的 `matmul(w8a8) run failed` 未复现。候选没有写入 p1/p3、rootfs 或 U-Boot 环境。由于显示 DRM 未编入，RKNPU 成为唯一 render 节点 `renderD128`；节点号变化不是 CPU 回退证据，sysfs 的 RKNPU driver 绑定才是判据。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 原厂 NPU 内核驱动 | RKNPU 初始化日志 | `rknpu 0.8.2` 已初始化 | 通过，但不足以运行当前 RKLLM Runtime |
| 候选 NPU 内核驱动 | RKNPU 0.9.8 与 DRM 设备绑定 | `renderD128` 绑定 `RKNPU`，同模型实际生成 | 通过 |
| NPU 用户态设备接口 | `/dev/rknpu*` 或 DRM 接口 | 原厂为 `/dev/dri/renderD129`；候选为唯一的 `renderD128` | 通过 |
| RKNN 用户态运行时 | 运行库或服务 | `librknnrt.so`、活动的 `rknn_server.service` | 通过，未推理 |
| 本机 RKNN 模型/通用示例 | `.rknn` 与可执行客户端 | 未找到模型；仅有服务、脚本和相机程序 | 缺失 |
| RKLLM 用户态运行时 | 库、工具或模型 | Runtime 1.3.0 在 RKNPU 0.9.8 候选上生成 `Alright,`；生成约 7.99 tokens/s，峰值 1673.56 MB | 通过 |

## 结论

**已验证**：R1 可通过非持久 RAM 启动 RKNPU 0.9.8 候选内核，并在该候选上用同一 W8A8 RKLLM 模型完成实际文本生成；无需先修复 `rockchip.service` 才能执行 NPU LLM。

**边界**：该候选为当前 NPU 项目验证而关闭 Rockchip display DRM 与 Mali GPU，尚不是可烧录的完整板级镜像。启动期 PCIe link failure 与 PL330 `Bad Desc` 日志有观察到但尚未分析；它们没有阻止本次登录和推理。

## 关联知识与问题

- 支持或修正的知识点：设备文件的名称由驱动 ABI 决定；同一硬件可经专用字符设备或 DRM 节点向用户态暴露。
- 关联问题：[ISSUE-20260810-001](../issue/issue-20260810-001-systemd-degraded-failed-units.md)。

## 后续行动

- [x] 将官方 `rknn-llm` 获取到主机 `src/` 并固定 v1.3.0 提交。
- [ ] 在当前候选系统上执行一条多 token 的项目相关 prompt，记录完整生成文本与性能，作为 Linux→Zephyr 受限命令接口的 LLM 基线。
