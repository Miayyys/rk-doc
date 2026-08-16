---
title: "EXP-20260812-004 定位上游 GPIO 驱动源码的有效 ref"
type: experiment
status: active
created: 2026-08-12
updated: 2026-08-12
tags: [rk3588, r1, linux, gpio, source]
related:
  - "[[experiment/exp-20260812-001-locate-running-kernel-config]]"
  - "[[experiment/exp-20260812-003-check-running-kernel-source-availability]]"
  - "[[note/gpio-controller-and-pinctrl]]"
  - "[[note/device-tree-platform-driver-binding]]"
  - "[[status/current]]"
---

# EXP-20260812-004 定位上游 GPIO 驱动源码的有效 ref

## 目标

验证 Linux stable 官方仓库中可用于阅读 `drivers/gpio/gpio-rockchip.c` 的有效分支或标签。目标只是找到可追溯的上游学习样本，不声称它等同 R1 厂商 `5.10.110` 内核源码。

## 环境与前置条件

- 执行端：Arch 主机 fish Shell。
- R1 当前运行内核为 `5.10.110`，但镜像没有本机 modules、headers 或源码；见[EXP-20260812-003](exp-20260812-003-check-running-kernel-source-availability.md)。
- 运行配置确认 GPIO 核心与 Rockchip GPIO 控制器均内建；见[EXP-20260812-001](exp-20260812-001-locate-running-kernel-config.md)。

## 风险与恢复

- 影响范围：只对官方 Git 服务器发起 HTTP/Git 元数据请求。
- 备份：不需要。
- 恢复方法：不下载、修改或访问开发板，无恢复操作。

## 步骤与证据

### 步骤 1：尝试直接访问推测的 raw-source URL

目的：测试以下“stable 仓库 + `v5.10.110` ref + raw 文件路径”组合是否可用。预期可能为成功或 HTTP 错误；HTTP 错误应保留以避免重复使用同一未验证 URL。

```fish
set source_url 'https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/drivers/gpio/gpio-rockchip.c?h=v5.10.110'
curl --fail --location --head $source_url
```

实际输出（学习者提供；未显示其后的 fish 退出码）：

```text
HTTP/2 404
server: nginx
date: Wed, 12 Aug 2026 14:18:08 GMT
content-type: text/html; charset=UTF-8
vary: Accept-Encoding
expires: Thu, 01 Jan 1970 00:00:05 GMT
last-modified: Wed, 12 Aug 2026 14:18:08 GMT
vary: Accept-Encoding

curl: (22) The requested URL returned error: 404
```

观察：主机已经到达 `git.kernel.org` 并获得 HTTP 响应，但该 URL 组合返回 404。证据只支持“此路径/ref 组合当前不可用”；尚不能区分是仓库路径、raw 接口参数、ref 名称，还是服务端展示方式不匹配。不能把 404 解读为 Linux 源码不存在，也不应改用未经核对的第三方镜像。

### 步骤 2：查询候选维护分支与精确标签

目的：不再根据 URL 猜测 ref，而是直接由官方 Git 远端列出候选分支和标签。预期每条成功查询可能输出一行 `<commit> <ref>`；无输出且退出码为 0 表示远端可访问但未列出该匹配 ref。传输错误不能解释为 ref 不存在。

```fish
set stable_repo https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
git ls-remote --heads $stable_repo linux-5.10.y
printf 'branch-query exit=%s\n' $status
git ls-remote --tags $stable_repo v5.10.110
printf 'tag-query exit=%s\n' $status
```

实际输出（学习者提供）：

```text
acccef89f184a697fee1c96a1dc9cccbba36937b	refs/heads/linux-5.10.y
branch-query exit=0
fatal: unable to access 'https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/': TLS connect error: error:0A000126:SSL routines::unexpected eof while reading
tag-query exit=128
```

观察：第一条成功，确认 `linux-5.10.y` 是当前远端可见的维护分支，查询时返回 commit `acccef89f184a697fee1c96a1dc9cccbba36937b`。第二条在 TLS 连接读取期间中断，未得到标签列表；退出码 128 说明 Git 传输失败，而不说明 `v5.10.110` 标签存在或不存在。由于同一远端前一请求刚成功，**推测**这次是瞬时连接/服务器端传输中断；先保持查询参数不变进行一次重试，避免同时改变 transport、URL 和 ref。

### 步骤 3：保存浏览器取得的主线阅读样本并检查绑定线索

目的：检查学习者通过浏览器保存的 `gpio-rockchip.c` 是否为可读 C 源文件，并确认其中存在与 R1 DTS `rockchip,gpio-bank` 和平台驱动模型相关的最小线索。它仅用于理解通用驱动结构，不作为当前 R1 厂商内核 `5.10.110` 的源码证据。

执行端：Arch 主机；Agent 在本仓库内只读检查。学习者说明该文件来自 Linux 主线 `7.1`，但尚未保存下载 URL、commit 或浏览器页面信息。

```sh
stat -c 'path=%n; type=%F; size=%s bytes' build/local/r1-20260812/gpio-rockchip.c
sha256sum build/local/r1-20260812/gpio-rockchip.c
rg -n -m 5 'rockchip,gpio-bank|platform_driver|MODULE_DESCRIPTION' build/local/r1-20260812/gpio-rockchip.c
```

实际输出（本机检查）：

```text
path=build/local/r1-20260812/gpio-rockchip.c; type=regular file; size=21276 bytes
0e8b196719891df0b9b56e886d568cb83d5bb0744294956884aa085ae8b7f181  build/local/r1-20260812/gpio-rockchip.c
800: { .compatible = "rockchip,gpio-bank", },
805: static struct platform_driver rockchip_gpio_driver = {
826: MODULE_DESCRIPTION("Rockchip gpio driver");
```

观察：该文件是 21276 字节的普通文本文件，包含 R1 FIT DTS 中出现的 `rockchip,gpio-bank` compatible 字符串，以及 Rockchip GPIO `platform_driver` 注册定义。这足以作为“设备树 compatible 与 platform driver 的 match table 相连”的首次代码阅读样本。源文件不会在内容中自带完整 Linux release 标识；`7.1` 来自学习者对浏览器下载页的说明，故其精确 URL、commit 和版本仍是**用户提供**，尚未独立核验。保存位置是可重生成且被 Git 忽略的 `build/local/`，符合本仓库分析产物规则。

### 步骤 4：阅读 match table 与 platform driver 结构

目的：从源码中确认 `compatible` 字符串、设备树匹配表、`platform_driver` 与 `probe` 回调之间的静态连接，而不把“匹配”误解为“驱动已完成初始化”。

执行端：Arch 主机 fish Shell。学习者读取并提供了以下片段；只读本机分析文件，不访问开发板或修改文件。

```c
static const struct of_device_id rockchip_gpio_match[] = {
	{ .compatible = "rockchip,gpio-bank", },
	{ .compatible = "rockchip,rk3188-gpio-bank0" },
	{ },
};

static struct platform_driver rockchip_gpio_driver = {
	.probe		= rockchip_gpio_probe,
	.remove		= rockchip_gpio_remove,
	.driver		= {
		.name	= "rockchip-gpio",
		.of_match_table = rockchip_gpio_match,
	},
};
```

观察：`rockchip_gpio_match` 是设备树匹配表，其中第一项的 `rockchip,gpio-bank` 与 R1 FIT DTS 中 5 个 SoC GPIO bank 节点的 compatible 文本一致。`rockchip_gpio_driver.driver.of_match_table` 明确引用该表，`.probe` 指向 `rockchip_gpio_probe`，`.remove` 指向拆除路径 `rockchip_gpio_remove`。

学习者的解释为：“DTS 配置设备属性；`.of_match_table` 把驱动和 DTS 节点匹配起来；不清楚 probe。”其中前两点与代码和先前 R1 DTS 证据相符。补充结论：匹配表只说明该驱动**可以处理**一个 compatible 相符的设备树节点；当 platform bus 完成匹配后，内核才调用 `.probe`，让驱动读取节点、准备硬件资源并注册运行时接口。实际 R1 厂商内核的 probe 实现版本尚未读取，不能从主线 7.1 文件断言其逐行相同。

### 步骤 5：解释 probe 开头建立的对象关系

目的：区分 `probe` 的局部变量声明与实际硬件操作，并将 platform device、设备树节点、父 pinctrl 和 Rockchip GPIO bank 关联到已知 R1 DTS 路径。

执行端：Arch 主机 fish Shell；学习者读取 `rockchip_gpio_probe()` 开头并提供变量声明。随后根据代码与 R1 已保存 DTS 完成解释；没有访问开发板或修改文件。

```c
struct device *dev = &pdev->dev;
struct device_node *np = dev->of_node;
struct device_node *pctlnp = of_get_parent(np);
struct pinctrl_dev *pctldev = NULL;
struct rockchip_pin_bank *bank = NULL;
struct rockchip_pin_deferred *cfg;
static int gpio;
int id, ret;
```

观察：这段尚未读写 GPIO 寄存器，而是在收集 probe 需要的对象与临时变量。`pdev` 是 platform bus 传入的设备对象；`pdev->dev` 是其通用 device 部分；`dev->of_node` 给出对应 DTS 节点；`of_get_parent(np)` 取得该节点的父节点。在 R1 保存的 DTS 中，GPIO 节点路径形如 `/pinctrl/gpio@fec20000`，因此该父节点是 pinctrl 节点。随后 `pctldev` 将保存内核中对应的 pinctrl 对象，`bank` 将指向 Rockchip 驱动内部表示一个 GPIO bank 的数据。`cfg`、`gpio`、`id` 与 `ret` 是后续延迟配置、备用编号、bank 标识和返回值处理所用变量。

学习者反馈“明白了”，说明已能把 `pdev → DTS 节点 → 父 pinctrl → GPIO bank` 作为一条对象关系理解。这个结论只说明概念解释已完成；本实验尚未阅读实际资源申请和注册 GPIO controller 的后续代码。

### 步骤 6：预测永久配置错误与延迟 probe 的区别

目的：从 `probe` 的第一个保护检查，区分设备树结构缺失和依赖驱动尚未就绪两种不同状态。

执行端：Arch 主机 fish Shell；学习者读取并提供以下代码开头。只读本机样本。

```c
if (!np || !pctlnp)
	return -ENODEV;

pctldev = of_pinctrl_get(pctlnp);
```

学习者判断：父节点不存在应失败，因为 pinctrl 负责同一物理引脚的复用和电气特性；R1 DTS 中 GPIO 节点位于 pinctrl 之下。若 pinctrl 没准备好，应等待后再继续，而非直接不用。

观察：这个判断与代码意图相符。`!np || !pctlnp` 是设备树结构不满足该驱动前提，函数立即返回 `-ENODEV`（没有可用设备/节点），而非继续进行错误初始化。`of_pinctrl_get(pctlnp)` 则开始按父 DTS 节点取得相应的运行时 pinctrl 对象；这一行本身不是错误返回，也不是 GPIO 寄存器操作。紧随其后的条件分支将决定“pinctrl 对象尚不可取得”时的处理方式，仍需由学习者读取确认。

补充讨论：学习者提出 `device_node` 的引用管理是因为 API 传递节点结构体指针、而非复制一份节点信息；这样也比复制节点高效。该理解正确。更准确地说，引用计数首先保证同一共享节点对象的生命周期和并发安全；避免复制节点属性、父子关系和关联数据，同时保持同一节点身份，是这种设计的效率与一致性收益。

### 步骤 7：确认延迟 probe 与 GPIO alias 编号入口

目的：验证 pinctrl 运行时对象不可得时是否采用延迟 probe，并识别 GPIO bank 编号优先来自哪里。

执行端：Arch 主机 fish Shell；学习者读取并解释以下代码。只读本机样本。

```c
pctldev = of_pinctrl_get(pctlnp);
of_node_put(pctlnp);
if (!pctldev)
	return -EPROBE_DEFER;

id = of_alias_get_id(np, "gpio");
if (id < 0)
	id = gpio++;
```

学习者解释：`of_pinctrl_get()` 取得设备；未取得则等待后再取得。

观察：理解正确，且可更精确为：该函数从父 DTS 节点找出其对应、已经注册的运行时 pinctrl 对象 `pctldev`；若当前没有该对象，驱动返回 `-EPROBE_DEFER`，请求内核在依赖就绪后重新调用 probe，而非在本函数中阻塞等待。随后 `of_alias_get_id(np, "gpio")` 从当前 GPIO 的 DTS 节点查询 `gpio` alias 的编号；R1 的 FIT DTS 已验证存在 `gpio0` 至 `gpio4` aliases。若查询失败，才使用静态计数器 `gpio++` 分配备用编号。该备用编号并非 R1 排针编号，也不是通用稳定规则。

### 步骤 8：阅读 bank 关联、锁与硬件资源获取入口

目的：区分 GPIO bank 与当前 platform device/DTS 节点的关联、并发保护与可失败操作的返回值，避免将 `slock` 误认为时钟。

执行端：Arch 主机；学习者读取 `probe` 中下列代码并提问。Agent 随后只读 `rockchip_get_bank_data()` 定义确认其职责。

```c
bank = rockchip_gpio_find_bank(pctldev, id);
if (!bank)
	return -EINVAL;

bank->dev = dev;
bank->of_node = np;

raw_spin_lock_init(&bank->slock);

ret = rockchip_get_bank_data(bank);
if (ret)
	return ret;
```

学习者判断该段“主要还是和设备关联起来”。观察：这抓住了中间两行的核心，但整段还有三个职责。

- `rockchip_gpio_find_bank(pctldev, id)` 从 pinctrl 对象中查找具体 bank；无对应 bank 返回 `-EINVAL`，说明当前 ID 与该 pinctrl 的 bank 数据不相容。
- `bank->dev = dev` 与 `bank->of_node = np` 将 bank 关联到当前的运行时 device 与 DTS 节点，供后续资源获取和 `dev_err()` 等日志使用。
- `slock` 是 `raw_spinlock_t` 自旋锁，不是 clock；它用于保护可能被并发/中断上下文访问的 bank 内部临界区。初始化锁本身不访问 GPIO 硬件。
- `ret` 是惯用的整型返回码变量。`rockchip_get_bank_data(bank)` 成功通常返回 0，失败返回负 errno；`if (ret) return ret;` 原样向上传播错误。

读取该函数定义确认，它依次从 `bank->of_node` 取 MMIO `reg` 资源、映射寄存器、解析 IRQ、取得并启用时钟、读取 GPIO 版本寄存器并选择 v1/v2 寄存器布局。因而这次调用才是 probe 开始接触实际硬件资源的入口；其细节尚未逐行学习。

### 步骤 9：由 pinctrl 私有数据查找具体 GPIO bank

目的：确认 DTS alias 的数值 `id` 如何在运行时 pinctrl 对象中定位为完整的 `rockchip_pin_bank`，并区分通用 device、pinctrl driver 私有数据和具体 bank。

执行端：Arch 主机 fish Shell；学习者读取并提供 `rockchip_gpio_find_bank()`。只读本机样本。

```c
info = pinctrl_dev_get_drvdata(pctldev);
bank = info->ctrl->pin_banks;
for (i = 0; i < info->ctrl->nr_banks; i++, bank++) {
	if (bank->bank_num == id) {
		found = 1;
		break;
	}
}

return found ? bank : NULL;
```

学习者提出“bank 信息在 dev 里”。观察：方向接近，但须区分对象层次。`pctldev` 是运行时 pinctrl 对象；`pinctrl_dev_get_drvdata(pctldev)` 取的是 pinctrl 驱动此前挂在该对象上的私有 `struct rockchip_pinctrl *info`，不是通用 `struct device` 的公开字段。`info->ctrl->pin_banks` 是 Rockchip pinctrl 驱动维护的 bank 数组，循环以 `bank->bank_num == id` 查找匹配项。成功时返回数组内该 bank 的指针，失败时返回 `NULL`。因此 `id` 只是查找键；完整 bank 才包含后续所需的节点、资源和状态。

### 步骤 10：读取 IRQ 和时钟资源

目的：区分“从 DTS 取得硬件描述”“转换为 Linux 运行时对象”和“实际启用时钟”三个阶段。

执行端：Arch 主机；学习者阅读并提供如下代码。只读本机样本。

```c
bank->irq = irq_of_parse_and_map(bank->of_node, 0);
if (!bank->irq)
	return -EINVAL;

bank->clk = of_clk_get(bank->of_node, 0);
if (IS_ERR(bank->clk))
	return PTR_ERR(bank->clk);
```

学习者判断：代码要取得中断和时钟，细节尚不清楚。

观察：判断正确。`irq_of_parse_and_map(..., 0)` 读取当前节点第 0 项 `interrupts` 描述，并经中断控制器/IRQ domain 将硬件中断标识转换为 Linux 使用的 IRQ 编号，保存到 `bank->irq`；返回 0 表示不能取得有效映射，所以返回 `-EINVAL`。它尚未注册实际中断处理函数。

`of_clk_get(..., 0)` 则读取当前节点第 0 项 `clocks` 描述，取得一个 `struct clk *` 时钟句柄保存到 `bank->clk`；它尚未启用时钟。该 API 可能返回错误编码指针（ERR_PTR）而不是空指针，因此使用 `IS_ERR()` 检查，再用 `PTR_ERR()` 还原原始负 errno 向上返回。紧接着的 `clk_prepare_enable(bank->clk)` 才会实际准备并启用该时钟。R1 已保存的 GPIO bank DTS 节点同时含 `interrupts` 与 `clocks` 属性，和此通用初始化顺序相容。

### 步骤 11：启用时钟并按硬件版本选择寄存器布局

目的：理解为什么 MMIO 读取前要启用外设时钟，为什么同一 Rockchip GPIO 驱动需要按硬件版本选择不同寄存器偏移表，以及失败后的资源回滚。

执行端：Arch 主机；学习者阅读并提供该函数的版本判断分支。只读本机样本。

```c
clk_prepare_enable(bank->clk);
id = readl(bank->reg_base + gpio_regs_v2.version_id);

switch (id) {
case GPIO_TYPE_V2:
	bank->gpio_regs = &gpio_regs_v2;
	bank->db_clk = of_clk_get(bank->of_node, 1);
	if (IS_ERR(bank->db_clk)) {
		clk_disable_unprepare(bank->clk);
		return -EINVAL;
	}
	break;
case GPIO_TYPE_V1:
	bank->gpio_regs = &gpio_regs_v1;
	break;
}
```

学习者推测：两个版本的功能不一样，V2 还带消抖时钟。

观察：推测方向正确，代码能直接支持的结论是：V1/V2 至少寄存器布局不同，因此驱动选择不同偏移表；不能仅凭此段概括所有功能差异。主时钟必须在 `readl()` 前启用，避免对被时钟门控的外设寄存器进行无效或不可靠访问。V2 分支从 DTS 第 1 项 `clocks` 取得 `db_clk`，即硬件 debounce（消抖）逻辑所需的时钟句柄；它不是驱动自动生成的内部时钟。若获取失败，代码禁用已开启的主时钟并返回错误，避免资源遗留。未知版本 ID 则返回 `-ENODEV`。

### 步骤 12：在 mutex 保护下注册 GPIO controller

目的：理解 `rockchip_gpiolib_register()` 前的锁为什么是 mutex，而不是本段先前出现的 raw spinlock，并确认失败时的回滚顺序。

执行端：Arch 主机；学习者读取并提供如下 probe 片段。只读本机样本。

```c
mutex_lock(&bank->deferred_lock);

ret = rockchip_gpiolib_register(bank);
if (ret) {
	clk_disable_unprepare(bank->clk);
	mutex_unlock(&bank->deferred_lock);
	return ret;
}
```

学习者判断：先上锁再注册，可能防止被打断。

观察：先上锁再注册的判断正确；但这把 `mutex` 的目的不是防硬中断。它保护的是 `bank->deferred_lock` 所对应的延迟引脚配置状态；源码紧邻注释说明它避免“恰好此时有延迟输出设置被加入”而与注册/后续处理竞争。mutex 可睡眠，通常用于进程/可睡眠上下文的互斥；硬中断或不可睡眠的临界区不能直接使用它，那类路径才需要 `raw_spinlock_t slock` 等自旋锁。

`rockchip_gpiolib_register(bank)` 成功时会把 bank 注册给 GPIO 子系统；失败时 `ret` 非 0，代码按已建立的资源顺序回滚：禁用此前启用的主时钟、释放 mutex、原样返回错误。这确保失败不留下锁或主时钟占用。

### 步骤 13：配置并注册 gpio_chip

目的：确认硬件资源已经准备好后，哪一步让 GPIO core 能把该 bank 当作 controller 管理，并区分此前的 device/DTS 关联与此处的 GPIO 子系统注册。

执行端：Arch 主机；学习者读取并提供 `rockchip_gpiolib_register()` 前半段。只读本机样本。

```c
bank->gpio_chip = rockchip_gpiolib_chip;

gc = &bank->gpio_chip;
gc->base = bank->pin_base;
gc->ngpio = bank->nr_pins;
gc->label = bank->name;
gc->parent = bank->dev;

ret = gpiochip_add_data(gc, bank);
```

学习者判断：现在才把内核和 bank 关联起来。

观察：接近，但应分两次关联看。probe 前面 `bank->dev = dev`、`bank->of_node = np` 已经把 bank 关联到当前 platform device 与 DTS 节点；本段进一步把 bank 封装并注册为 GPIO core 的 `gpio_chip` controller。`bank->gpio_chip = rockchip_gpiolib_chip` 复制此驱动的通用 GPIO 回调模板，`base`、`ngpio`、`label`、`parent` 分别设置 legacy 基址、线数、名称与父 device。`gpiochip_add_data(gc, bank)` 是正式向 GPIO core 注册 controller 的调用，并把该 controller 的私有驱动数据指向 `bank`。这一步成功后，GPIO core 才拥有把该 controller 暴露为运行时 GPIO 接口（包括本板已观察到的 gpiochip character device）的必要信息；具体字符设备编号仍由 GPIO core 运行时分配，不能从 `gc->base` 直接推出。

### 步骤 14：确认回调为何需要私有 bank 数据

目的：验证 `gpiochip_add_data(gc, bank)` 的第二个参数如何使通用 GPIO 回调回到具体 Rockchip GPIO bank，避免把 `gpio_chip` 自身误认为已包含所有硬件寄存器信息。

执行端：Arch 主机；学习者提出“为什么要把 bank 作为私有数据传入”，Agent 只读 `.set`/`.get` 回调确认。无板端操作。

```c
static int rockchip_gpio_set(struct gpio_chip *gc, unsigned int offset,
			     int value)
{
	struct rockchip_pin_bank *bank = gpiochip_get_data(gc);

	rockchip_gpio_writel_bit(bank, offset, value, bank->gpio_regs->port_dr);
}

static int rockchip_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
	struct rockchip_pin_bank *bank = gpiochip_get_data(gc);

	data = readl(bank->reg_base + bank->gpio_regs->ext_port);
}
```

观察：GPIO core 调用 `.set`/`.get` 等回调时传入的是通用 `struct gpio_chip *gc` 和线偏移 `offset`，不知道具体的 Rockchip bank 寄存器、时钟、锁或版本布局。`gpiochip_add_data(gc, bank)` 在注册时把 `bank` 绑定为这个 controller 的私有数据；`gpiochip_get_data(gc)` 在每次回调中将它取回。这样同一组回调可服务多个 GPIO bank，但每次都操作其各自的 `reg_base`、`gpio_regs` 和 `slock`。私有数据是“通用 GPIO API → 特定硬件上下文”的桥，而不是重复保存一份 controller。

### 步骤 15：阅读 GPIO controller 操作路由表

目的：识别 GPIO core 收到不同类型请求时会分派到哪些 Rockchip 回调，区分“注册 controller”和“实际执行一个 GPIO 操作”。

执行端：Arch 主机；学习者读取并提供 `rockchip_gpiolib_chip` 模板。只读本机样本。

```c
static const struct gpio_chip rockchip_gpiolib_chip = {
	.request = gpiochip_generic_request,
	.free = gpiochip_generic_free,
	.set = rockchip_gpio_set,
	.get = rockchip_gpio_get,
	.get_direction = rockchip_gpio_get_direction,
	.direction_input = rockchip_gpio_direction_input,
	.direction_output = rockchip_gpio_direction_output,
	.set_config = rockchip_gpio_set_config,
	.to_irq = rockchip_gpio_to_irq,
	.owner = THIS_MODULE,
};
```

观察：此结构体是 GPIO controller 的操作路由表，不会在定义时读写硬件。GPIO core 在某条线被请求、释放、读值、写值、查询/设置方向、应用配置或转为 IRQ 时，分别通过对应函数指针调用 Rockchip 驱动；这些函数再通过私有 `bank` 操作特定硬件。`.owner = THIS_MODULE` 表达该回调实现所属模块/内核单元的所有权关系。学习者已阅读此表；下一步选取 `direction_output` 一条路径，观察“设置为输出并给定初始值”如何避免错误电平。

## 结果对照

| 检查项 | 预期 | 实际 | 判定 |
| --- | --- | --- | --- |
| 推测的 raw-source URL | 返回源码或明确 HTTP 错误 | HTTP 404 | 该 URL 组合不可用，需验证远端 refs |
| `linux-5.10.y` 维护分支 | 输出该分支的 commit/ref | `acccef89…` / `refs/heads/linux-5.10.y`，退出码 0 | 已确认存在 |
| `v5.10.110` 精确标签 | 输出标签或空结果 | TLS EOF，退出码 128 | 传输失败，标签状态未知 |
| 浏览器保存的主线 GPIO 文件 | 普通 C 源文件，含绑定线索 | 21276 B，SHA-256 `0e8b…f181`，含 `rockchip,gpio-bank` 和 `platform_driver` | 可作通用阅读样本 |
| match table 与 driver 回调关系 | 代码中有明确引用 | `of_match_table = rockchip_gpio_match`、`.probe = rockchip_gpio_probe` | 已读并能解释静态连接 |
| probe 开头的对象关系 | 能区分对象取得与硬件操作 | `pdev → dev → of_node → parent pinctrl → bank` | 已读并能解释 |
| 配置缺失与依赖未就绪 | 能预测两类状态的不同处理 | 对 `-ENODEV` 与延迟初始化的角色判断正确 | 已理解，待读后续代码验证 |
| pinctrl 延迟与 GPIO alias | 能解释实际返回和编号入口 | `!pctldev → -EPROBE_DEFER`；优先查询 `gpio` alias | 已读并能解释 |
| bank 关联、锁与返回值 | 能区分关联、并发保护和返回码 | `dev`/`of_node` 关联；`slock` 为自旋锁；`ret` 传播错误 | 已读并能解释 |
| ID 到 GPIO bank 的查找 | 能区分私有数据与通用 device | `pctldev → drvdata → pin_banks[] → bank_num == id` | 已读并能解释 |
| IRQ 与时钟句柄获取 | 能区分映射/获取与实际启用 | IRQ domain 映射、`struct clk *`、ERR_PTR 检查 | 已读并能解释 |
| 时钟启用与 GPIO 版本选择 | 能解释主时钟、寄存器布局、消抖时钟与回滚 | V1/V2 偏移表；V2 第 1 项 clocks；失败回滚主时钟 | 已读并能解释 |
| GPIO controller 注册前的锁与回滚 | 能区分 mutex 与硬中断保护 | deferred 配置互斥；失败时关时钟、解锁、返回 | 已读并能解释 |
| gpio_chip 配置与注册 | 能区分 device/DTS 关联与 GPIO core 注册 | `gpiochip_add_data(gc, bank)` 注册 controller 并传入私有 bank | 已读并能解释 |
| GPIO 回调的私有 bank 数据 | 能解释注册时传入 bank 的目的 | `gpiochip_get_data(gc)` 取回 bank 并访问对应资源 | 已读并能解释 |
| GPIO 操作路由表 | 能将 GPIO core 请求对应到 Rockchip 回调 | `.set`/`.get`/方向/配置/IRQ 回调已定位 | 已读 |

## 结论

`linux-5.10.y` 已确认是有效的官方维护分支；精确标签 `v5.10.110` 的状态仍未知，因为标签查询遇到传输失败。学习者另保存了一份自述来自主线 7.1 的 `gpio-rockchip.c`；其内容已确认包含目标 compatible 和 platform driver。学习者已理解 probe 的对象关系、资源取得、时钟启用、V1/V2 寄存器布局选择和 V2 消抖时钟依赖，也理解 GPIO controller 注册前 mutex 对延迟配置状态的保护和失败回滚。现在已确认 `gpiochip_add_data(gc, bank)` 把具体 bank 注册给 GPIO core，理解私有 bank 数据如何使通用回调回到对应硬件资源，并已定位“GPIO core 请求 → Rockchip 回调”的路由表；这是与 R1 已观察 `/dev/gpiochipN` 的通用连接点。该样本版本距 R1 的 5.10.110 较远，不能用来解释厂商内核的逐行行为。下一步跟踪 `direction_output`，理解设置初始电平与切换输出方向的顺序。

## 关联知识与问题

- 支持的知识点：[GPIO 控制器、pinctrl 与运行时 gpiochip](../note/gpio-controller-and-pinctrl.md)。
- 关联问题：无。

## 后续行动

- [x] 保留第一次 raw-source URL 的 404 输出。
- [x] 确认官方维护分支 `linux-5.10.y` 存在。
- [x] 保存并核对主线 GPIO 驱动阅读样本的格式、哈希及最小绑定线索。
- [x] 阅读样本的 match table 与 `platform_driver` 定义，建立 DTS compatible、匹配表和 `.probe` 回调的基本关系。
- [x] 读取 `rockchip_gpio_probe()` 开头，识别 platform device、DTS 节点、父 pinctrl 与 GPIO bank 的关系。
- [x] 读取第一个保护检查和 pinctrl 获取，区分 DTS 结构缺失与依赖未就绪。
- [x] 读取 pinctrl 获取失败时的返回值，确认 `!pctldev` 使用 `-EPROBE_DEFER`，并识别 GPIO alias 编号入口。
- [x] 理解 bank 与当前 device/DTS 的关联、自旋锁、返回码传播，并定位硬件资源获取入口。
- [x] 读取由 GPIO ID 查找 Rockchip bank 的代码，区分 pinctrl driver 私有数据与通用 device。
- [x] 读取 `rockchip_get_bank_data()` 的 MMIO、IRQ 和时钟资源获取步骤。
- [x] 读取时钟启用、版本寄存器读取与 GPIO v1/v2 寄存器布局选择。
- [x] 阅读 GPIO controller 注册前 mutex 的用途与失败回滚。
- [x] 读取 `rockchip_gpiolib_register()` 的 `gpio_chip` 字段和 GPIO core 注册调用。
- [x] 验证私有 bank 数据如何被 `.set`/`.get` 回调取回并访问具体硬件资源。
- [x] 阅读 `rockchip_gpiolib_chip` 回调模板，建立 GPIO 操作到驱动函数的路由表。
- [ ] 跟踪 `direction_output` 的设置初始电平与输出方向顺序；精确 `v5.10.110` 标签查询暂不阻塞此概念学习。
