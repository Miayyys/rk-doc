---
title: "EXP-20260902-002 LZAMP Rockchip develop-6.12 官方 EVB4 主机基线与 RAM-only booti 诊断"
type: experiment
status: verified
created: 2026-09-02
updated: 2026-09-04
tags: [rk3588, lzamp, linux, bsp, boot]
related:
  - "[[decision/dec-20260902-001-establish-lzamp-engineering-root]]"
  - "[[experiment/exp-20260822-001-build-r1-amp-shmem-ping]]"
  - "[[status/current]]"
---

# EXP-20260902-002 LZAMP Rockchip develop-6.12 官方 EVB4 主机基线与 RAM-only booti 诊断

## 目标

在 LZAMP 中固定 Rockchip 官方 `develop-6.12` 基线，确认主机构建和现有厂商 U-Boot 的直接 `Image`/DTB→`booti` RAM 启动路径；同时记录官方 EVB4 DTB 与 R1 板级差异对启动/eMMC 短读的影响。此实验不验证 R1 板级兼容性、AMP、MailMsg、RKLLM、无线或显示功能。

## 环境与前置条件

- 执行端：Arch 主机负责源码配置/构建；R1 板端使用现有厂商 U-Boot 载入 `Image` 和 DTB 并执行 `booti`。
- 硬件及版本：youyeetoo R1；板端目标为 Linux `6.12.69`，官方 EVB4 DTB 的硬件模型仍是 `EVB4`。
- 软件、镜像或 Git commit：Rockchip 官方 `develop-6.12`，commit `470f9dccbdc42e7b8a824d0a5c5640a10e9457d2`；使用 `rockchip_linux_defconfig`。Image SHA-256：`d65c9cc775f1c2ddd7c84c832740ea6f89b8d227032068a4a70fbc10ea9d570c`；官方 `rk3588s-evb4-lp4x-v10-linux.dtb` SHA-256：`aa1de6ce9289724019f4a197f19cb0349c25a50af80655be25613afc0ee1370c`。
- 连接方式：板端通过现有厂商 U-Boot 直接从 ext4 载入 `Image`/DTB 至 RAM，再执行 `booti`；未使用 FIT/resource 打包路径。
- 操作前状态：保留现有可恢复路径；本轮不写 eMMC boot 分区，不执行 `saveenv`。

## 风险与恢复

- 影响范围：仅 RAM-only 载入和启动候选；未改变 eMMC、启动分区或 U-Boot 环境。
- 备份：沿用既有 eMMC 可恢复备份；本实验不新增块设备写入。
- 恢复方法：重启回现有可恢复启动路径；不以本实验的 EVB4 DTB 作为 R1 持久化配置。

## 步骤与证据

### 步骤 1：构建 develop-6.12 官方 EVB4 主机基线

目的与预期结果：在锁定 commit 上用 `rockchip_linux_defconfig` 生成 ARM64 `Image` 和官方 EVB4 Linux DTB，并记录可核对哈希。

实际结果和退出码：主机构建成功；Image 与 DTB 的完整 SHA-256 分别为 `d65c9cc775f1c2ddd7c84c832740ea6f89b8d227032068a4a70fbc10ea9d570c`、`aa1de6ce9289724019f4a197f19cb0349c25a50af80655be25613afc0ee1370c`。配置侧确认 `AMP/RKNPU/RPMSG_ROCKCHIP_MBOX=y`，`MT7921E` 未启用；该配置事实来自主机基线，不等于板端运行验证。

### 步骤 2：官方 EVB4 DTB 直接 booti 与首次诊断

目的与预期结果：不经过 FIT/resource，验证现有 U-Boot 直接载入 `Image`/DTB 后能否进入 6.12，并观察官方 EVB4 DTB 的板端差异。

实际结果和退出码：直接 `booti` 进入 Linux `6.12.69`。首次官方 EVB4 DTB 在 `i2c8`（`feca0000`）地址 `0x22` 持续超时；源码/DTB 对照确认该设备为 FUSB302，旧 R1 YYT DTS 将其放在 `i2c6`，官方 6.12 EVB4 则放在 `i2c8`。禁用 `i2c8` 后出现 MMC CQE recovery 和 I/O read errors。DTB 对照还确认官方 6.12 含 `supports-cqe`，已验证的 R1 5.10 DTB 不含。上述故障现象是诊断证据，不把单一差异直接扩展为所有启动问题的根因。

### 步骤 3：禁用 i2c8 并移除 supports-cqe 的第二轮 RAM-only booti

目的与预期结果：在不写 eMMC 的前提下，用诊断 DTB 排除已观察到的 FUSB302 地址冲突和 CQE 差异，确认最小 EVB4 启动/eMMC 短读边界。

实际结果和退出码：诊断 DTB 同时禁用 `i2c8`、删除 `supports-cqe`；主会话记录其 SHA-256 为 `84277b…9b67`（仅保留此前缀/后缀，未记录完整哈希）。RAM-only `booti` 进入 Linux `6.12.69`，显示 8 核、model `EVB4`，root 为 `/dev/mmcblk0p6` 上的 ext4 且为 `rw`；读取约 38 MB 的 `Image` 校验一致，错误过滤无输出。该步骤未写 boot 分区、未固化 DTB、未执行 `saveenv`。

### 步骤 4：统一命名 LZAMP 板级 DTB 与 RAM-only booti 回归（已验证）

目的与预期结果：为 LZAMP 后续工程建立不与旧 5.10/YYT 名称混淆的 DTB 命名，确认该 DTS 在锁定的 6.12 源码中可以生成 DTB，并以 RAM-only `booti` 回归其最小启动路径。

实际结果和退出码：项目自有 DTS 统一命名为 `rk3588s-lzamp-linux.dts`，生成 DTB 为 `rk3588s-lzamp-linux.dtb`；根节点 `model = "LZAMP RK3588S"`，`compatible = "lzamp,rk3588s", "rockchip,rk3588"`。该 DTS 仍 include 官方 `rk3588s-evb4-lp4x.dtsi` 与 `rk3588-linux.dtsi`，并设置 `i2c8` 为 disabled、未包含 `supports-cqe`。主机构建 DTB 成功，SHA-256 为 `1ddfea41247dc88f80af6b25782bdceaae7cc8897ecdc25d046c4066113d8d95`；具体构建退出码未记录。随后该正式 `rk3588s-lzamp-linux.dtb` 已通过 RAM-only `booti` 进入系统，输出 kernel `6.12.69-g470f9dccbdc4`、`nproc=8`、model `LZAMP RK3588S`、compatible `lzamp,rk3588s rockchip,rk3588`。

启动阶段观察到 `fe180000.pcie` 和 `fe190000.pcie` 重复报告 `LTSSM=0x3`、`Link Fail`，最终 `failed to initialize host`；系统因此停顿约数秒后仍继续启动。源码查阅得到的定位候选为 `fe180=l1/domain3`、`fe190=l2/domain4`；旧已知 MT7922 BDF `0004:41:00.0` 指向 `fe190`，但本次 6.12 `lspci` 尚未验证。上述仅是现象和查阅候选，不构成 PCIe 根因或修复结论。本轮仍为 RAM-only，未写 eMMC 或保存 U-Boot 环境。

旧 Linux `5.10`/YYT DTS 名称只作为历史证据，不代表 LZAMP 新 DTS 命名或其运行结果。

### 步骤 5：PCIe domain 观察与待验证 DTS 修正候选（主机侧，待板测）

目的与预期结果：记录正式 LZAMP DTB 会话中的 PCI domain 枚举边界，并保存针对启动阶段 PCIe 现象的可审查 DTS 候选；本步骤不把主机构建或静态检查当作板端修复验证。

实际结果和退出码：在正式 LZAMP DTB 会话执行 `lspci -Dnn | grep -E '(^0003:|^0004:|14c3:0616)' || echo 'domain3/4-no-device'`，输出 `domain3/4-no-device`，即该会话的 PCI domain3/4 未枚举 endpoint。主机源码核查确认 `fe180=pcie2x1l1/domain3`、`fe190=pcie2x1l2/domain4`；历史已知 MT7922 BDF `0004:41:00.0` 指向 `fe190`，但本次 6.12 `lspci` 未枚举该设备。

主会话随后形成待验证 DTS 修正候选：`fe180` disabled；`fe190` 保持 okay，将 reset 改为 GPIO4 PA7 active-high，并显式加入 `vpcie3v3-supply`。新 DTB 主机构建和静态检查通过，大小 `267595` B、SHA-256 `a8e93651e10ff79272427f718c727eda89fe2bb02f66dc5d915f2731f435677d`；静态检查确认 `fe180` disabled、`fe190` okay、reset raw `131 7 0`、supply phandle `132`。该候选尚未上板，不能据此宣称 MT7922 恢复、PCIe 延迟消失、根因或修复成立。

### 步骤 6：PCIe 修正版 LZAMP DTB RAM-only 回归

目的与预期结果：使用步骤 5 的主机构建候选回归正式 LZAMP DTB 的启动与 PCIe endpoint 枚举，区分已观察现象和仍待验证的驱动/网络功能。

实际结果和退出码：第一次使用 PCIe 修正版 LZAMP DTB 时，在运行 `/sbin/init` 后串口静止；重启一次后进入系统，因此该静止只能记为单次、未复现现象，根因未知。成功启动中 `lspci` 显示 `0004:41:00.0 Network controller [14c3:0616]`，证明 MT7922 PCIe endpoint 已枚举。`fe190` 日志从 `LTSSM=0x3` 到 `Link up 0x130011`，链路为 PCIe Gen2 x1 并完成 endpoint 枚举；`fe180` 未出现在用户提供的筛选输出中，仅记录为当前输出观察，不能扩展为该控制器已完成链路验证。

成功启动期间还出现 `fixed dependency cycle`、`invalid prsnt-gpios`、`can't get current limit`、`IRQ ptm not found` 等非致命日志。当前仅验证 endpoint 枚举；驱动绑定、firmware 加载、网络功能、长期稳定性以及首次串口静止的根因仍未验证。本轮仍为 RAM-only，未写 eMMC 或保存 U-Boot 环境。

### 步骤 7：网络接口现状与 MT7921E 模块候选（待板测）

目的与预期结果：区分正式 LZAMP 6.12 当前网络接口缺失的 DTS/内核配置边界，并准备下一轮可通过 RAM-only 验证的 MT7921E 模块候选；不把接口缺失直接归因于硬件故障。

实际结果和退出码：在 PCIe endpoint 枚举成功的 6.12 系统执行 `ip -br addr`，仅有 `lo`，未出现 `eth` 或 `wlan` 接口。原因层面仅记录为：当前内核未加载或不含可用的 `mt7921e` 模块，且 LZAMP DTS 尚未移植目标板有线 `GMAC`；不能称硬件失败。

主机已新建 LZAMP config fragment，并将配置改为 `CONFIG_MT7921E=m`、`CONFIG_BCMDHD` disabled、`LOCALVERSION=-lzamp`、`AUTO` off；因源码工作树 dirty，实际 kernelrelease/vermagic 为 `6.12.69-lzamp+`。全 modules 构建曾因 Rockchip `bcmdhd` 缺少 proprietary `typedefs.h` 失败，随后禁用 `BCMDHD` 并定向构建 mt76 成功。新的 Image SHA-256 记录为 `928d67…`；`extract-ikconfig` 确认 `CFG80211=y`、`MAC80211=y`、`PCI=y`、`MT7921E=m`。五个模块和两个 MT7922 固件已整理到被忽略的 `LZAMP/build/mt7922-smoke/board`，尚未上板验证。

主机从本机 `linux-firmware` 压缩包解出但未提交、未上板的两个固件为：`WIFI_MT7922_patch_mcu_1_1_hdr.bin`，`137632` B，SHA-256 `7fc9075d6d54b31e0539734614ca971fa52caf41b642a3000c8a4501591a4d98`；`WIFI_RAM_CODE_MT7922_1.bin`，`1231252` B，SHA-256 `d100e5e04c324fc8da6302112d1e0b4fac952286447d2419eabcd8fb0a1f5b93`。内核源码支持以 `firmware_class.path` 指定固件路径；该候选 Image 与固件尚未上板验证，本步骤未写 eMMC 或保存 U-Boot 环境。

### 步骤 8：MT7922 模块与固件 RAM-only smoke 回归（部分验证）

目的与预期结果：在不写 eMMC 的前提下，使用步骤 7 准备的模块与固件，验证 MT7922 驱动绑定、固件加载和无线接口创建；不把接口创建等同于扫描、关联或网络可用。

实际结果和退出码：从旧 5.10 系统经 `/userdata` 传包，使用 raw Image + LZAMP DTB 执行 RAM-only `booti`，运行内核为 `6.12.69-lzamp+`；设置 `firmware_class.path=/userdata/lzamp/mt7922-smoke/firmware`。按依赖加载 5 个模块后，`lspci` 显示 `Kernel driver in use mt7921e`，sysfs driver 为 `/sys/bus/pci/drivers/mt7921e`，并出现 `phy0`、接口 `wlP4p65s0` 和 `p2p0`。日志显示 ASIC revision `79220010`，WM firmware 成功加载。

同期 `regulatory.db` 报错 `-2` 尚未解决；无线关联尚未验证，有线 `GMAC` 仍未移植，不能称 Wi-Fi 或网络可用。本轮仍为 RAM-only，未写 eMMC 或保存 U-Boot 环境。

在同一会话中，`wlP4p65s0` 与已 UP 的 `p2p0` 使用同一 MAC；启用 `wlP4p65s0` 时 mac80211 返回 `ENOTUNIQ`。关闭 `wlP4p65s0`、改用 NetworkManager 认可的 `p2p0` 后，`nmcli` 扫描返回 `bss_count=1`、`scan_exit=0`。NetworkManager 状态为 `p2p0 wifi disconnected`、`wlP4p65s0 wifi unavailable`、`p2p-dev-p2p0 unmanaged`。这只验证了基本射频扫描代表路径；关联、DHCP、吞吐、稳定性仍未验证，`regulatory.db` 错误仍未解决。

### 步骤 9：RKNPU/IOMMU RAM-only 初始化观察（部分验证）

目的与预期结果：在不写 eMMC 的前提下，确认当前 6.12 LZAMP Image 的 RKNPU 驱动初始化、IOMMU 归属、DRM 节点映射和代表性 RKLLM 短 smoke；不把一次短 smoke 扩展为长稳、性能或产品验证。

实际结果和退出码：使用新候选 raw Image + LZAMP DTB 执行 RAM-only `booti`，进入 `6.12.69-lzamp+`，`nproc=8`，model 为 `LZAMP RK3588S`。候选 Image SHA-256 为 `928d67e9d819d46404892e8f9aba1ddf60df02561794d5356de464eab4406cf8`，DTB SHA-256 为 `a8e93651e10ff79272427f718c727eda89fe2bb02f66dc5d915f2731f435677d`，板端/主机哈希一致。

启动后 `fdab0000.npu` 加入 IOMMU group `15` 并处于 IOMMU mode；初始化阶段输出三段 MMIO request `-EBUSY`，随后 DRM 初始化 `rknpu 0.9.8`、注册 minor `1`，debugfs version 为 `v0.9.8`。节点映射为 `renderD128=rockchip-drm`、`renderD129=RKNPU`。在 PCI domain4 枚举后、`pcieport 0004:40:00.0: PME: Signaling with IRQ 115` 附近体感停顿；该现象的根因待查，不将停顿写成已知因果。

同一 `6.12.69-lzamp+` 会话中，RKLLM Runtime `1.3.0`、driver `0.9.8`、Qwen3.5-0.8B W8A8（target `rk3588`）初始化成功，Enabled CPUs 为 `[4,5,6,7]`、count `4`；`chat-smoke` 返回 `{"ok":true,"executed":false,"response":"READY"}`、exit `0`。随后筛选 `dmesg` 未见 IOMMU fault、SError 或 job timeout，仅见启动期三段 MMIO `-EBUSY` 及 `Initialized`。源码审计将该 `-EBUSY` 解释为 IOMMU 子窗口与 NPU 大窗口重叠时驱动回退普通 `ioremap`；这是源码审计结论，不表示完全无风险。此次仅验证代表性短文本 smoke，本轮为 RAM-only，未写 eMMC 或保存 U-Boot 环境；不覆盖长稳、性能、DVFS、AMP 或 MailMsg。

### 步骤 10：6.12 LZAMP NPU+AMP+MailMsg RAM-only 组合代表路径（已验证）

目的与预期结果：在 6.12 LZAMP DTB 中验证 CPU3 carveout、MailMsg mailbox0 四通道、Zephyr 启动器、四优先级基本回归和一次 NPU+AMP+MailMsg native tool 组合闭环；不扩展为 stop/rearm、压力或长期验证。

实际结果和退出码：Arch 主机正常构建通过；Image SHA-256 为 `68dd367729b7082da8f6e1ad862dc2108928c975b956b1f3802772d62aaa8d41`，DTB SHA-256 为 `573734159a7bc8a8d13eaa6160ff4a33f6cc3cad578df885e8594d626e38e5b8`。DTB 静态检查确认不存在 `cpu@300`，`zephyr@50000000` 为 `1 MiB`、`no-map`，入口 `0x5000100c`，image slot `49152`，`mailbox0` 为 okay 且包含四个 channel；正常构建和 LZAMP 15 项主机测试通过。

板端使用该候选 RAM-only `booti` 进入 `6.12.69-lzamp+`，`nproc=7`，`cpu@300` 缺失、reserved 区存在，mailbox controller 已绑定，launcher sysfs 存在，初始 affinity 为 `off`。加载冻结 normal Zephyr（SHA-256 仅记录为 `29dcd57a…`）后 `CPU_ON ret=0`，affinity 转为 `on`，MailMsg 为 `active session=1/1`，worker valid，并收到 `SESSION_READY`。加载前 observation 区的随机值均为 `valid=0`，不作为消息或故障证据。

在 active session `1/1` 下，四优先级基本回归均通过：p0 `100→ACK/PONG 101`，p1 `200→ACK/PONG 201`，p2 得 `PONG 301`，p3 得 `PONG 401`；各级 `tx=1`、`full=0`、`depth=0`、notify 成功，`rx` 分别为 `3/2/1/1`，`incomplete/crc/invalid/stale` 全为 `0`，worker `irq/wake/drain/msg=4`，pending `0`。

随后以 Qwen3.5 target `rk3588-r7`（CPU3–6）完成 native tool 闭环：decision `zephyr_increment(41)`，p1 `window=1`，Agent exit `0`、result `42`，transport 为 ACK seq `8`/peer `5`、PONG seq `9`；终态 active session `1/1`、CPU3 on、pending `0`、worker `msg=5`。dmesg 筛选未见新增 IOMMU fault、SError 或 RKNPU timeout，仅显示已记录的启动期 `-EBUSY`。

本次 6.12 API 适配事实为：cache API 改用 `dcache_clean/inval_poc(start,end)`；driver 采用 built-in，并抑制 manual bind attrs 以适应 remove 语义。上述支持 6.12 NPU+AMP+MailMsg 首次组合代表路径；controlled stop/rearm、压力、长稳、性能、持久化仍待验证，未写 eMMC 或保存 U-Boot 环境。

### 步骤 11：6.12 MailMsg 受控停止与 rearm 第二会话回归（已验证）

目的与预期结果：在 active session 上执行 `mailmsg_stop`，确认 Linux 可观察 Zephyr 停止结果、CPU3 affinity 和 MailMsg offline 状态，并验证显式 rearm 后可建立第二会话；不扩展为压力或持久化验证。

实际结果和退出码：板端执行 `mailmsg_stop` 后 `stop_exit=0`；`affinity_state` 为 `mpidr=0x300 level=0 state=off (1)`，status 显示 `mailmsg_state=offline`、`stop_sequence=1`、`stop_reply=6`、`stop_result=0`。随后执行 rearm、重新写入 controlled-stop Zephyr 并 `start`；p0 value `900` 返回 ACK `type=3 seq2 peer1 status0` 与 PONG `type=2 seq3 value901`，`client_exit=0`，affinity `on`、`mailmsg_state=active`、session `2/2`、`session_result=0`。第二会话随后再次执行 `mailmsg_stop`，得到 `affinity_state` `mpidr=0x300 level=0 state=off (1)`、`mailmsg_state=offline`、`stop_reply=6`、`stop_result=0`。

该结果支持两次受控停止及一次 rearm/第二会话 active 的生命周期代表回归；压力、长期生命周期、持久化及其他恢复场景仍待验证。

### 步骤 12：6.12 LZAMP canonical source 可复现主机构建

目的与预期结果：在锁定的 Rockchip `develop-6.12` commit 上，从干净 worktree 重放 LZAMP 补丁和构建流程，确认 DTS、Kconfig/Makefile 接入及主机测试可复现；不把新构建 Image 当作已上板验证。

实际结果和退出码：现有 patch `0001` 已更新为与已验证的 95 行 DTS 一致；新增 patch `0002` 接入 Kconfig/Makefile 和两个 canonical-source wrappers。新增 prepare 脚本检查锁定 commit `470f9dccbdc42e7b8a824d0a5c5640a10e9457d2`、拒绝 dirty tree 并应用补丁。在从该 commit 创建的干净 worktree 中，prepare 成功，`cmp` 确认 DTS 一致；完整 Image、DTB 和 modules 构建成功，LZAMP 两个驱动对象成功编译。配置阶段补传 `CROSS_COMPILE` 后，在全新输出目录无交互完成，关键配置为 `MAILBOX=y`、`ROCKCHIP_MBOX=y`、`LZAMP_AMP_MAILMSG=y`、`MT7921E=m`；LZAMP 15 项主机测试通过。主会话未提供各步骤的单独退出码。DTB SHA-256 仍为 `573734159a7bc8a8d13eaa6160ff4a33f6cc3cad578df885e8594d626e38e5b8`；新构建 Image 的 SHA-256 为 `890c9b36259654065427b334bd1d076fc6a2c19e92ebdcb9bcdf3cf627b1b4e7`；步骤 12 记录当时该 Image 尚未上板，后续板端核验见步骤 13。

观察：本步骤验证的是锁定源码、补丁、prepare 和主机构建链的可重复性。它不改变步骤 10–11 的既有板端证据；步骤 12 当时未证明新 Image 的 RAM-only 启动或功能兼容性，后续步骤 13 已补充同一 Image 的板端核验。

### 步骤 13：6.12 LZAMP GMAC1 以太网 RAM-only 回归（已验证）

目的与预期结果：定位初始 6.12 LZAMP 系统中 GMAC1 未形成网络接口的板级资源冲突，使用保留旧 YYT 已验证参数的 canonical DTS 恢复 GMAC1，并以 RAM-only 启动验证链路和 IPv4 SSH；不扩展为所有板级外设功能。

实际结果和退出码：初始 6.12 LZAMP 会话中 runtime 的 GMAC 状态为 `okay`，但日志显示 GPIO3_A5 被 `febb0000.serial` 占用，导致 GMAC pinctrl probe 失败。源码核对确认 `febb0000` 为 UART8；EVB4 启用 UART8 M1，其 A2/A3/A5 与 GMAC1 RGMII 冲突，而旧 YYT DTS 未启用 UART8。canonical DTS 随后加入旧 YYT 已验证的 GMAC1 参数：`rgmii-rxid`、clock input、`MDIO phy@0`、TX `0x44`/RX `0x18` 和 GPIO3 PB7 reset，并禁用 EVB4 `wireless_bluetooth` 与 UART8。新 DTB SHA-256 为 `47d866bf9c49aa81fab16884903c6ea0e19fe09334a658a7b49bd33760eec75b`。

U-Boot 明确加载 `/userdata/lzamp/linux-6.12-repro/Image`，板端核对 SHA-256 为 `890c9b36259654065427b334bd1d076fc6a2c19e92ebdcb9bcdf3cf627b1b4e7`，与步骤 12 新构建 Image 一致。板端 Linux `6.12.69-lzamp+` RAM-only 回归中，`/sys` driver 为 `rk_gmac-dwmac`，carrier=`1`、speed=`1000`；日志显示 `RTL8211F stmmac-1:00` 与 `Link Up 1Gbps Full flow rx/tx`，IPv4 SSH 实际可用。另观察到 GPIO3-19 已被 GMAC 占用，造成 `febf0030.pwm` pinctrl 冲突；该冲突未影响本次以太网验证，暂不判断其板级功能或根因。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| develop-6.12 主机构建 | Image/官方 EVB4 DTB 可生成并可校验 | `rockchip_linux_defconfig` 构建成功；Image/DTB SHA-256 已记录 | 通过（主机基线） |
| 直接 Image/DTB→booti | 无需 FIT/resource 即可进入 Linux | 官方 EVB4 DTB 直接进入 Linux `6.12.69` | 通过（启动路径） |
| 官方 EVB4 初始 DTB | 观察板级设备差异 | `i2c8` FUSB302 地址 `0x22` 持续超时；禁用后出现 CQE/I/O 错误 | 失败（诊断分支） |
| 诊断 DTB 的 eMMC 短读 | Linux/rootfs 可用且短读无观察错误 | 8 核 EVB4、`/dev/mmcblk0p6` ext4 `rw`，约 38 MB Image 校验一致、错误过滤无输出 | 通过（短读范围） |
| LZAMP 正式命名 DTS/DTB | 新名称与板级身份可固定并完成最小 RAM-only 启动 | `rk3588s-lzamp-linux.dts/.dtb`，kernel `6.12.69-g470f9dccbdc4`、8 核、model/compatible 与 DTB SHA-256 已核对；启动阶段观察到两路 PCIe Link Fail，系统仍继续启动 | 通过（RAM-only booti；PCIe Link Fail 待定位） |
| PCIe 启动阶段 | 记录正式 LZAMP DTB 的现象，不提前归因 | `fe180000.pcie`、`fe190000.pcie` 重复 `LTSSM=0x3`/`Link Fail`，最终 `failed to initialize host`；约数秒停顿后继续启动 | 已观察；根因/修复未确认 |
| PCI domain 枚举 | 记录正式 DTB 会话的 endpoint 发现结果 | `lspci` 输出 `domain3/4-no-device`；domain3/4 未枚举 endpoint，历史 BDF `0004:41:00.0` 本次未验证 | 已观察；不等于 PCIe 根因 |
| PCIe 修正候选主机构建 | 主机构建/静态检查通过后进入板测队列 | `fe180` disabled；`fe190` okay，reset raw `131 7 0`、supply phandle `132`；DTB `267595` B，SHA-256 `a8e93651e10ff79272427f718c727eda89fe2bb02f66dc5d915f2731f435677d` | 通过（主机侧） |
| PCIe 修正版 DTB 回归 | RAM-only 启动并观察 endpoint 链路 | 重启一次后进入系统；`fe190` 从 `LTSSM=0x3` 到 `Link up 0x130011`、Gen2 x1，`0004:41:00.0` endpoint 枚举 | 通过（单次代表路径；驱动/网络未验证） |
| MT7922 驱动与网络功能 | endpoint 可绑定驱动并完成基本网络功能 | `mt7921e` 已绑定，出现 `phy0`、`wlP4p65s0`、`p2p0`；基本射频扫描已在关闭冲突接口后得到 1 个 BSS，关联/网络功能仍未验证 | 部分通过 |
| 6.12 网络接口现状 | endpoint 枚举后出现可用网络接口 | `ip -br addr` 仅有 `lo`；当前内核未加载或不含可用 `mt7921e` 模块，LZAMP DTS 尚未移植目标板有线 `GMAC` | 已观察；不等于硬件失败 |
| MT7921E 模块主机侧候选 | 构建包含模块及依赖的可核对 Image | `CONFIG_MT7921E=m`、`CONFIG_BCMDHD` disabled、`CFG80211/MAC80211/PCI=y`、kernelrelease/vermagic `6.12.69-lzamp+`；Image SHA-256 `928d67…`；5 个模块和 2 个固件已整理，完整哈希见步骤 7 | 通过（主机构建；尚未上板） |
| MT7922 驱动与固件 RAM-only 回归 | 模块绑定、firmware 加载并创建无线接口 | `firmware_class.path=/userdata/lzamp/mt7922-smoke/firmware`；`mt7921e` in use，sysfs driver、`phy0`、`wlP4p65s0`、`p2p0` 均出现；ASIC `79220010`、WM firmware 成功 | 通过（代表路径） |
| regulatory.db 与无线业务 | 无线区域数据库和基本业务路径可用 | `regulatory.db` 报错 `-2` 尚未解决；关闭同 MAC 的 `wlP4p65s0` 后使用 `p2p0` 扫描得到 `bss_count=1`、`scan_exit=0`；关联、DHCP、吞吐、稳定性未验证 | 部分通过（仅基本扫描） |
| RKNPU 6.12 驱动初始化 | RKNPU 加入 IOMMU 并完成 0.9.8 DRM 初始化，节点映射可核对 | `fdab0000.npu` 加入 IOMMU group `15`/IOMMU mode；三段 MMIO request `-EBUSY` 后仍初始化 `rknpu 0.9.8`、DRM minor `1`、debugfs `v0.9.8`；`renderD128=rockchip-drm`、`renderD129=RKNPU` | 部分通过（不等于 RKLLM 推理） |
| RKNPU/RKLLM 代表性短 smoke | 当前 RKNPU 初始化后完成代表性 RKLLM 短文本 smoke | Runtime `1.3.0`、driver `0.9.8`、Qwen3.5-0.8B W8A8 target `rk3588` 初始化成功；Enabled CPUs `[4,5,6,7]`/count `4`；`chat-smoke` 返回 `ok=true/executed=false/response=READY`、exit `0`；筛选 dmesg 未见 IOMMU fault、SError 或 job timeout | 通过（短 smoke；不覆盖长稳/性能/DVFS/AMP/MailMsg） |
| 6.12 LZAMP AMP 候选主机构建与静态布局 | AMP DTB/Zephyr 槽位和 mailbox0 四通道可构建并静态核对 | Image/DTB SHA-256 已记录；无 `cpu@300`，`zephyr@50000000` 为 `1 MiB` `no-map`、entry `0x5000100c`，slot `49152`，mailbox0 okay/四 channel；正常构建和 15 项主机测试通过 | 通过（主机/静态范围） |
| 6.12 LZAMP AMP 候选启动与 SESSION_READY | RAM-only booti 后可启动 CPU3 并完成一次 MailMsg session 握手 | Linux `6.12.69-lzamp+`、7 核、cpu300 absent/reserved present、mailbox 绑定、launcher sysfs 存在；Zephyr `CPU_ON ret=0`、affinity on、active `session=1/1`、worker valid、收到 `SESSION_READY`；加载前 observation 均 `valid=0` | 通过（启动/握手代表路径） |
| 6.12 四优先级与 NPU+AMP+MailMsg native tool 组合 | 四优先级基本请求及一次 Qwen3.5 native tool 应闭环 | p0/p1 ACK+PONG `101/201`、p2/p3 PONG `301/401`；各级 tx/full/depth 与错误计数符合记录；Qwen3.5 `zephyr_increment(41)` 经 p1/window1 得 result `42`，ACK `8/5`、PONG `9`、exit `0`；终态 active/session `1/1`、CPU3 on、pending `0`、worker msg `5`，无新增 IOMMU fault/SError/RKNPU timeout | 通过（首次组合代表路径；不覆盖 stop/rearm、压力/长稳/性能/持久化） |
| 6.12 MailMsg 受控停止与 rearm 第二会话 | `mailmsg_stop` 成功进入 offline，rearm 后可建立第二会话并完成 p0 回归 | 第一次 STOP `stop_exit=0`；affinity `mpidr=0x300 level=0 state=off (1)`；status `mailmsg_state=offline`、`stop_sequence=1`、`stop_reply=6`、`stop_result=0`；rearm/重新加载/`start` 后 p0 `900` 得 ACK `seq2 peer1`、PONG `seq3 value901`、exit `0`，affinity on、active、session `2/2`、session_result `0`；第二次 STOP 同样得到 affinity off、`mailmsg_state=offline`、`stop_reply=6`、`stop_result=0` | 通过（两次受控停止与一次 rearm 代表回归；不覆盖压力/长期/持久化） |
| 6.12 LZAMP canonical source 可复现主机构建 | 干净 worktree 可重放 prepare、补丁、构建与测试 | 锁定 commit `470f9dccbdc42e7b8a824d0a5c5640a10e9457d2` 的 prepare 成功，DTS `cmp` 一致，Image/DTB/modules 构建成功，LZAMP 两个驱动对象成功编译，关键配置 `MAILBOX=y`、`ROCKCHIP_MBOX=y`、`LZAMP_AMP_MAILMSG=y`、`MT7921E=m`，15 项主机测试通过；DTB SHA-256 `573734159a7bc8a8d13eaa6160ff4a33f6cc3cad578df885e8594d626e38e5b8`，新 Image SHA-256 `890c9b36259654065427b334bd1d076fc6a2c19e92ebdcb9bcdf3cf627b1b4e7`；步骤 12 记录当时尚未上板，步骤 13 已用同一 SHA 完成板端回归 | 通过（主机构建可复现，且后续同一 Image 已完成 GMAC1 板端验证） |
| 6.12 LZAMP GMAC1 以太网 RAM-only 回归 | GMAC1 可绑定、链路建立并提供 IPv4 SSH | 修正 DTS DTB SHA-256 `47d866bf9c49aa81fab16884903c6ea0e19fe09334a658a7b49bd33760eec75b`；`rk_gmac-dwmac`、carrier=`1`、speed=`1000`，RTL8211F 1Gbps Full 链路，IPv4 SSH 可用；GPIO3-19 的 PWM pinctrl 冲突另行观察 | 通过（以太网代表路径；不覆盖其他板级功能） |
| R1 板级兼容 | R1 专用功能与外设保持正确 | 正式 LZAMP DTB 已完成 RAM-only 启动，但 R1 完整兼容性、其余 AMP/MailMsg 生命周期、RKLLM、无线、显示仍未验证 | 未确定 |

## 结论

已验证：锁定的 Rockchip `develop-6.12` 可在主机用 `rockchip_linux_defconfig` 构建 Image/官方 EVB4 DTB；现有厂商 U-Boot 可直接 ext4load 后 `booti`，无需 FIT/resource 打包即可进入 Linux `6.12.69`。在禁用官方 EVB4 `i2c8` FUSB302 节点并移除 `supports-cqe` 的诊断 DTB 上，完成了 EVB4 model 的 RAM-only 启动和 eMMC 约 38 MB 短读。

本结论包括正式 LZAMP DTB 的一次 RAM-only 启动，以及官方 EVB4/诊断 DTB 的启动与 eMMC 短读路径；它们均不等于 R1 完整板级兼容性。RKNPU 0.9.8 驱动初始化、IOMMU 归属、DRM 节点映射和一次 Qwen3.5 代表性短文本 smoke 已观察验证，但不等于长稳、性能或产品验证。6.12 LZAMP AMP 候选已完成一次四优先级、NPU+AMP+MailMsg 组合及两次受控停止、一次 rearm/第二会话 active 代表回归；步骤12当时该 Image 尚未上板，随后步骤13已用同一 SHA 完成板端回归。步骤 13 已验证 GMAC1 1Gbps 链路及 IPv4 SSH。压力、长期运行、持久化和更广泛的 R1 兼容性仍未验证。无线关联和显示也仍未验证。

正式 LZAMP DTB `rk3588s-lzamp-linux.dts/.dtb` 已完成一次 RAM-only `booti` 回归：kernel `6.12.69-g470f9dccbdc4`、8 核、model `LZAMP RK3588S`、compatible `lzamp,rk3588s`/`rockchip,rk3588` 均与预期一致。此前正式 DTB 会话观察到两路 PCIe Link Fail 和 domain3/4 无 endpoint；步骤 5 的 PCIe 修正版候选在重启后的成功启动中使 `fe190` 完成 Gen2 x1 链路并枚举 MT7922 `0004:41:00.0`，但一次串口静止仍为未复现现象，不能据此宣称 PCIe 根因或修复已确认。`fe180=l1/domain3`、`fe190=l2/domain4` 仍是源码查阅候选；6.12 `fe180` 链路状态、无线关联和完整网络功能仍未验证。RKNPU 0.9.8 驱动初始化、`renderD128`/`renderD129` 映射和一次 Qwen3.5 代表性短文本 smoke 已观察验证，但不覆盖 RKLLM 长稳/性能/DVFS；PCI domain4 枚举后 `PME` 附近停顿的根因待查。该 PCIe 回归不覆盖 R1 完整兼容性、以太网、显示、长期运行或 eMMC 固化；6.12 AMP+MailMsg 组合及两次受控停止、一次 rearm/第二会话 active 回归见步骤 10–11，旧 5.10/YYT 名称只作为历史证据。
在 endpoint 枚举成功的 6.12 系统中，正式 Image 的 `ip -br addr` 仅显示 `lo`；原因层面仅记录为当时内核未加载或不含可用 `mt7921e` 模块，以及 LZAMP DTS 尚未移植目标板有线 `GMAC`，不构成硬件失败结论。随后使用 `CONFIG_MT7921E=m` 候选完成一次 RAM-only smoke：`mt7921e` 绑定并加载 WM firmware，创建 `phy0`、`wlP4p65s0` 和 `p2p0`；在关闭同 MAC 的 `wlP4p65s0` 后，使用 `p2p0` 完成一次返回 1 个 BSS 的基本扫描。`regulatory.db`、关联、DHCP、吞吐和稳定性仍未验证。步骤 13 已在修正 DTS 上验证目标板 GMAC1 的 1Gbps 链路与 IPv4 SSH；这不扩展为完整 R1 网络或板级兼容性结论。

## 关联知识与问题

- 支持或修正的知识点：[DEC-20260902-001](../decision/dec-20260902-001-establish-lzamp-engineering-root.md) 的 6.12 主机基线与板测边界。
- 关联问题：暂无新增问题记录；`i2c8`/CQE 现象仅作为本实验的诊断证据。

## 后续行动

- [ ] 唯一优先的下一步：在不改变 eMMC/U-Boot 可恢复路径的前提下，继续处理无线 `regulatory.db` 错误并验证关联；步骤 13 已用步骤 12 的 `890c9b36259654065427b334bd1d076fc6a2c19e92ebdcb9bcdf3cf627b1b4e7` Image 完成 GMAC1 RAM-only 回归，压力、长稳、持久化仍不在本实验已验证范围。
