# 资源档案

记录引入仓库或用于实验的镜像、Loader、源码、补丁、数据集和外部资料。至少包含：来源 URL、厂商或作者、适用板型、版本或 commit、发布日期、获取日期、文件大小、SHA-256、许可证以及验证状态。

来源和适用板型不明的二进制文件不得用于烧录。`rk3588_spl_loader.bin` 曾为空文件，随后由学习者主动删除，见已归档的 [ISSUE-20260805-001](../issue/issue-20260805-001-empty-loader.md)。

## 索引

## 源码

- [U-Boot v2026.07 上游源码](u-boot-v2026-07-upstream-source.md)：已验证 tag、提交和干净工作树；仅作学习基线，R1 适配性未确认。
- [youyeetoo R1 Linux 5.10 内核源码候选](youyeetoo-r1-linux-kernel-5-10.md)：厂商组织仓库已在线定位；是否精确匹配当前 Ubuntu 5.10.110、RKNPU 与构建/恢复路径均待核对。

## 厂商资料

- [youyeetoo R1 官方文档索引仓库](youyeetoo-r1-documentation-repository.md)：已验证 Git 身份与 README 校验；提供资料链接和 30PIN 表，不含内核源码或固件。
- [airockchip rknn-llm 官方 SDK 候选](airockchip-rknn-llm.md)：已固定主机浅克隆的 v1.3.0 提交；RK3588 的 RKLLM Toolkit、Runtime 与示例来源，当前驱动版本配对待核对。

## 厂商镜像候选

- [R1 Ubuntu Camera Image V2/V3 候选镜像](r1-ubuntu-camera-image-v2-v3.md)：本机大小和 SHA-256 已记录；直接来源、镜像格式、板型适用性和 NPU 内容待验证，禁止烧录。

## 模型候选

- [DeepSeek-R1-Distill-Qwen-1.5B W8A8 RK3588 模型候选](deepseek-r1-distill-qwen-1-5b-w8a8-rk3588.md)：主机已记录大小和 SHA-256；精确下载来源、与 R1 旧驱动的兼容性和实际推理均待验证。
