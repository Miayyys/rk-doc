# 实验记录

每次实际操作使用独立的 `EXP-YYYYMMDD-NNN` 记录，采用 [`templates/experiment.md`](../templates/experiment.md)。同一天编号从 `001` 递增，文件名形如 `exp-20260805-001-identify-board.md`。

实验失败也必须保留。重复实验在原记录追加带时间的运行结果；环境或目标发生明显变化时新建实验，并互相链接。

## 索引

### 阶段 0：环境与设备识别

- [EXP-20260805-001：识别 Rockchip USB 设备与启动模式](exp-20260805-001-identify-rockusb-device.md)
- [EXP-20260807-001：连接并验证 Debug UART](exp-20260807-001-connect-debug-uart.md)
- [EXP-20260807-002：通过 Debug UART 观察 Linux 启动](exp-20260807-002-boot-linux-via-debug-uart.md)

实验结论应通过正文和 `related` 关联到对应知识笔记或问题记录；索引只按学习阶段提供入口。
