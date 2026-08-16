---
title: "设备树节点如何绑定到 platform driver"
type: note
status: verified
created: 2026-08-13
updated: 2026-08-13
tags: [rk3588, r1, linux, devicetree, platform-driver, gpio]
aliases: [OF match, probe, platform bus]
related:
  - "[[experiment/exp-20260812-004-locate-upstream-gpio-source-ref]]"
  - "[[experiment/exp-20260812-002-select-first-simple-device]]"
  - "[[note/device-tree-model-and-compatible]]"
  - "[[note/gpio-controller-and-pinctrl]]"
---

# 设备树节点如何绑定到 platform driver

## 学习目标

区分设备树的硬件描述、匹配资格与驱动初始化；能解释 R1 的 `rockchip,gpio-bank` 为什么会进入 Rockchip GPIO 驱动，而不把匹配本身误认为硬件已经准备完成。

## 核心概念

设备树（DTS/DTB）是数据，不是驱动执行代码。内核依据节点创建设备对象；对没有可枚举总线地址的片上外设，常见对象是 platform device。节点的 `compatible` 表达硬件兼容性，地址、时钟、中断和引脚等属性则供驱动初始化时读取。

驱动用 `struct of_device_id` 的 `.compatible` 项声明它支持什么硬件。该表被放到 `struct platform_driver.driver.of_match_table` 后，platform bus 才能比较设备树节点与驱动。匹配成功只表示“该驱动能处理此设备”；内核随后调用 `.probe`，由驱动申请资源、设置硬件并注册内核接口。设备移除时可调用 `.remove` 清理。

```text
DTS 节点 → platform device → of_match_table 比较 compatible
       → 匹配成功 → probe 初始化 → 运行时接口（例如 gpiochip）
```

probe 接收的 `struct platform_device *pdev` 不是设备树文本本身，而是内核创建的设备对象。驱动常经 `pdev->dev` 取得通用 `struct device`，再用 `dev->of_node` 取得关联的设备树节点。R1 GPIO 节点位于 `/pinctrl/gpio@...` 之下，所以 `of_get_parent(np)` 可获得父 pinctrl 节点；GPIO bank 处理 GPIO 电平/方向/中断，而 pinctrl 处理物理引脚的复用和电气配置。获取这些对象只是 probe 的准备阶段，不等于已读写 GPIO 或已经注册 gpiochip。

若 `of_node` 或其父 pinctrl 节点缺失，样本驱动返回 `-ENODEV`，因为该设备树结构无法满足驱动前提。此类结果不是系统崩溃，只表示该设备的 probe 无法继续。它和延迟 probe 不同：当节点存在但依赖的 pinctrl provider 尚未注册时，驱动可返回 `-EPROBE_DEFER`，请求内核在相关依赖准备后再次尝试 probe。后者适合初始化顺序问题，不能拿来掩盖 DTS 缺节点等配置错误。

本样本以 `of_pinctrl_get(pctlnp)` 取得运行时 pinctrl 对象；若结果为空，代码返回 `-EPROBE_DEFER`，不在该函数中睡眠等待。取得 pinctrl 后，它以 `of_alias_get_id(np, "gpio")` 优先读取当前 DTS 节点的 `gpio` alias 编号。R1 保存的 DTS 已有 `gpio0` 至 `gpio4` aliases；只有 alias 缺失时才使用驱动内的递增备用编号。这个编号用于驱动查找对应 bank，不等同于扩展排针位置。

查到 `struct rockchip_pin_bank *bank` 后，样本保存 `bank->dev` 与 `bank->of_node`，把具体 bank 关联到当前运行时 device 和 DTS 节点。它初始化的 `bank->slock` 是 raw spinlock（自旋锁），用于并发/中断上下文的临界区保护，和 `bank->clk` 这样的时钟对象无关。惯用变量 `ret` 保存函数返回码：0 通常成功，负值为 errno；驱动用 `if (ret) return ret;` 将资源准备失败向上层报告。

具体 bank 并不直接放在通用 `struct device` 的公开字段中。样本以 `pinctrl_dev_get_drvdata(pctldev)` 取得 Rockchip pinctrl 驱动附加给运行时 pinctrl 对象的私有 `struct rockchip_pinctrl`，再从其 `pin_banks` 数组中比较 `bank_num` 与 DTS alias 得到的 `id`。这说明：`id` 只是查找键；`pctldev` 提供所属 pinctrl 上下文；返回的 `bank` 才携带该 bank 的完整驱动状态与资源入口。

资源准备继续从 `bank->of_node` 读取 DTS 的 `reg`、`interrupts` 和 `clocks`。MMIO 的 `reg` 被解析为 resource 并映射到 `bank->reg_base`；`irq_of_parse_and_map()` 把第 0 项中断描述转换为 Linux IRQ 编号 `bank->irq`，但不在此时安装中断处理函数；`of_clk_get()` 取得第 0 项时钟的 `struct clk *` 句柄 `bank->clk`，但也尚未启用时钟。`of_clk_get()` 失败采用 ERR_PTR 表示，所以以 `IS_ERR()`/`PTR_ERR()` 处理，不能直接与空指针逻辑混用。

样本在首次 `readl(bank->reg_base + ...)` 前调用 `clk_prepare_enable(bank->clk)`，先使 GPIO 外设主时钟可用。随后按硬件版本 ID 选择 V1 或 V2 的寄存器偏移表。V2 分支从第 1 项 `clocks` 取得 `db_clk`，这是硬件消抖逻辑所需的时钟句柄，不是驱动凭空生成的时钟；若该项获取失败，会禁用已启用的主时钟后返回错误。

在调用 `rockchip_gpiolib_register(bank)` 前，样本以 `mutex_lock(&bank->deferred_lock)` 保护延迟引脚配置列表，避免其他可睡眠上下文恰在注册和后续应用延迟配置时并发修改它。该 mutex 不用于硬中断保护；同一 bank 的 `raw_spinlock_t slock` 才面向不可睡眠/中断相关临界区。若 GPIO controller 注册失败，代码禁用先前开启的主时钟、释放 mutex，再返回原始错误码。

`bank->dev`/`bank->of_node` 的关联不等于 GPIO 子系统注册。样本先将 GPIO 操作回调模板复制到 `bank->gpio_chip`，再设置线数、名称、父 device 等 controller 元数据，最后以 `gpiochip_add_data(gc, bank)` 向 GPIO core 注册。第二个参数把私有 `bank` 绑定为该 controller 的驱动数据，令回调能回到具体硬件资源。成功注册后 GPIO core 才有足够信息建立运行时 GPIO 接口；character device 的 `/dev/gpiochipN` 次设备编号是核心运行时分配的，不等同 legacy `base` 或扩展排针编号。

原因在于 GPIO core 的通用回调签名只传 `struct gpio_chip *gc` 和线偏移等通用参数，并不认识 Rockchip 的 `reg_base`、寄存器布局、锁或时钟。样本的 `.set`、`.get` 等回调用 `gpiochip_get_data(gc)` 取回注册时绑定的 `bank`，再访问该 bank 的资源。同一个回调模板因此可服务多个 bank，而每个 controller 始终回到自己的硬件上下文。

`struct gpio_chip` 中的 `.get`、`.set`、`.direction_input`、`.direction_output`、`.set_config`、`.to_irq` 等函数指针构成 GPIO core 到硬件驱动的操作路由表。定义该表不执行硬件操作；GPIO core 在具体请求发生时才调用相应回调。因而“controller 已注册”与“某条线已经读写或改方向”是不同阶段。

`of_get_parent(np)` 返回的是同一份 `struct device_node` 的指针，并为调用者增加一次引用，而不是复制父节点的名称、属性、子节点和关联关系。`of_node_put(pctlnp)` 归还这次临时使用权，不删除仍由设备树或其他使用者持有的节点。这样既保证并发/动态设备树场景下指针不会在使用期间失效，也避免复制复杂节点数据并保持所有使用者看到同一个节点身份；引用计数首先是生命周期规则，避免复制是它带来的重要效率与一致性收益。

## R1 的实际验证

R1 的 FIT 内 Linux DTS 有 5 个 `rockchip,gpio-bank` 节点，运行时分别对应 SoC 的 GPIO 控制器和 `/dev/gpiochip0` 至 `/dev/gpiochip4`。学习者保存的主线 7.1 `gpio-rockchip.c` 中，`rockchip_gpio_match[]` 含同一 compatible，`rockchip_gpio_driver` 把该表赋给 `.of_match_table`，并把 `.probe` 指向 `rockchip_gpio_probe`。这确认了通用绑定结构与 R1 已观察的设备树/运行时事实相容。

**限制**：该本地阅读样本是用户说明的主线 7.1 文件，R1 运行的是厂商 5.10.110；其下载 URL 与 commit 未保存。因此它只能支持通用机制学习，不能证明 R1 厂商内核的源码内容、probe 细节或排针电气状态。

## 易错点

- `compatible` 相同不等于外设已经成功工作；仍可能在 `.probe` 中缺少时钟、供电、中断或 pinctrl 资源。
- `.of_match_table` 负责声明/比较匹配资格，不执行初始化。
- `.probe` 不是“检测函数”或概率概念；它是内核在绑定设备和驱动时调用的初始化回调。
- 局部指针声明和获取 DTS/pinctrl 对象不是硬件操作；真正的资源申请、寄存器访问和 gpiochip 注册要继续看 probe 后半段。
- `-ENODEV` 和 `-EPROBE_DEFER` 都会让本轮 probe 不能继续，但前者描述缺失/不适用，后者表示可等待依赖就绪后重试。
- `of_get_parent()` 借用的是同一节点对象的引用，不是节点数据副本；每次额外借用都要在最后一次使用后配对 `of_node_put()`。
- `slock` 是自旋锁，`clk` 才是时钟对象；名字相似时应看结构体类型和初始化 API，而不是只按缩写猜测。
- DTS 的节点名或 GPIO 编号不能代替物理扩展排针编号。

## 总结

- DTS 描述硬件与资源，驱动代码实现初始化行为。
- `of_match_table` 将支持的 compatible 集合交给 platform bus。
- 匹配成功后才调用 `.probe`；运行时设备接口通常由 probe 之后建立。
- R1 的 GPIO DTS 和主线阅读样本在 compatible/匹配结构上相容，但版本差异必须保留。

## 参考资料

- 本机验证：[EXP-20260812-002](../experiment/exp-20260812-002-select-first-simple-device.md)、[EXP-20260812-004](../experiment/exp-20260812-004-locate-upstream-gpio-source-ref.md)。
