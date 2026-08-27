# 问题排查

问题使用 [`templates/issue.md`](../templates/issue.md)，文件名形如 `issue-20260805-001-empty-loader.md`。一个文件只跟踪一个可观察问题；相互独立的现象分别建档。

查阅问题时依次阅读“现象 → 假设与验证 → 根因 → 回归验证”。属性中的 `related` 用于跳转到相关知识点和实验；状态分类只表示当前处理状态，不代表问题重要程度。

## 活动问题

- [ISSUE-20260807-001：MaskROM 与后续 Linux 启动状态不一致](issue-20260807-001-maskrom-and-linux-boot.md)

## 已解决问题

- [ISSUE-20260809-002：只读 `.git` 挂载阻止 Git 初始化](issue-20260809-002-read-only-git-mount.md)
- [ISSUE-20260809-003：R1 未从 Arch 主机共享 DHCP 获得租约](issue-20260809-003-r1-dhcp-lease-missing.md)
- [ISSUE-20260809-005：R1 SSH 公钥认证被拒绝](issue-20260809-005-r1-ssh-public-key-rejected.md)
- [ISSUE-20260811-001：上游 U-Boot 构建缺少 swig](issue-20260811-001-uboot-build-missing-swig.md)
- [ISSUE-20260815-001：RKLLM demo 目标用户空间 ABI 不匹配](issue-20260815-001-rkllm-demo-target-abi-mismatch.md)
- [ISSUE-20260815-002：RKLLM W8A8 矩阵乘法执行失败](issue-20260815-002-rkllm-w8a8-matmul-run-failed.md)
- [ISSUE-20260816-001：候选内核 HDMI probe 空指针 Oops](issue-20260816-001-candidate-hdmi-null-dereference.md)
- [ISSUE-20260820-001：resource DTB 覆盖 FIT AMP DTB](issue-20260820-001-resource-dtb-overrides-fit-dtb.md)
- [ISSUE-20260821-001：RKLLM 在 7 CPU AMP 候选中的 CPU mask 不匹配](issue-20260821-001-rkllm-cpu-mask-after-amp-carveout.md)

## 已归档问题

- [ISSUE-20260805-001：Loader 文件状态与来源不明](issue-20260805-001-empty-loader.md)
- [ISSUE-20260809-004：UFW 阻止 R1 共享 NAT 转发（当前不需要外网转发）](issue-20260809-004-ufw-blocks-shared-nat-forward.md)
- [ISSUE-20260810-001：R1 systemd degraded 与失败单元（当前延后）](issue-20260810-001-systemd-degraded-failed-units.md)
- [ISSUE-20260811-002：上游 RK3588 U-Boot 缺少外部启动载荷（不再追溯版本）](issue-20260811-002-uboot-missing-external-boot-blobs.md)
