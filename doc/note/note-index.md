# 知识笔记

记录已经理解并能解释的稳定知识，如启动链、设备树、驱动模型和 NPU 基础。笔记不是命令流水账，也不保存尚未分析的错误日志。

新笔记复制 [`templates/note.md`](../templates/note.md)，文件名使用主题名称，例如 `uboot-environment.md`。结论应链接到实验或权威资料；未验证内容必须标记证据等级。

## 启动链与系统组成

- [Linux 内核启动参数：从 U-Boot 到 Ubuntu](linux-kernel-command-line.md)
- [设备树的 `model` 与 `compatible`](device-tree-model-and-compatible.md)
- [当前 R1 eMMC 分区布局与启动候选](r1-emmc-partition-layout.md)

## 关联查阅方法

打开知识笔记后，先看“前置知识”和“实际验证”，再通过属性中的 `related` 或 Obsidian 反向链接查看相关实验与问题。尚未形成稳定认识的内容保留在实验或问题记录中，不提前整理为知识笔记。
