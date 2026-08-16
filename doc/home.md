# RK3588 嵌入式 Linux 知识库

本目录是 Obsidian Vault，也是学习进度、知识、实验事实和排障过程的唯一文档入口。开始新任务前先阅读[当前状态](status/current.md)；新增或修改记录时必须遵守[记录规范](recording-standard.md)。

首次使用时，在 Obsidian 中选择“打开本地仓库”并指定本 `doc/` 目录；不要选择仓库根目录。打开后将本文固定为首页即可。

## 常用入口

- **继续当前学习**：[当前状态](status/current.md) → 查看“唯一下一步”。
- **查找知识点**：[知识笔记索引](note/note-index.md) → 从概念进入相关实验和问题。
- **回看排障过程**：[问题索引](issue/issue-index.md) → 按活动、解决或归档状态查找。
- **核对实际证据**：[实验索引](experiment/experiment-index.md) → 查看命令、原始输出和结论。
- **理解学习顺序**：[学习路线](roadmap/learning-roadmap.md) → 查看阶段目标和完成标准。

## 文档板块

| 板块 | 用途 | 何时更新 |
| --- | --- | --- |
| [当前状态](status/current.md) / [变更历史](status/history.md) | 最新快照，以及阶段、任务和问题的变化轨迹 | 每次有效学习或状态变化后 |
| [学习路线](roadmap/learning-roadmap.md) | 阶段目标、前置条件和完成标准 | 阶段开始、完成或调整时 |
| [环境基线](environment/environment-index.md) | 板卡、主机、软件与连接信息 | 环境发生变化时 |
| [知识笔记](note/note-index.md) | 已理解并验证的原理性知识 | 形成稳定认识后 |
| [实验记录](experiment/experiment-index.md) | 可复现的操作、输出与结论 | 每次实际实验时 |
| [问题排查](issue/issue-index.md) | 现象、假设、证据和根因 | 发现问题直到关闭 |
| [工具手册](tool/tool-index.md) | 常用工具及命令参数 | 工具用法得到验证后 |
| [资源档案](resource/resource-index.md) | 镜像、源码、资料及校验信息 | 引入外部资源时 |
| [恢复与安全](recovery/recovery-index.md) | 烧录、备份、恢复和硬件风险 | 执行高风险操作前后 |
| [技术决策](decision/decision-index.md) | 重要选择、理由与替代方案 | 作出影响后续工作的决定时 |
| [阶段复盘](review/review-index.md) | 掌握情况、缺口和后续计划 | 每个阶段结束时 |
| [边缘 AI](ai/edge-ai-index.md) | 模型、转换、部署和性能数据 | 进入 NPU 实践阶段后 |

模板统一放在 [`templates/`](templates/)，附件统一放在 `_assets/`。目录索引只负责导航和建立主题关系，不堆放实验正文或长篇知识内容。

## 如何使用关联

正文链接负责解释“为什么相关”；文档属性中的 `related` 用于 Obsidian 反向链接和关系图。知识结论应连接到验证它的实验，问题记录应连接到相关知识点和验证实验。关系图用于发现线索，最终结论仍以正文证据为准。
