---
title: "当前学习状态"
type: status
status: active
created: 2026-08-05
updated: 2026-08-19
tags: [rk3588, progress]
related:
  - "[[status/history]]"
  - "[[roadmap/learning-roadmap]]"
  - "[[environment/environment-index]]"
  - "[[issue/issue-20260805-001-empty-loader]]"
  - "[[issue/issue-20260809-002-read-only-git-mount]]"
  - "[[tool/dtc]]"
  - "[[tool/aarch64-linux-gnu-gcc]]"
  - "[[experiment/exp-20260809-003-check-uboot-host-build-prerequisites]]"
  - "[[experiment/exp-20260809-004-acquire-upstream-uboot-source]]"
  - "[[experiment/exp-20260812-002-select-first-simple-device]]"
  - "[[experiment/exp-20260812-003-check-running-kernel-source-availability]]"
  - "[[experiment/exp-20260812-004-locate-upstream-gpio-source-ref]]"
  - "[[experiment/exp-20260814-001-inspect-r1-npu-first-boot-script]]"
  - "[[experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image]]"
  - "[[experiment/exp-20260815-002-probe-r1-npu-runtime-chain]]"
  - "[[experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites]]"
  - "[[experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot]]"
  - "[[note/device-tree-platform-driver-binding]]"
  - "[[resource/youyeetoo-r1-documentation-repository]]"
  - "[[resource/r1-ubuntu-camera-image-v2-v3]]"
  - "[[resource/airockchip-rknn-llm]]"
  - "[[resource/deepseek-r1-distill-qwen-1-5b-w8a8-rk3588]]"
  - "[[resource/youyeetoo-r1-linux-kernel-5-10]]"
  - "[[resource/u-boot-v2026-07-upstream-source]]"
  - "[[decision/dec-20260810-002-linux-zephyr-amp-long-term-direction]]"
  - "[[decision/dec-20260813-003-npu-llm-required-project-core]]"
  - "[[note/r1-emmc-partition-layout]]"
  - "[[note/uboot-fit-image]]"
  - "[[note/uboot-spl-boot-order]]"
  - "[[note/rockchip-external-boot-blobs]]"
  - "[[issue/issue-20260809-003-r1-dhcp-lease-missing]]"
  - "[[issue/issue-20260809-004-ufw-blocks-shared-nat-forward]]"
  - "[[issue/issue-20260809-005-r1-ssh-public-key-rejected]]"
  - "[[issue/issue-20260810-001-systemd-degraded-failed-units]]"
  - "[[issue/issue-20260811-001-uboot-build-missing-swig]]"
  - "[[issue/issue-20260811-002-uboot-missing-external-boot-blobs]]"
  - "[[issue/issue-20260815-001-rkllm-demo-target-abi-mismatch]]"
  - "[[issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed]]"
  - "[[issue/issue-20260816-001-candidate-hdmi-null-dereference]]"
---

# 当前学习状态

## 当前阶段

**主阶段仍为阶段 0：设备识别、环境建档与调试条件准备；阶段 1 启动链路和阶段 2 Linux 系统组成正在只读交叉进行。** 已建立 Debug UART 与 Rockchip USB 观察通道，完成更新前 eMMC 的可重复校验在线备份。学习者报告已将 Ubuntu camera 候选镜像写入 eMMC，后续只读验证确认 R1 能从 `mmcblk0p6` 启动 Ubuntu 22.04 / Linux 5.10.110；新旧版本串相同，候选文件与当前 eMMC 的精确内容对应关系仍未知。阶段 0 仍未达到完成标准。

## 已知事实

- **用户提供**：开发板使用 RK3588，配置为 4 GB RAM、32 GB 存储；当前没有存储卡。现有电源、Type-C 数据线、USB 转串口模块、网线、杜邦线、裸 LED、光敏传感器、温湿度传感器和 4 线 RGB+V 彩灯模块；传感器/彩灯具体型号尚未清点。
- **用户提供**：RGB 彩灯曾在单片机环境以 3.3 V 和 5 V 尝试使用且未报告异常；其共阳/共阴、具体接线、额定供电、各通道限流和引脚电流仍未知。因此该经历不能证明 R1 GPIO 可安全直连，也不能推出“只要不超过 5 V 就安全”。
- **用户提供**：学习者记得 RGB 模块的 V 接公共端，RGB 三线分别经标记疑为 `681`/`687` 的元件后连接 RGB LED。**推测**：若标记为 `681`，它是 680 Ω 三位电阻码而非电容，且该结构是共阳 RGB；须用实物照片或万用表验证，不能据此直接接 R1。
- **用户提供**：主机运行 Arch Linux；有桌面 Linux 经验，刚接触嵌入式 Linux。
- **用户提供**：长期方向为边缘 AI，同时希望深入学习启动流程、内核和底层驱动。
- **用户决定**：近期项目的不可删核心是“R1 的 NPU 实际运行 LLM”；主机/MCP/CPU 代跑模型均不算完成。采用项目牵引、按需下钻的学习方式：先打通最小链路，已有驱动/运行时直接用，仅在具体阻断点补学或实现。近期范围、验收证据与 Zephyr AMP 的边界见[DEC-20260813-003](../decision/dec-20260813-003-npu-llm-required-project-core.md)。该核心已在临时 RAM 候选上完成首次生成验证，持久化板级镜像仍未完成。
- **已验证（原厂镜像阶段）**：更新后的系统启动日志显示 RKNPU 已为 `fdab0000.npu` 初始化 `rknpu 0.8.2`、启用 IOMMU 并注册 DRM minor 1；用户态存在 `/usr/lib/librknnrt.so`、活动的 `rknn_server.service`（版本 1.3.0，2022-04-29）及 NPU 入口 `/dev/dri/renderD129`。`renderD128` 属于显示 DRM。空闲 `rknn_server` 未打开 DRM 节点，这与等待请求相容。本机指定目录中未找到 `.rknn` 模型、RKLLM 可执行文件或通用示例；这是原厂 0.8.2 驱动阶段的观察，不是当前候选的可用性结论。见[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- **已验证 + 资料记载**：airockchip `rknn-llm` 上游说明 RK3588 可使用主机 Toolkit 转换 `.rkllm` 模型，再在板端以 RKLLM Runtime C/C++ API 推理。主机已固定该 SDK 的浅克隆提交 `878f9361fd3afa7e167b7079918918f78d2c1c2a`（`release v1.3.0`）；其 AArch64 runtime 为 `librkllmrt.so` 并提供 `rkllm.h`，纯文本 C++ demo 以 `.rkllm`、`rkllm_init()`、`rkllm_run()` 和 callback 完成单轮生成，运行时需 Runtime >=1.3.0。仓库含 `rknpu_driver_0.9.8_20241009.tar.bz2`；本板为 0.8.2，资料未给出二者的最低兼容关系，不能直接部署最新 runtime/模型/驱动。见[SDK 资料档案](../resource/airockchip-rknn-llm.md)。
- **已解决**：主机 GCC 16 交叉编译的 `llm_demo` 曾要求 R1 不具备的 `GLIBC_2.38`、`GLIBCXX_3.4.32`，在 `main()` 前失败。R1 原生 `g++` 重建的 `llm_demo-r1` 已在 `LD_LIBRARY_PATH=./lib` 条件下运行，且后续在 RKNPU 0.9.8 候选上完成 `rkllm_init()` 与实际文本生成。详见[ISSUE-20260815-001](../issue/issue-20260815-001-rkllm-demo-target-abi-mismatch.md)。
- **已验证**：主机已有 `DeepSeek-R1-Distill-Qwen-1.5B_W8A8_RK3588.rkllm` 候选文件，大小为 2,040,247,614 字节、SHA-256 为 `85123bc6796760c9e670d6676a7d3e9527d1847406807441976fe1206b04115b`；该文件已传至 R1 `/userdata/rkllm-api-demo/models/`，板端 SHA-256 一致。传输前 R1 `/userdata` 尚余约 14 GiB；总 RAM 3.8 GiB、无 swap、当时 available 为 3.2 GiB。精确下载来源仍待确认；原厂 RKNPU 0.8.2 不兼容，但该模型已在 RKNPU 0.9.8 RAM 候选上完成生成。见[模型资源档案](../resource/deepseek-r1-distill-qwen-1-5b-w8a8-rk3588.md)与[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。
- **已验证**：以保守参数启动的 `llm_demo-r1` 已到达 `user:`，但输入短 prompt `ok` 后重复输出 `E rkllm: matmul(w8a8) run failed`，没有生成文本。Runtime 初始化头明确警告本板 `rknpu driver version: 0.8.2` 过低，要求升级至 `0.9.7`；Runtime 为 1.3.0，模型 Toolkit 为 1.2.1b1、目标 RK3588、W8A8。失败后内存为 total 3.8 GiB、available 3.2 GiB、无 swap；已采集过滤后的内核日志，未见 OOM killer、IOMMU fault 或本次执行后的新 RKNPU 报错。SDK 的 0.9.8 驱动包已确认只是 `drivers/rknpu/` 内核源码子树；运行内核 `CONFIG_ROCKCHIP_RKNPU=y`，且匹配 modules/build 树缺失，故不能直接编译/加载 `.ko`。公开厂商 Linux 5.10 候选已浅克隆并固定 commit；其 RKNPU 配置/DRM GEM 结构与运行系统相符，但版本宏已确认也是 `0.8.2`，不能解决 Runtime 的最低版本要求；它与当前 Ubuntu 5.10.110 的精确匹配仍待确认。见[ISSUE-20260815-002](../issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed.md)与[内核源码档案](../resource/youyeetoo-r1-linux-kernel-5-10.md)。
- **用户提供**：最终拟完成 Linux+Zephyr AMP 项目：Linux 侧运行 LLM，Zephyr 侧计划使用 1–2 个 CPU 核执行实时任务；除实时性外，要求尽可能接近 MPU+MCU 的隔离性，并支持低延迟消息同步与高频、大量数据交换。该方向已作为长期技术决策记录；Zephyr 的具体任务、所需外设、数据方向和实时指标尚未确定，详见[DEC-20260810-002](../decision/dec-20260810-002-linux-zephyr-amp-long-term-direction.md)。
- **已验证**：当前 R1 Linux 的 CPU `possible`、`present`、`online` 都为 `0-7`，即全部 8 个 CPU 由 Linux 在线管理；8 个 CPU DTS 节点的 `enable-method` 都为 `psci`。未发现 `/sys/class/remoteproc`、`/sys/bus/rpmsg`，也未在运行时 DTS 名称中发现 mailbox/remoteproc/RPMsg/shared-memory 节点；`/reserved-memory` 仅列出 CMA、显示资源与 ramoops，未发现可明确归属 Zephyr 的 carveout。本次 Kconfig 输出未显示 `CONFIG_REMOTEPROC` / `CONFIG_RPMSG` 为 `y`；通用 mailbox、CPU hotplug、IOMMU 与 Rockchip IOMMU 均为 `y`，且 `CONFIG_ROCKCHIP_AMP` 明确未启用。这排除了“直接复用已暴露 remoteproc/RPMsg/AMP 路径”的假设。原厂运行时 `/proc/iomem` 还证明，EVB AMP 示例的 24 MiB `0x00800000`–`0x02000000` 区会覆盖当前 Kernel code、reserved 与 Kernel data，不能直接复制到 R1 DTS。另一方面，已完成 NPU RAM 验证的 Rockchip 5.10.252 候选配置启用 `CONFIG_ROCKCHIP_AMP=y`、mailbox、Rockchip mailbox RPMsg 和 VirtIO RPMsg，可作为 AMP 原型的内核配置基线；其 RK3588 AMP DTS/RPMsg/CPU 控制参考仍需针对 R1 审计。通用 `rockchip_amp` 驱动具备读取 `amp-cpus`、通过 SiP SMC 启动 CPU、提供 `/sys/rk_amp/boot_cpu` 的能力，但当前 RK3588 DTS 文件集合没有 `amp-cpus`，尚未证明其使用该能力。未离线 CPU、未启动 Zephyr、未写 eMMC。见[EXP-20260817-001](../experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites.md)。
- **已验证**：Zephyr v4.4.0（commit `684c9e8f32e4373a21098559f748f06915f950c9`）已为 `roc_rk3588_pc/rk3588` 构建，并在启用 `CONFIG_ARM64_DCACHE_ALL_OPS=y`、`CONFIG_ARM64_BOOT_DISABLE_DCACHE=y` 后生成 36,960 B 固件；主机与板端 SHA-256 均为 `782af16b0c0c7e6a702518d787d0abe29ae9157694136022a807e3e93acd4ad5`。R1 U-Boot 从 `mmc 0:8` 加载该固件到 `0x50000000`，`go 0x50000000` 成功显示 Zephyr v4.4.0 和 `Hello World! roc_rk3588_pc/rk3588`。`booti` 与 `bootm` 尝试均在厂商 U-Boot 内同步异常，未进入 Zephyr。本结果只验证单独启动，不是 Linux+Zephyr 并行运行、CPU/内存隔离或 IPC 证据。见[EXP-20260819-002](../experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot.md)。
- **用户提供**：开发板准确型号为风火轮（youyeetoo）R1。
- **用户提供**：开发板为 R1 V2；PCB 丝印照片或文字尚未保存。
- **用户提供**：板上有两颗物理 LED；其颜色、功能、供电/控制路径与 Linux 对应关系未确认。
- **用户提供**：R1 正反面带接口标注图已归档；图确认正面右侧有 30PIN 扩展口、背面有 Debug UART 位置标注，但未显示 30PIN 的逐针编号或 1 脚方向，不能作为 GPIO 接线依据。图像来源、文件元数据和 SHA-256 见[硬件环境基线](../environment/hardware.md)。
- **资料记载**：youyeetoo 官方 README 的 30PIN 逐针表列出 pin 9 `GPIO1_A6`、11 `GPIO1_A4`、13 `GPIO1_A7`、15 `GPIO1_B1`（均 3.3 V），以及 pin 17 `GPIO1_D5`、25 `GPIO0_A0`（均 1.8 V）；该 README 的“7 路 GPIO”摘要又列出不同组合，资料内部不完全一致。逐针孔位已获得，但 R1 V2 实物 1 脚方向和本机默认复用仍待核对，不能直接接线。见[硬件环境基线](../environment/hardware.md)与[资料档案](../resource/youyeetoo-r1-documentation-repository.md)。
- **已验证**：R1 当前运行的 Linux 中，PID 1 的进程名为 `systemd`，其显示的启动参数为 `/sbin/init`，`/proc/1/exe` 最终解析为 `/usr/lib/systemd/systemd`；`/sbin/init` 指向 `/lib/systemd/systemd`，`/lib` 又指向 `/usr/lib`。这条路径链已闭合；systemd 所管理服务和系统健康状态待继续读取。见[实验 EXP-20260810-001](../experiment/exp-20260810-001-identify-linux-pid-1.md)。
- **已验证**：更新前 systemd 为 `degraded`，失败单元为 `apport-autoreport.service` 与 `rockchip.service`；更新后只列出 `rockchip.service`。旧系统中该服务的直接失败点是 `/etc/init.d/rockchip.sh` 调用 `tar` 无法打开预期的 `/rknpu2-rk3588-*.tar`，脚本以状态 2 退出；更新后是否仍是同一命令和退出码尚未读取。2026-08-14 已确认脚本的 RK3588/RK3588S 分支含该 tar 解包命令、根目录 tar 清理和 `first_boot_flag` 创建，但其外层生命周期条件尚未读取。见[ISSUE-20260810-001](../issue/issue-20260810-001-systemd-degraded-failed-units.md)与[EXP-20260814-001](../experiment/exp-20260814-001-inspect-r1-npu-first-boot-script.md)。
- **已验证**：R1 Debug UART 的 USB 转串口模块为 `/dev/ttyUSB0`；其所属组为 `uucp`，当前用户 `loser` 属于该组，可使用普通用户权限访问。见[实验 EXP-20260807-001](../experiment/exp-20260807-001-connect-debug-uart.md)。
- **已验证**：R1 重新上电后通过 Debug UART 启动 Linux，并进入可交互的 `root@R1:~#` Shell；见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：运行中的内核为 Linux `5.10.110 #4 SMP`，目标架构为 `aarch64`；`/etc/os-release` 确认用户空间为 Ubuntu 22.04 LTS（Jammy Jellyfish，`ID=ubuntu`、`ID_LIKE=debian`）。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：R1 当前运行内核导出 root 可读的 `/proc/config.gz`，长度为 41579 字节；这是 Linux 内核构建配置入口，不是 U-Boot `.config`。后续可用它核对本板实际启用的驱动选项，见[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。
- **已验证**：R1 当前运行内核版本为 `5.10.110`，`/lib/modules/5.10.110` 整个目录不存在，因此其下的 `build`、`source`、模块文件和模块元数据也均不可用。当前厂商镜像没有提供本机同版本 headers/source/modules 入口；这不影响运行中的驱动与 `/proc/config.gz`，但源码级阅读和模块构建必须依赖外部对应源码。见[EXP-20260812-003](../experiment/exp-20260812-003-check-running-kernel-source-availability.md)。
- **已验证**：当前运行内核的 GPIO 核心与 Rockchip GPIO 控制器支持均为内建：`CONFIG_GPIOLIB=y`、`CONFIG_GPIO_ROCKCHIP=y`。这与 R1 的设备树 `rockchip,gpio-bank` 节点及运行时 SoC gpiochip 相容，但不证明任意排针都已复用为 GPIO 或可安全驱动外部电路。见[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。
- **已验证**：尝试从 Linux stable 官方 raw URL 读取 `gpio-rockchip.c` 时，主机获得 HTTP 404。网络可到达该服务器，但推测的仓库路径/ref/接口参数组合不可用。随后以 Git ref 查询确认官方 `linux-5.10.y` 维护分支存在，查询结果为 `acccef89f184a697fee1c96a1dc9cccbba36937b`；`v5.10.110` 标签查询遇到 TLS EOF、退出码 128，标签状态仍未知。学习者通过浏览器保存的 `build/local/r1-20260812/gpio-rockchip.c`（21276 B，SHA-256 `0e8b196719891df0b9b56e886d568cb83d5bb0744294956884aa085ae8b7f181`）含 `rockchip,gpio-bank` match table 与 Rockchip `platform_driver` 定义；学习者称其来自主线 7.1，故它仅作通用阅读样本，不能替代 R1 厂商源码。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：学习者已阅读该通用样本的 `rockchip_gpio_match` 与 `rockchip_gpio_driver` 定义，能区分 DTS 的设备属性描述、`.of_match_table` 的匹配资格和 `.probe` 的初始化职责。相应的通用机制见[设备树到 platform driver 绑定笔记](../note/device-tree-platform-driver-binding.md)，代码阅读证据见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：学习者已读懂主线 GPIO `probe` 开头的对象准备：`pdev` 经 `pdev->dev` 取得通用 device，`dev->of_node` 取得关联 DTS 节点，`of_get_parent(np)` 取得父 pinctrl 节点，后续将取得 pinctrl 对象和 Rockchip GPIO bank。此处尚未操作 GPIO 硬件，见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：学习者正确区分了 probe 的两类前提问题：DTS 节点或其父 pinctrl 节点缺失应使本轮 probe 返回 `-ENODEV`；若节点存在而 pinctrl provider 尚未准备，应延迟后重试。样本代码的实际延迟返回值仍待读后续分支确认，见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本代码确认 `of_pinctrl_get()` 从父 DTS 节点取得运行时 pinctrl 对象；若当前不可得，`!pctldev` 返回 `-EPROBE_DEFER` 请求 probe 以后重试，而非在函数内等待。随后驱动优先以 `of_alias_get_id(np, "gpio")` 读取 DTS `gpio` alias 编号；R1 已验证有 `gpio0`–`gpio4` aliases。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本 GPIO probe 在查到 bank 后将其关联到当前 `device` 与 DTS 节点，初始化 `raw_spinlock_t` 类型的自旋锁 `slock`，并以 `ret` 向上传播 `rockchip_get_bank_data()` 的失败码。后者开始获取 MMIO、IRQ、时钟和版本寄存器等硬件资源；`slock` 不是时钟。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本 GPIO 驱动不从通用 `device` 直接取 bank；它经 `pinctrl_dev_get_drvdata(pctldev)` 取得 Rockchip pinctrl 驱动私有数据，再遍历 `pin_banks`，以 `bank_num == id` 找到具体 GPIO bank。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本 `rockchip_get_bank_data()` 从 bank DTS 节点分别取得 MMIO、IRQ 和时钟资源：`irq_of_parse_and_map()` 将第 0 项中断描述映射为 Linux IRQ 编号但未安装处理函数；`of_clk_get()` 取得第 0 项 `struct clk *` 句柄但尚未启用时钟，失败时用 ERR_PTR / `IS_ERR()` / `PTR_ERR()` 处理。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本在读取 GPIO 版本寄存器前启用主时钟，并按硬件版本选择 V1/V2 寄存器偏移表。V2 还从 DTS 第 1 项 `clocks` 取得硬件消抖所需的 `db_clk`；该步骤失败会禁用主时钟后返回错误。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本在 GPIO controller 注册前用 `deferred_lock` mutex 防止延迟引脚配置与注册/后续处理竞争；它不是硬中断保护。注册失败时会禁用主时钟、解锁并返回原始错误码，见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本通过 `gpiochip_add_data(gc, bank)` 将已准备好的具体 bank 注册给 GPIO core，并将 `bank` 作为 controller 私有数据；这与 R1 的运行时 `/dev/gpiochipN` 接口在通用机制上相连，但其编号不能由 legacy base 或排针编号直接推出。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：GPIO core 向 `.set`/`.get` 等通用回调传入 `gpio_chip`，样本回调再以 `gpiochip_get_data(gc)` 取回注册时绑定的私有 `bank`，从而访问该 bank 的 `reg_base`、寄存器布局和锁。私有数据让同一回调模板可服务多个具体 bank。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：样本 `gpio_chip` 模板已列出 GPIO core 的操作路由：请求/释放、读/写、查询/设置方向、配置和 IRQ 转换均分派到对应 Rockchip 回调。该表本身不操作硬件，具体请求发生时才执行回调。见[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
- **已验证**：学习者理解 `of_get_parent()` 返回共享 `device_node` 对象的指针和一次额外引用，而不是节点信息副本；`of_node_put()` 仅归还该临时引用。引用计数保证节点生命周期，且避免复制复杂设备树数据，见[设备树到 platform driver 绑定笔记](../note/device-tree-platform-driver-binding.md)。
- **已验证**：学习者取得的本地 `R1/` 是 youyeetoo 官方 GitHub 文档索引仓库，HEAD 为 `816bd5413ab9a6470f7c3b3add98d9f795ff6378`，不是浅克隆且工作树干净；它仅含 README、GPL-3.0 LICENSE 和 Git 元数据，不含内核源码、U-Boot 源码、固件或 Loader。它提供逐针表、原理图/2D 下载和 Ubuntu 源码编译入口链接，见[资料档案](../resource/youyeetoo-r1-documentation-repository.md)。
- **已验证**：R1 运行内核将 `STMMAC_ETH`、`STMMAC_ETHTOOL`、`DWMAC_ROCKCHIP`、`DWMAC_ROCKCHIP_TOOL` 和 `REALTEK_PHY` 均内建（`=y`）；这为本板的 `snps,dwmac-4.20a`/`rockchip,rk3588-gmac` 设备树节点与运行时 `st_gmac`/`rk_gmac-dwmac`/RTL8211F PHY 驱动关系提供构建侧证据。见[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。
- **已验证**：本次启动中 `rk_gmac-dwmac` 对 `fe1c0000.ethernet` 完成 probe，绑定 RTL8211F 外置 PHY、注册 PTP 时钟，并使 `eth0` 协商到 1 Gbps 全双工。部分 IRQ、regulator 和 clock 资源未找到，但初始化和链路协商已成功；其具体影响仍待验证。见[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。
- **已验证**：持久化重提取的 p3 FIT 内 Linux FDT SHA-256 为 `abd1c6c320e5de7cd91c90d1c56eb10822be19c4015d75259aaf04a96fe87546`，与先前 FIT 声明的 `fdt` 载荷一致。其 `ethernet@fe1c0000` 节点中的 RGMII RXID、输入时钟与 TX/RX 延时值均和 GMAC probe 日志逐项一致；`phy-handle = <0x103>` 已解析为 `mdio/phy@0`，MDIO 地址为 0，运行时路径为 `stmmac-1:00`、PHY ID 为 `0x001cc916`，并绑定 `RTL8211F Gigabit Ethernet` PHY 驱动。见[EXP-20260812-001](../experiment/exp-20260812-001-locate-running-kernel-config.md)。
- **已验证**：在持久化 FIT 内 Linux DTS 中搜索常见 `gpio-leds`/`gpio-led`/`leds` 节点没有匹配，`rg` 退出状态为 1；这不证明 R1 没有 LED，也不证明运行时不存在 LED 类设备。当前不向任何 GPIO 写值，见[EXP-20260812-002](../experiment/exp-20260812-002-select-first-simple-device.md)。
- **已验证**：R1 运行时 `/sys/class/leds` 注册了 `mmc0::`，它解析到 `fe2e0000.mmc` 平台 MMC 控制器下的 LED 对象；其 `/trigger` 属性不存在，但导出了 `brightness`（root 可写）和 `max_brightness`（只读），值域为 0–255。写入 255 后可读回 255，但学习者观察两颗实体 LED 均无可见变化；该对象不作为当前可见 GPIO LED 实验候选。**用户提供**：已恢复其原始逻辑值 0，恢复读回原始输出未保留。见[EXP-20260812-002](../experiment/exp-20260812-002-select-first-simple-device.md)。
- **已验证**：R1 运行内核导出了 6 个 GPIO character-device 节点 `/dev/gpiochip0` 至 `/dev/gpiochip5`，均为仅 root 可读写的字符特殊文件（主设备号 254，次设备号 0–5）。root Shell 中 `command -v gpioinfo` 无输出；目前只能确认该工具不可直接调用，不能据此判断安装状态。控制器编号不等同于 SoC bank 或物理排针编号，见[EXP-20260812-002](../experiment/exp-20260812-002-select-first-simple-device.md)。
- **已验证**：R1 保存的 FIT 内 Linux DTS 的 aliases 声明 `gpio0` 至 `gpio4`，分别指向 5 个 `rockchip,gpio-bank` 节点（`fd8a0000`、`fec20000`、`fec30000`、`fec40000`、`fec50000`）；每个节点均声明 `gpio-controller`、`#gpio-cells = <2>` 和大小为 32 的 `gpio-ranges`。运行时 legacy sysfs 的 label、线数和设备路径逐一匹配这 5 个 SoC bank；额外控制器是 SPI2 上 RK806 PMIC 提供的 `rk806-gpio`，有 3 条线。通过 `/sys/dev/char/254:<minor>` 已确认本次启动的精确映射：`/dev/gpiochip0`–`4` 依次对应 SoC `gpio0`–`4`（legacy base 0、32、64、96、128），`/dev/gpiochip5` 对应 RK806 GPIO（base 509）。两套编号不能混用，换内核后应重新验证。见[EXP-20260812-002](../experiment/exp-20260812-002-select-first-simple-device.md)与[GPIO 控制器笔记](../note/gpio-controller-and-pinctrl.md)。
- **已验证**：根文件系统 `/` 挂载自 `/dev/mmcblk0p6`。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`mmcblk0` 类型为 `MMC`，根文件系统位于 eMMC；登录欢迎信息表明用户空间为 Ubuntu 22.04 LTS。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：systemd 默认目标为 `graphical.target`；图形会话的实际显示输出尚未验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`/proc/cmdline` 包含 eMMC 根分区、Debug UART 控制台和 Android 兼容字段；详见[启动参数笔记](../note/linux-kernel-command-line.md)。
- **已验证**：当前运行时设备树 `model` 为 `Rockchip RK3588S EVB4 LP4X V10 Board`，其中包含 SoC 型号 RK3588S，但未出现 R1 商品名；这不能单独推翻实物为 R1 的用户提供信息。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：运行时设备树 `compatible` 依次为 `rockchip,rk3588s-evb4-lp4x-v10` 与 `rockchip,rk3588`，即由具体评估板描述回退到通用 SoC 描述；详见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：内核导出 root 只读的运行时 FDT 二进制 `/sys/firmware/fdt`，大小为 151552 字节；主机 SCP 副本长度和 FDT v17 头部字段一致，SHA-256 为 `51cb9beb30f4b6221d13aa8c85bef9d957cea86afd8c164dbb72c356205d068c`。它可用于研究本次启动使用的设备树，但不自动表明 eMMC 中的 DTB 文件来源。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`file` 将运行时 FDT 识别为 Device Tree Blob version 17；该版本是二进制格式版本，大小为 151552 字节，不能当作板型或构建版本。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：当前板端 Root Shell 的 `PATH` 中没有可直接调用的 `dtc`；后续反编译工具应优先在 Arch 主机侧确认，避免为阅读 DTB 改动板端环境。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机已安装 `dtc` 包 `1:1.8.1-1`，DTC v1.8.1 可执行；于 2026-08-07T20:53:39+08:00 单独指定安装，包元数据报告数字签名验证。见[dtc 工具记录](../tool/dtc.md)。
- **已验证**：Arch 主机可调用 `aarch64-linux-gnu-gcc`，其目标三元组为 `aarch64-linux-gnu`、版本为 GCC 16.1.0；可作为后续只在主机构建 AArch64 代码的工具链。它不同于当前厂商 U-Boot 构建日志中的 GCC 10.3.1 工具链，尚不用于复现厂商二进制。见[交叉编译器记录](../tool/aarch64-linux-gnu-gcc.md)。
- **已验证**：Arch 主机当前可调用 `make`、`bison`、`flex`、`bc`、`openssl`、`python`、`dtc` 和 `aarch64-linux-gnu-gcc`，具备开始 U-Boot 源码阅读和首次主机构建尝试的基础命令；尚未验证具体源码版本或 R1 配置。见[实验 EXP-20260809-003](../experiment/exp-20260809-003-check-uboot-host-build-prerequisites.md)。
- **已验证**：主机已取得干净的上游 U-Boot `v2026.07` 浅克隆；精确提交为 `ece349ade2973e220f524ce59e59711cc919263f`。该树是主机构建和阅读基线，不是已确认适配 R1 的厂商源码，也不能直接用于烧录。见[源码档案](../resource/u-boot-v2026-07-upstream-source.md)与[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游源码中的 `arch/`、`board/`、`boot/`、`cmd/`、`common/`、`configs/`、`doc/`、`drivers/` 和 `include/` 顶层目录均存在；下一步从 `configs/` 只读筛选 RK3588 候选，不把任意候选当作 R1 配置。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游 U-Boot `v2026.07` 有 30 份名称匹配的 RK3588/RK3588S defconfig，但没有 `r1` 或 `youyeetoo` 命名项；当前只能视作候选清单，不能选择任一配置构建或烧录 R1。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游 `evb-rk3588_defconfig` 的默认设备树为 `rockchip/rk3588-evb1-v10`，默认 DTB 文件为 `rockchip/rk3588-evb1-v10.dtb`；二者与 R1 运行时 `rk3588s-evb4-lp4x-v10` 不同。该候选的 Debug UART 基址 `0xFEB50000` 与波特率 1500000 与本板已观察的 U-Boot 串口一致，且启用了 FIT/SPL/ATF 相关选项；这些局部一致性不构成 R1 适配或烧录依据。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游 EVB1 的完整 DTS 位于 `dts/upstream/src/arm64/rockchip/rk3588-evb1-v10.dts`，同名 U-Boot `.dtsi` 位于 `arch/arm/dts/rk3588-evb1-v10-u-boot.dtsi`。当前源码树将二者分开放置；二者如何在构建时关联或合并待验证。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游 EVB1 基础 DTS 引用 `rk3588.dtsi`，根节点声明 EVB1 V10 的 `model`/`compatible`、`ethernet0` 与 `mmc0` 别名，且 `chosen/stdout-path` 为 `serial2:1500000n8`。其中串口编号/波特率与 R1 已观察现象相似，但 EVB1 板级标识与 R1 EVB4 LP4X 标识不同，不能据此推断适配。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游 EVB1 的 U-Boot 专用 `.dtsi` 引用 `rk3588-u-boot.dtsi`，并在 `/chosen` 声明 `u-boot,spl-boot-order = "same-as-spl", &sdhci;`。该属性的精确语义、`&sdhci` 的具体节点和与基础 DTS 的合并路径尚未验证。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游树中的 `doc/device-tree-bindings/chosen.txt`、`arch/arm/mach-rockchip/spl-boot-order.c` 与多个板级 `.dtsi` 均出现 `u-boot,spl-boot-order`；Rockchip Makefile 在 `CONFIG_SPL_ROCKCHIP_COMMON_BOARD` 条件下加入 `spl-boot-order.o`。属性的完整语法和 `same-as-spl` 规则尚未读取。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：同树 `chosen.txt` 规定 `u-boot,spl-boot-order` 是按书写顺序探测的设备列表，元素可为节点引用、完整路径或 alias。`same-as-spl` 在其位置请求插入 SPL 的启动来源，但是否插入由具体 SoC/板级实现决定，且可能与显式项重复。EVB1 的 `"same-as-spl", &sdhci` 因而表示“先尝试 SPL 来源、再尝试 `&sdhci`”的通用意图；Rockchip 实现是否执行前项尚未验证。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：上游 Rockchip v2026.07 的 `board_boot_order()` 按顺序读取 `/chosen/u-boot,spl-boot-order`，将 `same-as-spl` 替换为 `board_spl_was_booted_from()` 的返回值，再依次解析 alias、FDT 路径与 `BOOT_DEVICE_*` 映射；无 FDT、无 `/chosen` 或无有效匹配时回退 `spl_boot_device()`。当前 R1 厂商 U-Boot 2017.09 是否相同待验证。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：`board_spl_was_booted_from()` 与 `spl_node_to_boot_device()` 的上游 Rockchip v2026.07 入口已定位；为避免脱离当前目标，暂不逐行追踪实现，结论与回溯入口见[SPL 启动顺序笔记](../note/uboot-spl-boot-order.md)。
- **已验证**：首次生成 EVB RK3588 隔离 defconfig 的命令因运行时已在源码目录、却重复使用 `-C src/u-boot-upstream` 而停止；未形成可验证 `.config`，不涉及开发板或源码修改。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已验证**：更正后的 EVB RK3588 defconfig 命令运行完成，输出 `configuration written to .config`；日志仅含主机侧 Kconfig 工具构建，尚未出现 AArch64 目标编译或镜像生成。隔离输出目录中的 `.config` 已核对为 57552 字节普通文件；所查默认 DTS、默认 DTB、Debug UART 基址/波特率及 FIT/SPL/ATF 项均与 defconfig 一致。见[实验 EXP-20260809-004](../experiment/exp-20260809-004-acquire-upstream-uboot-source.md)。
- **已解决**：上游 `pylibfdt` 在 Python 3.14.6 + SWIG 4.5.0 下的旧 API 编译失败，已在本地实验分支以两行补丁越过；同一构建进入 `BINMAN .binman_stamp`，未重现 `PyInt_AsLong`、`PyString_FromString` 错误。该补丁尚未提交，且仅是主机学习构建兼容性修复，不代表 R1 适配。见[ISSUE-20260811-001](../issue/issue-20260811-001-uboot-build-missing-swig.md)。
- **已验证**：当前上游 EVB RK3588 构建在 Binman 打包 `simple-bin` 时缺少外部 `rockchip-tpl` 与 `atf-bl31`，因此报告 non-functional 并停止；`tee-os` 被标为 optional。最终 `.config` 已确认 `CONFIG_ARM64=y`、`CONFIG_ROCKCHIP_EXTERNAL_TPL=y`、`CONFIG_SPL_ATF=y`，与 Rockchip 通用 Binman DTS 的外部 TPL、AArch64/BL31 条件节点对应。上游文档说明 ARM64 Rockchip 镜像需要 TF-A，特定 SoC 的 TF-A 未公开时可采用 Rockchip 提供的 BL31，并为 `evb-rk3588_defconfig` 给出 `rk3588_bl31_v1.33.elf`、`rk3588_ddr_lp4_2112MHz_lp5_2736MHz_v1.09.bin` 的示例。EVB 默认 DTS 根节点为 `rk3588-evb1-v10`，而 R1 运行时根节点为 `rk3588s-evb4-lp4x-v10`；仅共有通用 `rockchip,rk3588` 回退项，故该示例不是 R1 适配或烧录依据。外部载荷尚未下载、核验或使用。见[ISSUE-20260811-002](../issue/issue-20260811-002-uboot-missing-external-boot-blobs.md)与[外部启动载荷笔记](../note/rockchip-external-boot-blobs.md)。
- **已验证**：板端 Bash 报告 `dumpimage: command not found`，当前不能直接使用该工具结构化解析 FIT；这不影响已取得的 FIT 格式证据。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：板端 `/usr/bin/base64` 可直接调用，可作为经 UART 传输少量 FIT 元数据的编码工具；编码传输完整性尚待验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Base64 手工传输后的主机临时文件 `/tmp/r1-p3-fit.dtb` 长度为 1536 字节，且主机 `file` 重新识别为预期的 Device Tree Blob；传输内容的密码学完整性和 FIT 载荷哈希仍未验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机 DTC v1.8.1 已将该临时 FIT 元数据成功反编译为 `/tmp/r1-p3-fit.dts`。默认配置 `conf` 选择 `fdt`、`kernel` 与 `resource`；三者的偏移、大小和 SHA-256 声明已读取。FIT 中 `fdt` 为 147826 字节，与 151552 字节运行时 FDT 不同。见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)。
- **用户提供**：主机重启后 `/tmp` 中的分析文件会丢失；后续将 `/tmp` 限定为一次性中转，将可重复生成的本机分析文件保存到 Git 忽略的 `build/local/`，并在文档中记录来源与校验。该目录策略已建立，尚待重新提取 DTB 验证。
- **已验证**：板端当前 Shell 中没有可直接调用的 `sz`，因此尚无可确认的 ZMODEM 发送链路；没有为此安装软件。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`p3` 偏移 `0x800`、长度 147826 字节的实际 `fdt` 载荷 SHA-256 已匹配 FIT 声明值；这不验证 U-Boot 已执行哈希或签名检查。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：该 FIT `fdt` 载荷含有与运行时 FDT 相同的根 `model` 文本 `Rockchip RK3588S EVB4 LP4X V10 Board`，以及相同的根 `compatible` 文本 `rockchip,rk3588s-evb4-lp4x-v10`、`rockchip,rk3588`。完整 DTS diff 为 73 行、5 个差异块，确认运行时树额外含内存、显示、MAC、启动参数和 logo 缓冲区等有限数据。**推测**：这是启动加载器或更早固件的 DTB fixup，具体写入组件待查。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：学习者已在主机读取 FIT 内 Linux DTS 根节点：`#address-cells` 与 `#size-cells` 均为 `<0x02>`，`aliases` 中 `ethernet1` 映射到 `/ethernet@fe1c0000`。这些是硬件描述/引用信息，不直接证明驱动绑定成功。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：学习者沿 `ethernet1` 读取到 FIT 内 Linux DTS 的 `ethernet@fe1c0000`：声明 Rockchip GMAC/Synopsys DW MAC 兼容项、地址 `0xfe1c0000` 的 64 KiB 寄存器范围与 `status="okay"`。该启用状态不单独证明运行时驱动绑定。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：R1 的 `ethtool -i eth0` 报告运行时驱动为 `st_gmac`；该结果确认网络接口已绑定驱动，但空的 `bus-info` 与 `firmware-version` 不单独表示故障，设备—驱动 sysfs 链接尚待读取。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`/sys/class/net/eth0/device/driver` 解析为 `/sys/bus/platform/drivers/rk_gmac-dwmac`，确认 `eth0` 底层设备已绑定该平台驱动；这与 ethtool 的 `st_gmac` 是不同接口呈现的名称，模块细节待确认。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`/sys/class/net/eth0/device` 解析为 `/sys/devices/platform/fe1c0000.ethernet`；地址与 DTS 的 `ethernet@fe1c0000`、`reg` 起始地址一致，已形成 `eth0 → 平台设备 → 平台驱动` 的 sysfs 关联。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`/sys/class/net/eth0/device/of_node/compatible` 输出 `rockchip,rk3588-gmac`、`snps,dwmac-4.20a`，与 FIT 内 Linux DTS 的 GMAC 节点兼容列表一致；本板已完成“运行时设备树 compatible → 平台设备 → 已绑定驱动 → eth0”的证据链。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已整理**：上述证据链已沉淀为“设备树节点 → 平台设备 → 驱动 → 网络接口”的最小驱动模型；其中 `compatible` 是匹配依据，sysfs 链接只是运行结果的观察入口。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：`eth0` 运行时 OF 节点的 `status` 输出为 `okay`；它证明节点被启用，但不替代驱动绑定、网络接口注册或网络连通性的证据。见[设备树笔记](../note/device-tree-model-and-compatible.md)。
- **已验证**：板端 `eth0` 曾标志为 `UP,LOWER_UP`，已建立与 Arch Linux 主机的物理以太网链路；随后未获 IPv4/IPv6 地址，NetworkManager 状态变为 `disconnected`。直连主机并不自动提供 DHCP、路由或互联网，当前尚不能用于 IP 传输。`lo` 仅本机回环，`can0` 当前 DOWN，均不能用于主机传输。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`NetworkManager.service` 为 `active (running)`，它是当前 `eth0` 连接与 DHCP 状态的直接观察入口；其是否为该接口创建并激活连接尚待查询。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：NetworkManager 曾将 `eth0` 关联到 `Wired connection 1` 并尝试获取 IP，随后状态为 `disconnected`。这不单独证明 DHCP 服务不存在；网线对端为 Arch 主机，主机侧网络配置仍待检查。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机的 `enp108s0` 是当前已连接、标志为 `UP,LOWER_UP` 的直连以太网接口；其 IP、DHCP 服务和连接共享状态尚待检查。主机 `wlo1` 是独立的 Wi-Fi 连接。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机直连口 `enp108s0` 配置为 `192.168.0.1/24`，另有 IPv6 链路本地地址；DHCP 服务和互联网共享状态尚待检查。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机 `enp108s0` 的 NetworkManager 配置为 `ipv4.method=manual`、`ipv4.addresses=192.168.0.1/24`，不是 `shared` 模式；不能预期它自动给 R1 分配 DHCP 地址。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：R1 在运行时为 `eth0` 临时配置 `192.168.0.2/24` 后，ping 主机 `192.168.0.1` 获得 2/2 回复、0% 丢包。主机—板子同网段 IPv4 通信已验证；该临时地址不持久化，互联网转发和 DNS 尚未验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：R1 的 `Wired connection 1` 使用 `ipv4.method=auto`，是 DHCP 客户端模式；主机启用共享后可直接请求租约。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：主机 `enp108s0` 的共享网段地址为 `10.42.0.1/24`；R1 通过 DHCP 自动获得 `10.42.0.192/24`，并收到主机的三次 ICMP 回复。主机—板子自动地址与直连通信已建立；DNS 与经 `wlo1` 的外网转发尚未验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已解决**：R1 共享 DHCP 未获租约的根因是 UFW 丢弃了 UDP 67 Discover；最小放行后回归通过。详见[ISSUE-20260809-003](../issue/issue-20260809-003-r1-dhcp-lease-missing.md)。
- **已验证**：R1 ping 外部 IPv4 `1.1.1.1` 为 3/3 超时；学习者报告 Arch 主机可 ping 百度 IP。共享 NAT 外网转发尚未完成，但学习者当前只需要主机—板子直连通信，已归档为后续任务。详见[ISSUE-20260809-004](../issue/issue-20260809-004-ufw-blocks-shared-nat-forward.md)。
- **已验证**：R1 的 `ssh` systemd 服务为 `active`，主机 SSH 公钥登录已回归成功并输出 `root`、`R1`；它现在可作为完整运行时 FDT 的只读传输通道。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **用户提供**：学习者不知道板端 root 的 SSH 密码；串口启动后直接进入 root Shell。该本地自动登录行为不代表 SSH 密码或认证策略。
- **已验证**：`passwd -S root` 显示状态 `P`，root 账户设置过密码；密码内容未知，密码状态上次变更日期为 2024-08-23。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：sshd 有效配置为 `permitrootlogin yes`、`passwordauthentication yes`、`pubkeyauthentication yes`、`kbdinteractiveauthentication no`。root 密码未知，后续优先采用公钥认证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：Arch 主机已有 `~/.ssh/id_ed25519.pub`，可作为 R1 的授权公钥；私钥保留在主机。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：R1 root 当前没有 `.ssh` 或 `authorized_keys`，新建授权文件不会覆盖现有 root 公钥。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：主机公钥已写入 R1 root 的 `authorized_keys`，目录/文件权限为 700/600，均归 root 所有；公钥认证尚待主机侧验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已解决（镜像更新后的复发）**：R1 sshd 启用 `StrictModes=yes`；镜像更新后 `/root` 曾再次错误归 `youyeetoo:youyeetoo` 所有且授权文件缺失，导致 SSH 请求密码。恢复 `/root` 为 `root:root`、创建安全 `.ssh` 并直接写入主机 Ed25519 公钥后，显式私钥、禁用密码的 SSH 回归输出 `root`、`R1`、`target-absent`。详见[ISSUE-20260809-005](../issue/issue-20260809-005-r1-ssh-public-key-rejected.md)。
- **已验证**：当前已挂载根文件系统的 `/boot` 为空，未发现内核镜像、DTB、`extlinux/` 或其他启动配置；这不排除 eMMC 独立分区中存在启动组件。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：当前 eMMC 有 8 个分区；`p1` 标签为 `uboot`，`p3` 标签为 `boot`，`p6` 为挂载到 `/` 的 ext4 `rootfs`。`p1` 起始为 2560 字节 FDT v17，主机反编译确认它是 RK3588S EVB4 平台设备树，而非 FIT 元数据；全分区文本另含 ATF、OP-TEE、U-Boot、MCU FIT 与 U-Boot `2017.09-g33a7c066a8-dirty #youyeetoo1` 构建标识，所属对象待定位。`p3` 开头的 1536 字节 FDT 已确认为包含 `fdt`、`kernel`、`resource` 的 Linux FIT 元数据树。详见[eMMC 分区笔记](../note/r1-emmc-partition-layout.md)与[FIT 笔记](../note/uboot-fit-image.md)。
- **已验证**：主机通过 SSH 只读提取的 `/tmp/r1-p1-fit.dtb` 为 2560 字节普通文件，与 `p1` 起始 FDT 长度一致；DTC 已将其反编译为平台设备树。文件名是先前 FIT 假设留下的命名，不代表其实际语义。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：主机 `/home` 所在文件系统可用空间为 186 GiB，足够容纳当前约 29 GiB eMMC 原始镜像并留出校验空间；后续已完成镜像导出和校验。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：完整备份的源路径为 R1 的 `/dev/mmcblk0`（28.8 GiB），目标目录为主机 `~/Study/rk3588-backup`；在线读取完成后长度和 SHA-256 校验均通过。在线读取根文件系统会得到块级快照，不能等同于离线文件系统一致性备份。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：完整 eMMC 在线只读导出命令以 `ssh/dd exit=0` 结束，主机镜像位于 `~/Study/rk3588-backup/r1-emmc-20260809.img`；远端 eMMC 与镜像长度均为 30924603392 字节，镜像 SHA-256 为 `62b683c11cfbc5870ccfc98e9b0b334d5e2ab88a33d1d3f83f13a384a4a938d0`，`sha256sum -c` 已报告成功。在线根文件系统的一致性边界不变；尚未进行恢复演练。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **用户提供 + 已验证**：学习者已下载名为 `R1_UbuntuCamera_ImageV2V3.img` 的官方 Ubuntu camera 候选镜像；主机本机文件为 9,639,807,562 字节，SHA-256 为 `55cd40508c70f48d05f411f9103f9cbb004456a05574e6f473007df78e2c758f`，起始 ASCII 为 `RKFW`，即 Rockchip 固件容器而非 raw eMMC 镜像。已安装 `rkdeveloptool` 的帮助没有完整 `RKFW` 统一固件升级子命令；其 `ul` 只接收 Loader，`wl`/`wlx`/`gpt`/`ef` 不能替代。youyeetoo R1 eMMC 页面明确的是 Windows GUI `RKDevTool` 的统一固件流程；其 macOS raw 镜像示例同样不能套用。直接来源 URL、厂商校验值、容器内载荷、R1 V2 适用性、可验证的 Arch Linux 客户端和可重复烧录步骤仍待确认；详见[资源档案](../resource/r1-ubuntu-camera-image-v2-v3.md)与[EXP-20260815-001](../experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image.md)。
- **用户提供**：学习者报告已使用上述候选镜像完成固件更新；烧录工具/操作、设备模式和客户端输出未保存。该报告尚不等同于“新系统已从 eMMC 正常启动”或“NPU LLM 已可用”；首先需通过 Debug UART 进行只读启动验证，见[EXP-20260815-001](../experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image.md)。
- **已验证**：在上述更新报告后，R1 的 Debug UART root Shell 输出 Ubuntu 22.04、`Linux R1 5.10.110 #4 SMP Sun Sep 29 10:38:13 CST 2024`，根文件系统为 `/dev/mmcblk0p6`，systemd 状态为 `degraded`。这验证系统可从 eMMC 启动；与更新前的版本字符串相同，不能单靠此证明候选文件的精确内容。见[EXP-20260815-001](../experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image.md)。
- **已验证**：更新后 `systemctl --failed --no-pager` 只列出 `rockchip.service`，不再列出更新前曾失败的 `apport-autoreport.service`。因此当前 `degraded` 至少由 `rockchip.service` 直接造成；前者未被列出不等同于其根因已解决。见[ISSUE-20260810-001](../issue/issue-20260810-001-systemd-degraded-failed-units.md)。
- **已验证**：Arch Linux 主机的 `/usr/bin/rkdeveloptool` 将设备报告为 `DevNo=1 Vid=0x2207,Pid=0x350b,LocationID=106 Maskrom`；见[实验 EXP-20260805-001](../experiment/exp-20260805-001-identify-rockusb-device.md)。
- **已验证**：当前项目已经是有效 Git 工作树；当前分支为 `master`。必要的 `.gitignore`、`AGENTS.md` 和 `doc/` 共 39 个文件已纳入首次提交 `2ed79a1`（`docs: initialize RK3588 learning repository`）。见[ISSUE-20260809-002](../issue/issue-20260809-002-read-only-git-mount.md)。
- **资料记载**：R1 使用 RK3588S，32 GB 配置为板载 eMMC 可选规格；见[硬件环境基线](../environment/hardware.md)。
- **推测**：先前进入 MaskROM 可能由按键、上电/连接顺序或当时的启动介质状态触发；该推测尚未解释为何已可启动的 eMMC 当时未被加载。
- **已验证**：`rk3588_spl_loader.bin` 曾被观察到存在且大小为 0 字节，随后已不存在。
- **用户提供**：该文件由学习者主动删除；具体删除时间未记录。相关问题已归档，详见 [ISSUE-20260805-001](../issue/issue-20260805-001-empty-loader.md)。

## 进度

| 项目 | 状态 | 完成证据 |
| --- | --- | --- |
| 明确总体学习方向 | 已完成 | 本文“已知事实” |
| 确定近期项目的 NPU LLM 核心约束 | 已完成 | [DEC-20260813-003](../decision/dec-20260813-003-npu-llm-required-project-core.md) |
| NPU 厂商初始化脚本的首轮定位 | 进行中 | RK3588 分支与 NPU tar、清理、first-boot 标记已定位；外层条件待读，见 [EXP-20260814-001](../experiment/exp-20260814-001-inspect-r1-npu-first-boot-script.md) |
| NPU 最小运行时链路 | 已验证（RAM 候选） | RKNPU 0.9.8 候选进入 Linux 5.10.252，`renderD128` 绑定 `RKNPU`，同一 W8A8 模型已生成 `Alright,`；当前候选不含 Rockchip 显示 DRM/Mali GPU，且未写 eMMC，见 [ISSUE-20260815-002](../issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed.md) |
| AMP 运行时前置盘点 | 已完成首轮 | 原厂 R1 未启用 `ROCKCHIP_AMP`，也未暴露 remoteproc/RPMsg 或 Zephyr carveout；8 CPU 经 PSCI 管理。已验证的 NPU 候选内核已启用 AMP/RPMsg，R1 内存与 DTS 仍需适配，见 [EXP-20260817-001](../experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites.md) |
| Zephyr A55 独立 RAM 启动 | 已验证 | U-Boot `ext4load` 后以 `go 0x50000000` 启动 Zephyr v4.4.0 hello_world；Linux 未并行运行，见 [EXP-20260819-002](../experiment/exp-20260819-002-boot-zephyr-standalone-from-uboot.md) |
| 官方 Ubuntu camera 候选镜像身份核对 | 进行中 | 文件大小、SHA-256 与 Rockchip `RKFW` 容器格式已确认；容器内载荷、板型适用性和厂商校验待确认，见 [EXP-20260815-001](../experiment/exp-20260815-001-inspect-r1-ubuntu-camera-image.md) |
| 建立文档结构和记录规范 | 已完成 | [知识库首页](../home.md)与[记录规范](../recording-standard.md) |
| 建立 Obsidian 知识关联与查阅入口 | 已完成 | [DEC-20260807-001](../decision/dec-20260807-001-adopt-obsidian-vault.md)与[知识库首页](../home.md) |
| 确认准确板卡型号和硬件版本 | 部分完成 | 型号为 youyeetoo R1 V2；PCB 丝印仍未记录 |
| 建立主机与板卡通信链路 | 已完成 | Rockchip USB 和 Debug UART 通信均已验证；UART 可进入 Root Shell |
| 准备早期启动日志通道 | 已完成 | `picocom` 以 1500000 baud 接收可读内核日志并支持交互 |
| 验证 Linux 可启动 | 已完成 | 重新上电后获得内核日志和 `root@R1:~#`；见 EXP-20260807-002 |
| 启动链只读勘察 | 进行中 | 已直接观察 SPL → 早期 FIT 校验 → BL31/OP-TEE → U-Boot → Linux FIT `conf`；已确认 U-Boot `mmc 0` 就是 Linux eMMC |
| 上游 U-Boot 主机侧 defconfig | 已完成 | EVB RK3588 `.config` 已在隔离 `build/` 目录生成、路径核对，关键项与 defconfig 一致；尚未目标编译或烧录 |
| 上游 U-Boot 主机侧首次完整构建 | 已归档 | pylibfdt 已通过；外部载荷版本漂移的原因已定位到足够程度，学习者决定不再追溯或选择二进制；不涉及 R1 |
| U-Boot 构建输出盘点 | 已完成 | `.bin` 是裸程序与 DTB 的拼接且两个名称内容相同；`.img` 两名称内容相同但比 `.bin` 多 984 字节，封装待识别 |
| R1 GMAC 驱动证据链 | 已完成 | 内核配置内建 STMMAC/DWMAC，probe 绑定 RTL8211F 并使 eth0 协商至 1 Gbps；资源缺失日志影响待查 |
| 首个简单设备 GPIO 候选筛选 | 已完成 | 已排除不适合的 MMC LED，完成 DTS、运行时控制器来源和两套编号映射；厂商逐针表已归档但资料冲突待实物核对；见 EXP-20260812-002 |
| 运行内核本机源码入口检查 | 已完成 | `5.10.110` 的 modules、build、source 目录均不存在；须使用外部匹配源码；见 EXP-20260812-003 |
| GPIO 核心与 Rockchip 控制器构建方式 | 已完成 | `CONFIG_GPIOLIB=y`、`CONFIG_GPIO_ROCKCHIP=y`；见 EXP-20260812-001 |
| 上游 GPIO 源码阅读入口 | 进行中 | 已确认官方 `linux-5.10.y` 分支；保存主线 7.1 通用阅读样本，精确标签查询发生 TLS EOF；见 EXP-20260812-004 |
| DTS 到 platform driver 的基本绑定 | 已完成 | 已读 match table、`.of_match_table`、`.probe` 静态连接；见 EXP-20260812-004 |
| GPIO probe 对象关系 | 已完成 | 已读 `pdev → DTS 节点 → 父 pinctrl → GPIO bank`；见 EXP-20260812-004 |
| GPIO probe 的依赖分类 | 已完成 | 已区分 DTS 结构缺失与 pinctrl 未就绪；延迟返回值待读代码确认；见 EXP-20260812-004 |
| GPIO probe 的 pinctrl 依赖与 alias 入口 | 已完成 | `!pctldev` 返回 `-EPROBE_DEFER`，优先读取 DTS `gpio` alias；见 EXP-20260812-004 |
| GPIO bank 关联与资源入口 | 已完成 | 已区分 `dev`/DTS 关联、自旋锁、`ret` 与资源获取入口；见 EXP-20260812-004 |
| GPIO ID 到 bank 的运行时查找 | 已完成 | `pctldev → drvdata → pin_banks[] → bank_num`；见 EXP-20260812-004 |
| GPIO IRQ 与时钟资源入口 | 已完成 | DTS 中断映射为 Linux IRQ，取得时钟句柄但尚未启用；见 EXP-20260812-004 |
| GPIO 时钟启用与版本布局 | 已完成 | 启用主时钟、按 V1/V2 选偏移表、V2 取得消抖时钟并可回滚；见 EXP-20260812-004 |
| GPIO controller 注册前互斥与回滚 | 已完成 | mutex 保护 deferred 配置，失败时关时钟、解锁、返回；见 EXP-20260812-004 |
| GPIO bank 向 GPIO core 注册 | 已完成 | `gpiochip_add_data()` 注册 controller 并传入私有 bank；见 EXP-20260812-004 |
| GPIO 回调与私有 bank 数据 | 已完成 | 回调通过 `gpiochip_get_data()` 回到具体 bank 资源；见 EXP-20260812-004 |
| GPIO core 操作路由表 | 已完成 | `.get`/`.set`/方向/配置/IRQ 回调已定位；见 EXP-20260812-004 |
| 持久化本机分析区 | 已完成 | `build/local/r1-20260812/linux-fit-fdt.dtb` 已重新提取并以 SHA-256 核对；DTS 已用于 GMAC 属性/日志对照 |
| Linux 用户空间入口观察 | 已完成 | PID 1 为 `systemd`；`/sbin/init → /lib/systemd/systemd → /usr/lib/systemd/systemd`；见 [EXP-20260810-001](../experiment/exp-20260810-001-identify-linux-pid-1.md) |
| systemd 整体服务状态 | 进行中 | 更新后仍为 `degraded`，当前只剩 `rockchip.service` 失败；须重新确认直接失败命令，见 [ISSUE-20260810-001](../issue/issue-20260810-001-systemd-degraded-failed-units.md) |
| 备份或确认可恢复方案 | 部分完成 | 完整 eMMC 在线备份已导出、长度匹配且校验文件回归成功；厂商固件、恢复入口和写回流程仍待确认 |

## 当前阻塞与未知信息

- **待确认**：R1 V2 的 PCB 丝印、Type-C 实物接口标签与其具体针脚定义。
- **待确认**：完整启动链所在介质、图形会话是否实际出画、`p1` 中 FIT 文字所属对象和起始偏移、运行时 DTB 补充的具体写入组件和签名验证策略。共享 NAT 的实际出口、外部 IPv4 和 DNS 已延期，详见[ISSUE-20260809-004](../issue/issue-20260809-004-ufw-blocks-shared-nat-forward.md)。1536 字节 FIT 元数据的手工 Base64 传输已完成长度和 FDT 格式核对，但未做密码学完整性校验。
- **已验证**：当前启动日志直接显示 SPL 在其称为 `MMC1` 的 `0x4000 sector` 读取早期 FIT，逐项校验 ATF、U-Boot、FDT 和 OP-TEE 后跳转到 BL31/OP-TEE 与 U-Boot 本体。`0x4000` 与 U-Boot `mmc 0`（即 Linux eMMC）的 `uboot` 分区起点一致；SPL/第二阶段 U-Boot 的 MMC 编号不可直接按名称混同。`Hotkey: ctrl+m` 的准确中断条件待验证。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：第二阶段 U-Boot 显示自动启动中断键为 `CTRL+C`，在 Android 路径失败后选择 Linux FIT 的 `conf`，加载并 SHA-256 校验 kernel（35707392 B）与 FDT（147826 B）。二者与 p3 FIT 的偏移、大小和哈希一致；物理源分区未由该日志直印。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：学习者可在 `CTRL+C` 时中断自动启动进入 U-Boot `=>`，并以只读 `version` 确认第二阶段 U-Boot 的版本及 AArch64 GNU 工具链版本。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`bootcmd` 的候选顺序为 Android → FIT → RKP → distro；当前日志中 Android 失败后 FIT 成功启动，因此后两分支未运行。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：U-Boot 当前 `devtype=mmc`、`devnum=0`；`boot_android`、`boot_fit` 不在环境变量表，而是实际被启动脚本调用的内建命令。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：`boot_android` 的帮助说明其 BCB/Android 模式流程，`boot_fit` 的帮助说明其从内存或 `boot`/`recovery` 分区启动 FIT；二者解释 Android 失败后 FIT 回退的职责分工。其参数默认值待确认。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：U-Boot `mmc 0` 的 GPT 分区标签、LBA 范围和 Partition GUID 与 Linux `/dev/mmcblk0` 的 8 个分区一致；SPL 的早期 FIT LBA `0x4000` 与其中 `uboot` 分区起点一致。p1 起始 FDT 的厂商封装关系仍待解释。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：U-Boot 环境 `bootargs` 仅含 `storagemedia=emmc` 和 Android 兼容字段；当前 `fdt_blob=0x08300000` 的 `/chosen/bootargs` 与 FIT 内 DTB 一致，仅含根分区和串口参数，且无 `/memory` 节点；运行时 FDT 含两类补充。故参数/内存节点补充发生在当前 `=>` 提示符之后、`Starting kernel ...` 之前。**推测**：具体在 `boot_fit` 或更底层 Linux 启动流程中完成，代码路径待确认。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：U-Boot 环境 `kernel_addr_r=0x00400000`、`fdt_addr_r=0x08300000` 分别与本次日志中的 Linux kernel、FIT FDT 实际装载地址一致；`ramdisk_addr_r=0x0a200000` 已定义但本次是否使用未确认，`loadaddr` 未定义。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **已验证**：U-Boot 的可用 DRAM 分为 `0x00200000–0x083fffff` 和 `0x09400000–0xefffffff` 两段；kernel、FDT、ramdisk 候选地址均在可用区域，U-Boot 重定位和栈在高地址且不重叠。两段之间空洞与此前日志的 OP-TEE 保留范围相符。`fdt_blob` 指向 `0x08300000`，其具体 FDT 身份待读内容确认。见[实验 EXP-20260807-002](../experiment/exp-20260807-002-boot-linux-via-debug-uart.md)。
- **待确认**：电源规格、上电时 LED/显示表现和 Type-C 所在的实物接口标签。
- **待确认**：USB 转串口模块的芯片与逻辑电平；先前 MaskROM 与本次 Linux 启动的触发条件。

## 既往过程摘要（非当前下一步）

近期项目已调整为 R1 NPU 实际运行 LLM；GPIO 驱动样本阅读在操作路由表处暂停，不再作为唯一主线。候选镜像与当前 `rkdeveloptool` 的命令集不匹配：该工具没有接收完整 `RKFW` 的统一固件升级命令，绝不能以低层 LBA、分区、GPT 或擦除命令替代。

学习者再次确认采用项目牵引方式：不把 `rockchip.service` 的完整根因当作项目启动前置条件。候选 RKLLM demo 与 Runtime 已传入受限 `/userdata/rkllm-api-demo` 并完成 SHA-256 核对；主机 GCC 16 交叉产物的 ABI 不匹配已由 R1 原生 `g++` 重建解决，`llm_demo-r1` 已加载 Runtime 并进入 `main()`。不要升级 R1 的 glibc/libstdc++，也不要据此判定 RKNPU 硬件损坏。首次短 prompt 已在 W8A8 matmul 阶段失败；Runtime 已明确要求 driver >=0.9.7，R1 当前为 0.8.2。SDK 的 0.9.8 包只是内核驱动源码子树；公开厂商 Linux 5.10 候选的版本宏也确认是 0.8.2，不能作为升级答案。历史 `release-v1.2.1b1` Runtime 静态检查同样找到 `0.9.7` 与版本检查字符串。0.8.2→0.9.8 的紧凑 diff 显示新增独立 IOMMU/devfreq 文件，并修改几乎所有核心路径；它不是直接覆盖级别的小补丁。system monitor/IPA 已排除为当前阻塞，且 Kconfig 未显示差异。0.9.8 Makefile 无条件编译新增 IOMMU 对象，条件编译新增 devfreq 对象；运行 R1 同时启用 `CONFIG_PM_DEVFREQ=y`，两者都会参与构建。旧 R1 的 IOMMU domain alloc/free、`get_domain_for_dev`、DMA cookie 与 DMA ops 原型均已对照通过；新版实际执行 domain 迁移、IOVA/DMA 初始化和 DMA ops 重设，并维护缓存与实际 domain 的一致性。新版还直接设置 group `default_domain`，并用锁/引用计数阻止使用期间切换。运行 R1 已启用通用 IOMMU API、IOMMU 支持和 Rockchip IOMMU；但启用分支的公共头仅前向声明 group，无法支持 0.9.8 的直接字段访问。**已排除**将 SDK 0.9.8 RKNPU 原样替换进旧 R1 5.10 树的路径。已恢复的本地 R1 5.10 树含页面所列 `rockchip_linux_defconfig`、`rk3588_linux.config` 和 R1 DTS，但 RKNPU 仍为 0.8.2；它将作为板级配置参考而非问题修复版。官方 Rockchip BSP 的 `develop-5.10`、`develop-6.1`、`develop-6.6` 均已远程验证为 RKNPU 0.9.8，因此不因驱动版本选择高内核大版本；`develop-5.10` 当前精确提交已固定为 `bfa51d2ab08140d1309afc9a9fe0fc2878cee35a`，并成功以 22 MB partial/sparse clone 获取 RKNPU、RK3588 DTS 与配置目录。它已有与 R1 运行时 compatible 对应的 `rk3588s-evb4-lp4x-v10.dts`，但没有 Yyt 名称 DTS；已验证两者顶层正文相同，差异仅为官方 `rk3588s-evb4-lp4x.dtsi`/`rk3588-android.dtsi` 与 R1 `rk3588s-evb4-lp4x-yyt.dtsi`/`rk3588-linux.dtsi` 的 include 选择。唯一下一步是只核对这四份基础 include 在两棵树中的存在性，不构建或烧录。

补充：四份基础 include 的存在性已验证；只有 `rk3588s-evb4-lp4x-yyt.dtsi` 为本地 R1 厂商树独有，其他三份同名文件在两棵树均存在。唯一下一步改为只查看 Yyt 文件相对本地标准 EVB4 文件的 diff hunk 标题，以定位 R1 专有节点；不构建或烧录。

补充：该 Yyt 文件相对本地标准 EVB4 文件有 26 个 diff hunk，因此不能视为单点外设修补；但改动仍局限于这一份板级文件。唯一下一步改为只提取节点结构行与 `status` 变化，先按硬件类别建立差异地图；不构建或烧录。

补充：差异地图已显示 USB/PCIe/SATA、显示路由、I²C、PWM、CAN、UART、风扇与厂商 GPIO 改动，未出现直接 NPU 节点；还发现若干 `status = "disalbed"` 厂商拼写，效果待验证且不修改。唯一下一步改为只列出 eMMC/SD、GMAC、NPU 标签在 Yyt 文件中的出现位置，以核对当前启动、网口与 NPU 基础是否受专有差异影响；不构建或烧录。

补充：R1 与官方树的标准 EVB4、Linux、Android 三份同名基础 include 均不同，故不再尝试单独复制 Yyt 文件。迁移策略改为在独立工作树中带入完整 R1 DTS include 依赖包、使用新 DTB 名称进行纯编译验证；唯一下一步是先读取其第一层 include 清单，不复制、不构建、不烧录。

补充：R1 的 Yyt 文件直接使用 `rk3588s-evb-yyt.dtsi` 而非标准 `rk3588s-evb.dtsi`，并额外包含 OS04A10 相机与 Yyt LCD 描述；R1 属于替代性板级 DTS 组合。唯一下一步改为由 C 预处理器自动生成递归依赖清单，作为隔离移植的精确输入；不复制、不构建、不烧录。

补充：预处理器已把 R1 DTS 闭包收敛为 10 个 `.dts`/`.dtsi` 文件，`dt-bindings` 由内核 include 提供。唯一下一步改为创建固定官方 5.10 commit 的初始不检出 sparse Git worktree，后续 DTS 复制与编译仅在该隔离空间进行；不复制 DTS、不构建、不烧录。

补充：隔离工作树已在分支 `study/r1-dts-port` 建立且初始干净。唯一下一步改为逐个复制并字节核验 10 个 R1 DTS 输入；修改仅影响此可删除工作树，不构建、不烧录。

补充：10 个 R1 DTS 输入现已出现在隔离工作树：5 个同名基础文件修改、5 个 Yyt/R1 专有文件新增，集合与递归闭包相符；除最后一项外的逐文件 `cmp` 输出未保留。唯一下一步改为只读取 DTS Makefile 中的 RK3588S DTB 条目，确定新 R1 DTB 的最小构建入口；不修改、不构建、不烧录。

补充：隔离工作树 Makefile 已注册独立 `rk3588s-yyt.dtb` 目标，标准 EVB4 条目未被覆盖。Makefile 专项格式检查通过；全树检查仅提示 R1 导入文件末尾原有空行，暂为保持输入忠实而不改。唯一下一步改为核对当前 sparse checkout 是否含 DTB 构建依赖，避免盲目执行 `make`；不构建、不烧录。

补充：当前 sparse checkout 缺少 `scripts/`、`include/`，不能直接进行完整内核 DTB 构建；不扩展为完整下载。唯一下一步改为以 GCC 预处理器加 `dtc` 最小生成隔离 R1 DTB，DT bindings 临时取自本地 R1 基线 include；该结果仅验证 DTS 编译闭包，不表示内核可启动或可烧录。

补充：隔离生成 DTB 的 eMMC、GMAC、RKNPU 节点均存在，compatible 分别为 RK3588 DWCMSHC、RK3588 GMAC、RK3588 RKNPU，且 `status = "okay"`。R1 DTS 移植完成静态关键节点验证，但未证明启动或推理。唯一下一步改为确认空间后解除隔离 worktree 的 sparse checkout，准备完整官方 5.10 内核构建；会有较大主机网络下载，不构建、不烧录。

补充：隔离官方工作树现已完整展开，大小 1.4 GiB、sparse checkout 已禁用，且此前 R1 DTS 修改仍在该分支中。唯一下一步改为只检查官方 RK3588 Linux 配置输入及 RKNPU/eMMC/GMAC 关键符号，避免先构建出不满足 NPU 或启动前置的内核；不配置、不编译、不烧录。

补充：官方 ARM64 `rockchip_linux_defconfig` 已显式启用 RKNPU、STMMAC、MMC SDHCI/DW/Rockchip；三行 `rk3588_linux.config` 仅涉及 PCIe Broadcom Wi-Fi 与 Mali CSF，首个 NPU 启动验证暂不合入。唯一下一步改为在独立输出目录生成并读取 Kconfig 解析后的 `.config`，核对 DWMAC 及关键依赖的最终状态；不编译内核、不烧录。

补充：独立 Kconfig 输出已确认 DWMAC Rockchip、Realtek PHY、DRM、eMMC 和 RKNPU/DRM GEM 均为 `y`；配置不是当前阻塞。唯一下一步改为通过 Kbuild 仅构建 `rk3588s-yyt.dtb`，验证 DTS 移植通过真正内核流程；不构建 Image、不烧录。

补充：首次裸 DTB 目标解析遗漏 `rockchip/` 子目录，报“没有规则”，这不是 DTS 或配置失败。唯一下一步改为以完整 Rockchip 相对路径重试同一 DTB 目标；不改源码、不构建 Image、不烧录。

补充：完整路径重试显示 Kbuild 会自动添加 `arch/arm64/boot/dts/` 前缀，故该路径被重复拼接；正确目标是 `rockchip/rk3588s-yyt.dtb`。唯一下一步改为以这个相对目标第三次调用 Kbuild；尚未进入 DTS 编译，不构建 Image、不烧录。

补充：以 `rockchip/rk3588s-yyt.dtb` 为目标的官方 Kbuild 已成功生成 233,459 B 的 R1 DTB（SHA-256 `a76fbacb8ea37230be1dfb4d08762017b09cfa8a36516b0f668057f55268949d`）。其较手动 DTB 更大，原因待查；唯一下一步改为读取设备树符号相关配置、根节点与三个关键硬件节点，仍不构建 Image、不烧录。

补充：最小 DTS 编译已成功，隔离 R1 DTB 为 FDT v17、149,607 bytes、SHA-256 `dc3d5eafef03768c43a564a511caf6abf6a676ca063edfcd3042b981256b331d`。DTC 仅给出来源 alias 命名警告，未阻断生成；它不是启动或 NPU 成功证据。唯一下一步改为反编译并检查 eMMC、GMAC 和 RKNPU 三个关键节点是否仍在生成 DTB 中；不构建内核、不烧录。

补充：Kbuild 输出配置已确认 `CONFIG_DTC_SYMBOLS=y`，生成 DTB 根节点含 `__symbols__`，这解释其 233,459 B 大于手动 DTB 的主要元数据来源；`CONFIG_OF_OVERLAY` 未启用。Kbuild DTB 中 eMMC、GMAC、RKNPU 仍均为预期 compatible 与 `status = "okay"`。唯一下一步改为仅在隔离主机输出目录构建 `Image`，随后核对镜像身份及内建 RKNPU 0.9.8；不封装 FIT、不烧录。

补充：隔离官方 5.10 树已成功完成 `vmlinux` 链接、`System.map` 生成及 ARM64 `Image` 导出。唯一下一步改为只读取 `Image`/`vmlinux` 的身份、大小、哈希和 RKNPU 版本字符串；不封装 FIT、不烧录。

补充：`Image` 已确认为 37,675,520 B 的 ARM64 boot Image（SHA-256 `e39e443ccff8b670ece4caa3149a6c0272b34c723fca7fe5492e8fff918726d3`），`vmlinux` 为带 debug 信息的 AArch64 ELF；其中明确含 `RKNPU driver`、`0.9.8`、`20240828`，构建侧 NPU 版本目标已达成。唯一下一步改为只读识别当前板端 boot 载荷和主机 FIT 检查工具；不能直接写入裸 `Image`，不封装、不烧录。

补充：当前板端 `boot` 标签确定为 `/dev/mmcblk0p3`（64 MiB），首部为 1,536 B FDT/FIT 控制树，魔数为 `d0 0d fe ed`。这确认现有启动载荷是 FIT 结构，不能直接以裸 `Image` 覆盖。唯一下一步改为只确认主机 `mkimage`/`dumpimage` 工具是否可用；不提取、不修改、不烧录。

补充：主机当前缺少 `mkimage` 与 `dumpimage`；Arch `extra/uboot-tools` 的可用版本为 `2026.07-1`，下载约 236 KiB。唯一下一步改为安装这一主机检查依赖并确认命令版本；不读取或写入板子。

补充：主机 `mkimage` 与 `dumpimage` 均已确认为 2026.07。唯一下一步改为只读复制当前 R1 boot 分区到 `build/local/`，先核对长度和哈希；不解析、不写入板子。

补充：当前 boot 分区已只读导出为 `build/local/r1-20260816/r1-boot-p3.img`，长度精确 67,108,864 B，SHA-256 为 `e983740d4df29d51fa58dea9d504d536b87c8b205935ec2ceb8d64e679cd833b`。唯一下一步改为只用 `dumpimage -l` 列出此副本的 FIT 结构；不提取、不写入板子。

补充：FIT 默认 `conf` 引用 35,707,392 B kernel、147,826 B FDT 与 638,976 B resource，并声明 `sha256,rsa2048:dev` / PSS 签名；FDT 哈希与此前记录一致。新 Image 更大但尚不能从载荷大小推导可写入性或签名可重建性。唯一下一步改为只读查看 FIT 控制树的签名与外部数据属性；不提取、不写入板子。

补充：FIT 控制树已确认三项 image 均以 `data-position`/`data-size` 指向外置数据；`conf` 包含 `kernel`、`fdt`、`multi`、`rollback-index`，且有独立 `signature` 子节点。唯一下一步改为只读取位置/大小/签名文本属性；不提取、不写入板子。

补充：当前 FIT 的 fdt/kernel/resource 分别从 `0x800`/`0x24a00`/`0x2232400` 开始，签名用 `sha256,rsa2048`、PSS，覆盖 `fdt kernel multi`，密钥提示为 `dev`。以原 kernel 起点替换为新 Image 的纯容量计算仍余 29,283,328 B，故容量不是当前阻塞；签名验证策略仍未确认。唯一下一步改为只读检查签名 value 与 U-Boot 中的 `dev`/`required` 线索；不构建、不写入板子。

补充：boot FIT 的 signature 节点没有 `value`，故未携带实际签名数据；`Sign value: unavailable` 的原因已确认，但 U-Boot 内嵌公钥/required 策略仍未知。当前 GPT 再次确认 U-Boot 为 `/dev/mmcblk0p1`（4 MiB）。唯一下一步改为只读导出此分区并校验；不解析、不写入板子。

补充：当前 U-Boot 分区已只读导出为 `build/local/r1-20260816/r1-uboot-p1.img`，长度精确 4,194,304 B，SHA-256 为 `3b09148574d57f9b76f8afb064dc21af6df2819e0c5ccf4bce18e08f56820001`。唯一下一步改为只用 `dumpimage -l` 列出其早期 FIT 结构；不提取、不写入板子。

补充：当前 p1 为含 ATF、OP-TEE、U-Boot 与独立 7,829 B U-Boot DTB（image 5）的早期 FIT；它也声明 `sha256,rsa2048:dev` 但无可见签名 value。唯一下一步改为仅从本地主机副本提取 image 5 并核验；不写入板子。

补充：提取出的 `r1-uboot-control-fdt.dtb` 为有效 7,829 B FDT v17，SHA-256 `c07f4a4d713c2dde198a1c4fc7a980a98f5dc97665e3171dc7c319d7846dc381` 与 p1 FIT image 5 精确一致。唯一下一步改为只读列出其 `/signature` 公钥/required 策略线索；不修改、不写入板子。

补充：当前 U-Boot 控制 DTB 没有 `/signature`，故未发现 `key-dev` 公钥或 `required` 策略；结合 boot FIT signature 节点亦无实际 `value`，没有 p3 boot FIT 必须 RSA 验签的现有证据，但不能推断整个启动链均不验证。唯一下一步改为仅从 p3 本地副本提取 fdt/kernel/resource 并逐项校验哈希；不构建、不写入板子。

补充：p3 FIT 的 FDT 已提取并以 `abd1c6c3…fe87546` 成功核验；kernel/resource 提取失败源于把 `dumpimage -T` 误作子镜像类型而非整体 FIT 容器类型，未产生这两项文件。唯一下一步改为统一以 `-T flat_dt` 提取 image 1/2；不构建、不写入板子。

补充：p3 FIT 的 fdt、kernel、resource 三项现均已提取，并分别精确匹配 `abd1c6c3…fe87546`、`5e8fc7f4…49ceeb`、`492cbec9…6569c6`。唯一下一步改为先以三项旧载荷主机侧重建 FIT 克隆，验证 mkimage 布局与哈希；不替换 kernel、不写入板子。

补充：`mkimage -E -p 0x800 -B 0x200` 已用旧载荷成功生成可读 FIT 克隆，三项哈希保持原值，克隆总长 36,496,384 B，恰等于原外置数据终点。唯一下一步改为只读核对克隆 positions 与 `multi` 引用；不构建新内核 FIT、不写入板子。

补充：FIT 克隆的三个外置 position/size 与当前 p3 完全一致，`conf` 的 `kernel/fdt/multi` 引用也保持一致。主机侧旧载荷封装链已闭合；唯一下一步改为构造新 Image + 新 R1 DTB + 原 resource 的候选 FIT 并静态自检，不写入板子。

补充：RKNPU 0.9.8 候选 FIT 已生成且可由 dumpimage 解析；新 DTB、内核和原 resource 哈希均正确，文件 38,550,016 B，小于 p3 容量并余 28,558,848 B。唯一下一步改为只读核对其最终 layout/config/signature 节点；不写入板子。

补充：候选 FIT 的外置数据已确认连续对齐，末端 `0x24c3a00`（38,550,016 B）仍低于 p3 64 MiB；`conf` 引用与 rollback index 正确，signature 按设计缺失。唯一下一步改为进入 U-Boot 仅读取 TFTP/DHCP 支持和网络环境，确认 RAM 启动测试通道；不下载、不启动、不写入板子。

补充：U-Boot 提供 `tftpbootm [loadAddress] [[hostIPaddr:]bootfilename]` 和 `dhcp` 启动命令；三项网络环境变量均未定义。它们将来可用于非持久 RAM 测试，但当前先只读确认主机 `enp108s0` 地址和 TFTP 服务安装状态；不启动服务、不下载、不启动、不写入板子。

补充：RKNPU 0.9.8 的“无 Rockchip display DRM、无 Mali GPU”候选已通过 userdata→RAM 的 FIT 路径启动到 Linux `5.10.252`；没有写入 p1/p3、rootfs 或 U-Boot 环境。候选中 `/dev/dri/renderD128` 的 sysfs driver 为 `RKNPU`，OF compatible 为 `rockchip,rk3588-rknpu`。使用原有 DeepSeek-R1-Distill-Qwen-1.5B W8A8 `.rkllm` 模型输入 `ok` 已生成 `Alright,`；Prefill 20.37 tokens/s、Generate 7.99 tokens/s、峰值内存 1673.56 MB。候选板级 DTS 源码已保存为 `src/rockchip-linux-kernel-r1-dts-port` 的 `study/r1-dts-port` 分支提交 `799622bab`；`build/` 产物不纳入 Git。当前候选刻意不提供 Rockchip 显示 DRM 与 Mali GPU；它是 NPU LLM 的可复现验证载荷，不是可烧录的完整系统。PCIe link failure 与 PL330 `Bad Desc` 启动日志尚未分析，但没有阻止本次登录和推理。项目近期顺序已调整为：先验证 Linux+Zephyr AMP 的 CPU、内存、启动与最小 IPC 可行性，再集成 LLM 和实时外设；保持 eMMC `boot` 分区不变。首次运行时盘点已完成：全部 CPU 经 PSCI 管理，未发现现成 remoteproc/RPMsg、Zephyr carveout 或相关 DTS 名称节点；而 Rockchip BSP 中已发现 RK3588 AMP DTS/RPMsg 参考。详见[DEC-20260810-002](../decision/dec-20260810-002-linux-zephyr-amp-long-term-direction.md)、[EXP-20260817-001](../experiment/exp-20260817-001-inventory-r1-amp-runtime-prerequisites.md)、[ISSUE-20260815-002](../issue/issue-20260815-002-rkllm-w8a8-matmul-run-failed.md)与[EXP-20260815-002](../experiment/exp-20260815-002-probe-r1-npu-runtime-chain.md)。唯一下一步：主机只读阅读 `rk3588-amp.dtsi`，提取 CPU、内存、通信和外设资源划分；不改源码、不构建、不操作板端。

补充：学习者购买的 MT7922 已在候选系统枚举为 PCI `14c3:0616`，但 sysfs 显示 `unbound`；当前 Rockchip 5.10.252 源码没有 `mt7921` 驱动目录，候选配置也没有 `CONFIG_MT7921E`。该支线暂缓，不在 AMP 原型期间回移无线驱动；继续使用有线网络。见[EXP-20260819-001](../experiment/exp-20260819-001-probe-r1-mt7922-pcie.md)。唯一下一步回到 AMP 启动责任链：定位被排除 `cpu_l3` 的远端固件装载者与入口地址来源；不改 DTS、不调用 SMC、不写 eMMC。

## 唯一下一步

基于已验证的 Linux 5.10.252 候选制作一个**仅供静态检查**的 AMP DTS 变体：从 Linux CPU 拓扑移除 `cpu_l3`，同步移除 ARM PMU 对该核的 affinity，并为当前 Zephyr 链接区域 `0x50000000`–`0x50100000` 增加 1 MiB `no-map` 保留内存。只编译 DTB、反编译并核对 CPU 与内存节点；暂不调用 SiP SMC、不启动第二个 CPU、不写 eMMC。成功标准只是“Linux DTB 不再声明该 CPU 且不再分配 Zephyr 区域”，不是 AMP 已运行。
