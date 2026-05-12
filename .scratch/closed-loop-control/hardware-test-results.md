# 闭环控制硬件测试结果

状态：已完成烧录确认；后续串口确认、编码器校准和 IMU 台架测试按 `T01/T02/...` 多次记录，最终结论以各表的“通过/失败”和“最终接受值”为准。

本文档用于记录校准遥测固件的硬件台架测试结果。每个实验都写明现场操作、输入命令、期望现象和结果记录位置。测试可以反复执行；每次新测试都增加一个测试编号，例如 `T01`、`T02`、`T03`。串口原始输出只保留关键片段，但需要记录足够信息，让后续开发者能判断编码器校准值、IMU X 轴 yaw 方向和固件版本是否可信。

## 通用准备

操作：

1. 将车架起来，让四个轮子完全离地。
2. 接好电机电源，手边保留断电开关。
3. 将 SEEKFREE CMSIS-DAP 分配给 Windows 11 虚拟机。
4. 打开串口助手或逐飞助手，选择正确 COM 口。
5. 串口参数设置为 `115200 8N1`，无流控。
6. 串口发送命令时，每条命令后带换行。

开始任何实验前，先发送：

```text
stream off
stop
```

作用：

- `stream off`：关闭正在刷屏的数据流，避免干扰后续命令返回。
- `stop`：停车并锁定电机；如果后面要测电机，需要重新发送 `arm`。

## 测试会话

每次开始新一轮测试时，在下表追加一行，并在后面各实验记录表中使用同一个测试编号。

| 测试编号 | 日期时间                | 测试人     | 固件来源                        | 烧录/构建状态         | 串口端口         | 串口参数       | 备注                              |
| -------- | ----------------------- | ---------- | ------------------------------- | --------------------- | ---------------- | -------------- | --------------------------------- |
| T01      | 2026-05-12 15:54:04 CST |            | 当前工作区 `AI_Vision_RT1064` | 已烧录，`Verify OK` |                  | `115200 8N1` |                                   |
| T02      | 2026-05-12 17:27:56 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已烧录，`Verify OK` | Windows `COM3` | `115200 8N1` | Windows 侧串口确认和静态 IMU 采样 |
| T03      | 2026-05-12 17:33:02 CST | 用户+Codex | 当前工作区 `AI_Vision_RT1064` | 已烧录，`Verify OK` | Windows `COM3` | `115200 8N1` | 用户顺时针旋转约 90 度后读取 IMU  |
| T04      | 2026-05-12 17:43:43 CST | 用户       | 当前工作区 `AI_Vision_RT1064` | 已烧录，`Verify OK` | Windows `COM3` | `115200 8N1` | 用户粘贴顺时针 90 度连续旋转数据  |
| T09      | 2026-05-12 22:48:37 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 电机开关打开后的 raw PWM 与 speed bench 短测 |
| T10      | 2026-05-12 23:47:10 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，1 号轮，静摩擦/前馈补偿开启 |
| T11      | 2026-05-12 23:53:01 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，四个单轮，发现 2 号轮低中速响应偏弱 |
| T12      | 2026-05-12 23:56:58 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，2 号轮复测，`static=8%@120 mm/s` |
| T13      | 2026-05-12 23:59:08 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，四个单轮复测，`static=8%@120 mm/s` 使部分轮 `100 mm/s` 偏快 |
| T14      | 2026-05-13 00:02:49 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，2 号轮复测，`static=6%@120 mm/s` |
| T15      | 2026-05-13 00:03:54 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，2 号轮复测，`static=7%@120 mm/s` |
| T16      | 2026-05-13 00:05:48 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，四个单轮复测，`static=7%@120 mm/s` 仍使部分轮 `100 mm/s` 偏快 |
| T17      | 2026-05-13 00:14:24 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，measured-speed-gated static 初版，中途板子重启 |
| T18      | 2026-05-13 00:21:51 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，one-shot static 初版，1 号轮通过，2 号轮低速卡滞后重启 |
| T19      | 2026-05-13 00:25:08 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，2 号轮主目标，one-shot `static=6%@60 mm/s` |
| T20      | 2026-05-13 00:25:55 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，2 号轮主目标，one-shot `static=8%@60 mm/s` |
| T21      | 2026-05-13 00:32:52 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 已构建 `0 Error(s), 0 Warning(s)`；已烧录 `Verify OK` | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，bounded start boost，2 号轮主目标 |
| T22      | 2026-05-13 00:33:56 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 自动 speed-loop bench，bounded start boost，2 号轮 `static=12%@60 mm/s` |
| T23      | 2026-05-13 00:56:34 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 无效记录；电机开关未开 |
| T24      | 2026-05-13 01:00:27 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 部分无效；测试中途才打开电机开关 |
| T25      | 2026-05-13 01:02:00 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 2 号轮装配修正且电机开关打开后，主目标复测通过 |
| T26      | 2026-05-13 01:02:55 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 四个单轮主目标复测，3/4 号轮 `+200` 触发采样尖峰警告 |
| T27      | 2026-05-13 01:04:56 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 3 号轮 `+200 mm/s` 单点复测通过 |
| T28      | 2026-05-13 01:05:17 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 4 号轮 `+200 mm/s` 单点复测通过 |
| T29      | 2026-05-13 01:08:58 CST | Codex      | 当前工作区 `AI_Vision_RT1064` | 同 T21 固件 | Windows `COM3` | `115200 8N1` | 四轮同时和混合目标主验收通过 |

## 实验 1：烧录确认

目的：确认包含 `cal enc ...` 和 `imu yawx ...` 命令的新固件已经下载到板子。

现场操作：

1. 将 CMSIS-DAP 接到电脑。
2. 在 Parallels 中确认 CMSIS-DAP 分配给 Windows 11。
3. 保持板子供电。
4. 在仓库根目录执行烧录命令。

终端命令：

```bash
./tools/keil-vscode.sh flash
```

期望现象：

- Keil 日志出现 `Erase Done`。
- Keil 日志出现 `Programming Done`。
- Keil 日志出现 `Verify OK`。
- 日志出现 `Application running ...`，说明应用已启动。

烧录记录：

| 测试编号 | 构建命令/产物                                                              | 烧录方式                         | 烧录结果 | 关键日志      | 备注                                                                             |
| -------- | -------------------------------------------------------------------------- | -------------------------------- | -------- | ------------- | -------------------------------------------------------------------------------- |
| T01      | 本轮未重新构建；烧录使用现有 `Projects/mdk/Objects/AI_Vision_RT1064.axf` | `./tools/keil-vscode.sh flash` | 通过     | `Verify OK` | 末尾 `RDDI-DAP Error` 按 `docs/README-dev.md` 说明可视为下载后的断开连接警告 |
| T02      |                                                                            |                                  |          |               |                                                                                  |

关键日志：

```text
Erase Done.Programming Done.Verify OK.Application running ...
RDDI-DAP Error
Flash Load finished at 15:54:57
```

判定：

- 通过：已出现 `Verify OK`。
- 备注：末尾 `RDDI-DAP Error` 按 `docs/README-dev.md` 说明可视为下载后的断开连接警告。

## 实验 2：串口启动和命令清单确认

目的：确认烧录后的固件真的在运行，并且串口命令层已经包含新增校准命令。

现场操作：

1. 打开串口助手，参数设为 `115200 8N1`。
2. 复位或重新上电板子。
3. 观察启动输出。
4. 在串口助手里发送 `help`。
5. 再发送 `status`。

串口命令：

```text
help
status
```

期望启动输出：

```text
OK open_loop_test ready uart=UART1 baud=115200 ...
```

`help` 输出中应能看到：

```text
cal enc zero
cal enc total|delta
cal enc wheel <1|2|3|4> <turns>
imu yawx zero
stream off|imu_acc|imu_gyro|imu_yawx|enc5|enc100|duty
```

`status` 期望返回类似：

```text
OK rx status
OK status armed=0 stream=off
```

结果记录：

| 测试编号 | 启动输出出现   | `cal enc zero` | `cal enc wheel` | `imu yawx zero` | `stream imu_yawx` | `status` 返回摘要    | 通过/失败 | 备注                                             |
| -------- | -------------- | ---------------- | ----------------- | ----------------- | ------------------- | ---------------------- | --------- | ------------------------------------------------ |
| T01      |                |                  |                   |                   |                     |                        |           |                                                  |
| T02      | 未捕获启动输出 | 是               | 是                | 是                | 是                  | `armed=0 stream=off` | 通过      | Windows `COM3` 可通信，`help` 已列出新增命令 |

## 实验 3：编码器总数和方向快速检查

目的：确认四个编码器都有数据，轮号对应正确，手动向前转轮时计数方向符合“向前为正”的约定。

现场操作：

1. 车保持架空，四个轮子离地。
2. 不要发送 `arm`，这个实验只用手转轮子。
3. 每次只转一个轮子，其他轮子尽量保持不动。
4. 按轮号顺序测试：1 右前，2 左前，3 右后，4 左后。
5. 每测一个轮子前都重新清零基线。

先确认总数可读：

```text
stream off
cal enc total
```

期望输出：

```text
DATA cal_enc_total w1=<count> w2=<count> w3=<count> w4=<count>
```

测试 1 号轮：

```text
cal enc zero
```

手动操作：将 1 号轮，也就是右前轮，按车辆前进时该轮应转动的方向转动约 1/4 到 1/2 圈。

然后发送：

```text
cal enc delta
```

期望输出：

```text
DATA cal_enc_delta w1=<正数> w2=0或接近0 w3=0或接近0 w4=0或接近0
```

测试 2 号轮：

```text
cal enc zero
```

手动操作：将 2 号轮，也就是左前轮，按车辆前进时该轮应转动的方向转动约 1/4 到 1/2 圈。

然后发送：

```text
cal enc delta
```

期望输出：

```text
DATA cal_enc_delta w1=0或接近0 w2=<正数> w3=0或接近0 w4=0或接近0
```

测试 3 号轮：

```text
cal enc zero
```

手动操作：将 3 号轮，也就是右后轮，按车辆前进时该轮应转动的方向转动约 1/4 到 1/2 圈。

然后发送：

```text
cal enc delta
```

期望输出：

```text
DATA cal_enc_delta w1=0或接近0 w2=0或接近0 w3=<正数> w4=0或接近0
```

测试 4 号轮：

```text
cal enc zero
```

手动操作：将 4 号轮，也就是左后轮，按车辆前进时该轮应转动的方向转动约 1/4 到 1/2 圈。

然后发送：

```text
cal enc delta
```

期望输出：

```text
DATA cal_enc_delta w1=0或接近0 w2=0或接近0 w3=0或接近0 w4=<正数>
```

结果记录：

| 测试编号 | 轮号 | 物理位置 | 手动转动方向 | 变化字段 | 数值符号 | 通过/失败 | 备注 |
| -------- | ---: | -------- | ------------ | -------- | -------- | --------- | ---- |
| T01      |    1 | 右前     | 前进方向     |          |          |           |      |
| T01      |    2 | 左前     | 前进方向     |          |          |           |      |
| T01      |    3 | 右后     | 前进方向     |          |          |           |      |
| T01      |    4 | 左后     | 前进方向     |          |          |           |      |
| T02      |    1 | 右前     | 前进方向     |          |          |           |      |
| T02      |    2 | 左前     | 前进方向     |          |          |           |      |
| T02      |    3 | 右后     | 前进方向     |          |          |           |      |
| T02      |    4 | 左后     | 前进方向     |          |          |           |      |

判定：

- 如果转 1 号轮却是 `w2` 变化，说明编码器通道或轮号对应有问题。
- 如果向前转动时对应字段为负，优先检查或修改 `DRIVE_WHEELx_ENCODER_SIGN`。
- 只有方向和轮号确认正确后，才继续做每圈计数校准。

## 实验 4：编码器每圈计数校准

目的：多次测量每个轮子的 `counts_per_rev_x100`，比较复测一致性，然后把最终接受结果手动复制到 `drive_config.h`。

现场操作：

1. 车保持架空。
2. 选择一个正整数圈数，建议 `5` 或 `10` 圈。
3. 在轮胎上做一个临时标记，方便数整圈。
4. 每次只校准一个轮子，其他轮子保持不动。
5. 每个轮子校准前都先执行 `cal enc zero`。
6. 按车辆前进时该轮应转动的方向，手动转动指定圈数。
7. 转完后执行 `cal enc wheel <轮号> <圈数>`。

1 号轮，右前轮，示例按 5 圈校准：

```text
cal enc zero
```

手动操作：右前轮按前进方向准确转动 5 整圈。

```text
cal enc wheel 1 5
```

期望输出：

```text
OK cal enc wheel=1 counts=<计数值> turns=5 counts_per_rev_x100=<每圈计数*100>
```

2 号轮，左前轮，示例按 5 圈校准：

```text
cal enc zero
```

手动操作：左前轮按前进方向准确转动 5 整圈。

```text
cal enc wheel 2 5
```

期望输出：

```text
OK cal enc wheel=2 counts=<计数值> turns=5 counts_per_rev_x100=<每圈计数*100>
```

3 号轮，右后轮，示例按 5 圈校准：

```text
cal enc zero
```

手动操作：右后轮按前进方向准确转动 5 整圈。

```text
cal enc wheel 3 5
```

期望输出：

```text
OK cal enc wheel=3 counts=<计数值> turns=5 counts_per_rev_x100=<每圈计数*100>
```

4 号轮，左后轮，示例按 5 圈校准：

```text
cal enc zero
```

手动操作：左后轮按前进方向准确转动 5 整圈。

```text
cal enc wheel 4 5
```

期望输出：

```text
OK cal enc wheel=4 counts=<计数值> turns=5 counts_per_rev_x100=<每圈计数*100>
```

多次测试原始记录：

| 测试编号 | 轮号 | 圈数 | 计数值 | counts_per_rev_x100 | 通过/失败 | 备注 |
| -------- | ---: | ---: | -----: | ------------------: | --------- | ---- |
| T01      |    1 |   10 |  23845 |              238450 |           |      |
| T01      |    2 |   10 |  23907 |              239070 |           |      |
| T01      |    3 |   10 |  23913 |              239130 |           |      |
| T01      |    4 |   10 |  23866 |              238660 |           |      |
| T02      |    1 |   10 |  23879 |              238790 |           |      |
| T02      |    2 |   10 |  23891 |              238910 |           |      |
| T02      |    3 |   10 |  23920 |              239200 |           |      |
| T02      |    4 |   10 |  23920 |              239200 |           |      |
| T03      |    1 |   10 |  23880 |              238800 |           |      |
| T03      |    2 |   10 |  24641 |              246410 |           |      |
| T03      |    3 |   10 |  23879 |              238790 |           |      |
| T03      |    4 |   10 |  23894 |              238940 |           |      |
| T04      |    1 |   10 |  23923 |              239230 |           |      |
| T04      |    2 |   10 |  23888 |              238880 |           |      |
| T04      |    3 |   10 |  23890 |              238900 |           |      |
| T04      |    4 |   10 |  23895 |              238950 |           |      |
| T05      |    1 |   10 |  23876 |              238760 |           |      |
| T05      |    2 |   10 |  23872 |              238720 |           |      |
| T05      |    3 |   10 |  23845 |              238450 |           |      |
| T05      |    4 |   10 |  23916 |              239160 |           |      |
| T06      |    1 |   10 |  23873 |              238730 |           |      |
| T06      |    2 |   10 |  23887 |              238870 |           |      |
| T06      |    3 |   10 |  23862 |              238620 |           |      |
| T06      |    4 |   10 |  23892 |              238920 |           |      |
| T07      |    1 |   10 |  23939 |              239390 |           |      |
| T07      |    2 |   10 |  23933 |              239330 |           |      |
| T07      |    3 |   10 |  23882 |              238820 |           |      |
| T07      |    4 |   10 |  23895 |              238950 |           |      |
| T08      |    1 |   10 |  23891 |              238910 |           |      |
| T08      |    2 |   10 |  23894 |              238940 |           |      |
| T08      |    3 |   10 |  23916 |              239160 |           |      |
| T08      |    4 |   10 |  23921 |              239210 |           |      |

最终接受的校准值：

| 轮号 | 物理位置 | 来源测试编号 | 接受的 counts_per_rev_x100 | 是否已写入源码 | 备注         |
| ---: | -------- | ------------ | -------------------------: | -------------- | ------------ |
|    1 | 右前     | T01-T08      |                     238870 | 是             | 去除最大最小值后，剩余 6 次取平均 |
|    2 | 左前     | T01-T08      |                     239000 | 是             | 去除最大最小值后，剩余 6 次取平均；T03=246410 作为最大离群值剔除 |
|    3 | 右后     | T01-T08      |                     238903 | 是             | 去除最大最小值后，剩余 6 次取平均并四舍五入 |
|    4 | 左后     | T01-T08      |                     239020 | 是             | 去除最大最小值后，剩余 6 次取平均 |

本次已将 `counts_per_rev_x100` 复制到 `Libraries/user/drive/inc/drive_config.h`：

```c
#define DRIVE_WHEEL_DIAMETER_MM              (63U)
#define DRIVE_WHEEL1_COUNTS_PER_REV_X100     (238870)
#define DRIVE_WHEEL2_COUNTS_PER_REV_X100     (239000)
#define DRIVE_WHEEL3_COUNTS_PER_REV_X100     (238903)
#define DRIVE_WHEEL4_COUNTS_PER_REV_X100     (239020)
```

判定：

- `counts_per_rev_x100` 可以直接复制，不需要除以 100。
- 建议至少做两轮完整测试，例如 `T01` 和 `T02`。如果两轮结果接近，再把其中一组或平均后取整的结果写入“最终接受的校准值”。
- 如果输出为负，说明转动方向或编码器符号仍有问题。
- 如果同一个轮子重复两次差异很大，先检查是否数错圈、轮子是否带动了其他轮、编码器线是否松动。
- 这些值不会自动保存到板子 Flash；必须手动写入源码后重新构建和烧录。

## 实验 5：IMU X 轴 Yaw 观察

目的：确认当前机器人水平旋转时，IMU X 轴角速度和短时积分角度会按预期变化。后续 `ROTATE` 和 `MOVE` 航向保持会依赖这个方向。

现场操作：

1. 车放平并保持静止。
2. 关闭其他串口流。
3. 清零 IMU X yaw 观察值。
4. 开启 `imu_yawx` 流。
5. 静止观察约 5 秒，记录漂移。
6. 手动将车体绕竖直方向旋转约 90 度，记录 `gx_x10` 和 `yawx_x10` 的符号。
7. 测完立刻关闭流。

串口命令：

```text
stream off
imu yawx zero
stream imu_yawx
```

期望连续输出：

```text
DATA imu_yawx gx_x10=<X轴角速度*10> yawx_x10=<X轴积分角度*10>
```

数值含义：

- `gx_x10=123` 表示 X 轴角速度约 `12.3 deg/s`。
- `yawx_x10=900` 表示短时积分角度约 `90.0 deg`。

静止观察操作：

1. 不碰车体，观察 5 秒。
2. 记录 `gx_x10` 是否接近 0。
3. 记录 `yawx_x10` 是否缓慢漂移。

旋转观察操作：

1. 先确认当前 `yawx_x10` 接近 0。
2. 选择一个固定方向作为“正向 yaw 测试方向”，例如从上往下看逆时针。
3. 将车体绕竖直方向转动约 90 度。
4. 记录转动时 `gx_x10` 的正负。
5. 记录转完后 `yawx_x10` 的正负和大致数值。

结束命令：

```text
stream off
```

结果记录：

| 测试编号 | 静止 `gx_x10` 范围 | 静止 `yawx_x10` 漂移  | 测试的车体正向 yaw 方向 | 观察到的 `gx_x10` 符号 | 观察到的 `yawx_x10` 符号 | 约 90 度后 `yawx_x10` | 通过/失败 | 备注                            |
| -------- | -------------------- | ----------------------- | ----------------------- | ------------------------ | -------------------------- | ----------------------- | --------- | ------------------------------- |
| T01      |                      |                         |                         |                          |                            |                         |           |                                 |
| T04      | 结束静止约 -4~1      | 结束稳定从 +894 到 +891 | 顺时针旋转 90 度        | 正，峰值约 +589          | 正                         | +891~+894               | 通过      | 约 +89.1 度，方向和比例符合预期 |

判定：

- 静止时 `gx_x10` 应接近 0。
- 车体旋转时 `gx_x10` 应明显变化。
- 转完约 90 度后，`yawx_x10` 应接近 `+900` 或 `-900`，具体正负需要和项目约定统一。
- 如果转动车体时 `gx_x10` 几乎不变，说明 IMU 轴或数据读取不对，不能继续调闭环旋转。

## 实验 6：写入校准常量、重新构建和重新烧录

目的：把实验 4 接受的编码器每圈计数保存到源码，并重新烧录到板子。

现场操作：

1. 将实验 4 的四个 `counts_per_rev_x100` 填入 `drive_config.h`。
2. 保存文件。
3. 重新构建。
4. 构建通过后重新烧录。
5. 重新打开串口，重复实验 2，确认固件仍正常运行。

需要修改的源码位置：

```text
Libraries/user/drive/inc/drive_config.h
```

构建命令：

```bash
./tools/keil-vscode.sh build
```

烧录命令：

```bash
./tools/keil-vscode.sh flash
```

结果记录：

| 测试编号 | 已修改的配置 | 构建结果 | 重新烧录结果 | 重新发送 `help` 是否正常 | 是否需要新问题 | 备注 |
| -------- | ------------ | -------- | ------------ | -------------------------- | -------------- | ---- |
| T01      |              |          |              |                            |                |      |
| T02      |              |          |              |                            |                |      |

判定：

- 构建必须是 `0 Error(s)`。
- 烧录日志必须包含 `Verify OK`。
- 串口 `help` 仍应列出 `cal enc ...` 和 `imu yawx ...` 命令。

## 实验 7：速度环台架短测

目的：确认电机开关打开后，raw PWM、电机、编码器、speed bench 命令和 5 ms motion tick 的闭环观测链路可以连通。

现场操作：

1. 车保持架空，四个轮子离地。
2. 打开电机电源开关。
3. 通过 Windows `COM3` 发送短命令，只测 1 号轮。
4. 每次测试结束后发送 `speed stop`、`stream off` 和 `status`，确认回到安全状态。

raw PWM 验证命令：

```text
stream off
stop
status
cal enc zero
arm
motor 1 15 500
cal enc delta
stop
```

speed bench 验证命令：

```text
speed arm
stream speed
speed wheel 1 50
speed wheel 1 0
speed stop
stream off

speed arm
stream speed
speed wheel 1 200
speed wheel 1 0
speed stop
stream off
status
```

raw PWM 结果记录：

| 测试编号 | 轮号 | 命令 | 编码器结果 | 通过/失败 | 备注 |
| -------- | ---: | ---- | ---------- | --------- | ---- |
| T09      |    1 | `motor 1 15 500` | `DATA cal_enc_delta w1=12376 w2=0 w3=0 w4=0` | 通过 | 电机开关打开后，1 号轮 raw PWM 与编码器通道连通 |

speed bench 结果记录：

| 测试编号 | 轮号 | 目标速度 | `dt_ms` | 观测速度/编码器 | duty 摘要 | 通过/失败 | 备注 |
| -------- | ---: | -------- | ------- | ---------------- | --------- | --------- | ---- |
| T09      |    1 | `50 mm/s` | 5       | `m1=0.0 e1=0` | `d1` 约 `3.7~3.8%` | 通过但不起转 | 输出低于当前静摩擦/起转阈值，不能作为速度环失败判断 |
| T09      |    1 | `200 mm/s` | 5       | `m1` 约 `-33.1~348.0 mm/s`，`e1` 多数为 `1~21` | `d1` 约 `-6.6~12.5%` | 通过 | 速度环可驱动 1 号轮；5 ms delta 量化噪声明显，后续需要调静摩擦补偿/前馈或低速策略 |

自动 bench 结果记录：

| 测试编号 | profile/stage | 轮号 | 参数 | 目标速度 | 摘要 | 通过/失败 | 日志 |
| -------- | ------------- | ---: | ---- | -------- | ---- | --------- | ---- |
| T10      | `quick` / `normal` | 1 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=6%@60 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | `dt_ms=5` 稳定；`100/-100/200/-200` 平均速度接近目标且无持续饱和；`50 mm/s` 可起转但仍有低速粘滞；`±200 mm/s` 有明显 5 ms 量化尖峰 | 第一阶段通过，保留尖峰风险 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T10-20260512-234710.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T10-20260512-234710.summary.md` |
| T11      | `single` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=6%@60 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | 1/3/4 号轮 `±100/±200 mm/s` 平均速度接近目标；2 号轮 `100/-100 mm/s` 均值仅约 `39.3/-24.9 mm/s`，但 `200/-200 mm/s` 可达约 `179.5/-174.1 mm/s`；最终状态安全 | 部分通过；暂停四轮同时运行，先调 2 号轮低中速起转补偿 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T11-20260512-235301.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T11-20260512-235301.summary.md` |
| T12      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@120 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | 2 号轮 `50/100/-100/200/-200 mm/s` 均值约 `46.4/131.9/-113.2/187.9/-168.2 mm/s`；停轮段干净；最终状态安全 | 第一阶段通过；需要用同一参数复测四个单轮 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T12-w2-static8t120-20260512-235658.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T12-w2-static8t120-20260512-235658.summary.md` |
| T13      | `single` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@120 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | 2 号轮 `±100 mm/s` 维持可接受；但 1/3/4 号轮 `100 mm/s` 均值约 `161.5/160.5/156.1 mm/s`，`-100 mm/s` 也明显偏快；最终状态安全 | 不接受为全车候选参数；继续尝试降低 static duty | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T13-static8t120-20260512-235908.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T13-static8t120-20260512-235908.summary.md` |
| T14      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=6%@120 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | 2 号轮 `100 mm/s` 均值约 `95.5 mm/s`，但 `-100 mm/s` 仅约 `-71.5 mm/s`；最终状态安全 | 不作为首选；反向 `-100` 偏弱 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T14-w2-static6t120-20260513-000249.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T14-w2-static6t120-20260513-000249.summary.md` |
| T15      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=7%@120 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | 2 号轮 `50/100/-100/200/-200 mm/s` 均值约 `38.9/93.6/-115.9/182.8/-169.6 mm/s`；停轮段干净；最终状态安全 | 当前 2 号轮最佳折中；需要四个单轮复测 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T15-w2-static7t120-20260513-000354.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T15-w2-static7t120-20260513-000354.summary.md` |
| T16      | `single` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=7%@120 mm/s`；`ff=0.02` | `50,100,-100,200,-200 mm/s` | 2 号轮 `±100 mm/s` 可接受；但 1/3/4 号轮 `100 mm/s` 均值仍约 `153.2/160.8/150.8 mm/s`，说明 static duty 在 `100 mm/s` 稳态持续叠加 | 不接受为全车候选参数；改代码为 measured-speed-gated static duty 后复测 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T16-static7t120-20260513-000548.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T16-static7t120-20260513-000548.summary.md` |
| T17      | `single` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=6%@60 mm/s`；`ff=0.02`；measured-speed-gated static 初版 | `50,100,-100,200,-200 mm/s` | 1 号轮 `100/-100/200 mm/s` 均值约 `108.7/-115.2/214.9 mm/s`，说明稳态偏快已消失；但 `50/200` 出现大幅 duty/速度振荡，`w1_-200` 前板子重启，后续步骤 `NO_DATA` | 不接受；将 static 改为 one-shot start boost 后复测 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T17-measstatic6t60-20260513-001424.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T17-measstatic6t60-20260513-001424.summary.md` |
| T18      | `single` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=6%@60 mm/s`；`ff=0.02`；one-shot static 初版 | `50,100,-100,200,-200 mm/s` | 1 号轮 `100/-100/200/-200 mm/s` 均值约 `92.7/-97.7/202.5/-207.1 mm/s`；2 号轮 `50 mm/s` 基本未动，随后 `w2_100` 前板子重启 | 1 号轮通过；2 号轮低速卡滞风险，不能作为完整四轮结论 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T18-oneshotstatic6t60-20260513-002151.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T18-oneshotstatic6t60-20260513-002151.summary.md` |
| T19      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=6%@60 mm/s`；`ff=0.02`；one-shot static 初版 | `100,-100,200,-200 mm/s` | 2 号轮 `100/-100/200/-200 mm/s` 均值约 `11.1/-20.2/199.1/-170.0 mm/s`；不重启；停轮干净 | `±100` 太弱；`±200` 通过 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T19-w2-main-oneshot6t60-20260513-002508.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T19-w2-main-oneshot6t60-20260513-002508.summary.md` |
| T20      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；one-shot static 初版 | `100,-100,200,-200 mm/s` | 2 号轮 `100/-100/200/-200 mm/s` 均值约 `37.3/-43.7/185.0/-169.8 mm/s`；不重启；停轮干净 | `±100` 仍太弱；boost 太短，改为 bounded start boost 后复测 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T20-w2-main-oneshot8t60-20260513-002555.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T20-w2-main-oneshot8t60-20260513-002555.summary.md` |
| T21      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost | `100,-100,200,-200 mm/s` | 2 号轮 `100/-100/200/-200 mm/s` 均值约 `6.0/-32.8/196.2/-167.3 mm/s`；停轮干净；最终状态安全 | 不作为 PI 结论；后续现场发现 2 号轮装配问题 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T21-w2-main-bounded8t60-20260513-003252.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T21-w2-main-bounded8t60-20260513-003252.summary.md` |
| T22      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=12%@60 mm/s`；`ff=0.02`；bounded start boost | `100,-100,200,-200 mm/s` | 2 号轮 `100/-100/200/-200 mm/s` 均值约 `19.2/-13.2/178.4/-166.7 mm/s`；停轮干净；最终状态安全 | 不作为 PI 结论；后续现场发现 2 号轮装配问题 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T22-w2-main-bounded12t60-20260513-003356.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T22-w2-main-bounded12t60-20260513-003356.summary.md` |
| T23      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost | `100,-100,200,-200 mm/s` | 四个目标测得速度均为 `0.0 mm/s`，但 duty 有输出；最终状态安全 | 无效记录；测试时电机开关未开，不用于判断速度环 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T23-w2-after-assembly-bounded8t60-20260513-005634.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T23-w2-after-assembly-bounded8t60-20260513-005634.summary.md` |
| T24      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost | `100,-100,200,-200 mm/s` | 前三个目标速度为 `0.0 mm/s`，最后 `-200 mm/s` 均值约 `-198.0 mm/s`；最终状态安全 | 部分无效；测试中途才打开电机开关，不用于判断方向或 PI | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T24-w2-after-poweron-bounded8t60-20260513-010027.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T24-w2-after-poweron-bounded8t60-20260513-010027.summary.md` |
| T25      | `single` / `normal` | 2 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost | `100,-100,200,-200 mm/s` | 2 号轮 `100/-100/200/-200 mm/s` 均值约 `85.3/-82.2/192.3/-197.6 mm/s`；停轮干净；最终状态安全 | 通过；2 号轮装配和电机开关条件正确后，主目标可用 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T25-w2-after-motor-switch-bounded8t60-20260513-010200.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T25-w2-after-motor-switch-bounded8t60-20260513-010200.summary.md` |
| T26      | `single` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost | `100,-100,200,-200 mm/s` | 1/2 号轮全 PASS；3/4 号轮 `±100` 和 `-200` PASS；3/4 号轮 `+200` 均值约 `207.8/211.0 mm/s`，但因 5 ms 采样尖峰被脚本标记 `SIGN_CHECK`；所有停轮段干净；最终状态安全 | 第一阶段基本通过；对 3/4 号轮 `+200` 做单点复测确认 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T26-all-single-main-bounded8t60-20260513-010255.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T26-all-single-main-bounded8t60-20260513-010255.summary.md` |
| T27      | `single` / `normal` | 3 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost；`duration=6 s` | `200 mm/s` | 3 号轮 `200 mm/s` 均值约 `199.5 mm/s`；无持续饱和；停轮干净；最终状态安全 | 通过；T26 的 3 号轮 `+200` 警告按采样尖峰处理 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T27-w3-pos200-repeat-bounded8t60-20260513-010456.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T27-w3-pos200-repeat-bounded8t60-20260513-010456.summary.md` |
| T28      | `single` / `normal` | 4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost；`duration=6 s` | `200 mm/s` | 4 号轮 `200 mm/s` 均值约 `198.1 mm/s`；无持续饱和；停轮干净；最终状态安全 | 通过；T26 的 4 号轮 `+200` 警告按采样尖峰处理 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T28-w4-pos200-repeat-bounded8t60-20260513-010517.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T28-w4-pos200-repeat-bounded8t60-20260513-010517.summary.md` |
| T29      | `all` / `normal` | 1-4 | `kp=0.05 ki=0.02 kd=0`；`duty_limit=50%`；`max_speed=400 mm/s`；`static=8%@60 mm/s`；`ff=0.02`；bounded start boost；`duration=4 s` | `all ±100/±200 mm/s`；`mixed ±100 mm/s` | 四轮同向 `±100/±200 mm/s` 全 PASS；混合 `100,-100,100,-100` 和 `-100,100,-100,100` 全 PASS；所有停轮段干净；最终状态安全 | 通过；轮速闭环第一阶段主验收完成，可进入 MOVE/ROTATE 上层闭环接入 | raw: `.scratch/closed-loop-control/serial-logs/speed-bench-T29-allwheel-mixed-main-bounded8t60-20260513-010858.log`；summary: `.scratch/closed-loop-control/serial-logs/speed-bench-T29-allwheel-mixed-main-bounded8t60-20260513-010858.summary.md` |

结束状态：

```text
OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off
```

判定：

- raw PWM 有明确编码器 delta，说明电机供电、驱动、1 号编码器链路正常。
- `50 mm/s` 目标下默认 PI 输出过小，暂时低于起转阈值。
- `200 mm/s` 目标下速度环已能驱动轮子，但低速和 5 ms 采样下的观测噪声会影响控制平滑性。
- T10 使用预留静摩擦补偿和前馈后，1 号轮 `±100/±200 mm/s` 达到第一阶段稳定响应标准；`50 mm/s` 只作为可靠起转观察，不作为低速丝滑验收。
- T11 四个单轮测试显示 2 号轮在 `±100 mm/s` 响应偏弱，不能直接进入四轮同时运行；下一步先提高 2 号轮低中速段静摩擦补偿覆盖范围，再复测 2 号轮。
- T12 将静摩擦补偿改为 `8%@120 mm/s` 后，2 号轮 `±100 mm/s` 恢复到可接受范围；下一步用同一参数复测四个单轮，确认 1/3/4 号轮不会因补偿增大而明显过冲。
- T13 显示 `8%@120 mm/s` 对 1/3/4 号轮的 `±100 mm/s` 过强，因此不作为全车候选参数；下一步尝试 `6%@120 mm/s`，即只扩大补偿覆盖速度，不提高补偿 duty。
- T14/T15 对比显示 `7%@120 mm/s` 比 `6%@120 mm/s` 更适合 2 号轮 `±100 mm/s`；下一步用 `7%@120 mm/s` 做四轮单轮复测。
- T16 显示 `7%@120 mm/s` 仍让 1/3/4 号轮 `±100 mm/s` 偏快；代码检查发现 static duty 原先按目标速度阈值触发，导致 `100 mm/s` 稳态常驻补偿。已改为按实测轮速阈值触发，起步阶段补偿，轮子跑起来后撤掉。
- T17 验证 measured-speed-gated static 可以消除 `100 mm/s` 稳态偏快，但 5 ms 量化低速会反复触发补偿并造成振荡；static 已进一步改为 one-shot start boost，目标变化时装填，轮子按目标方向越过阈值后撤销。
- T18/T19/T20 显示 one-shot boost 不会造成 1 号轮稳态偏快，但对 2 号轮 `±100 mm/s` 助推时间过短；static 已进一步改为 bounded start boost，至少保持短起步窗口，再按实测轮速或最大窗口撤销。
- T21/T22 后经现场检查发现 2 号轮存在装配问题，因此这些记录只作为硬件问题暴露证据，不再用于评价 PI 或 bounded start boost。
- T23/T24 分别受“电机开关未开”和“中途才打开电机开关”影响，均不用于判断速度环。
- T25/T26/T27/T28 在 2 号轮装配修正且电机开关已打开后完成；`static=8%@60 mm/s`、`ff=0.02`、`kp=0.05 ki=0.02` 可作为第一阶段轮速闭环候选配置。
- T29 验证四轮同时同向和混合正反目标均可通过主验收，说明速度环可作为后续 MOVE/ROTATE 的内环候选。
- 当前第一阶段主目标 `±100/±200 mm/s` 已可进入下一环节；`50 mm/s` 仍只作为可靠起转观察，低速丝滑不阻塞 MOVE/ROTATE 的首版闭环。
