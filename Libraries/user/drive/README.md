# Drive 驱动库

本库统一封装 RT1064 小车底盘的四路电机 PWM 输出和四路正交编码器输入。

## 硬件默认配置

- PWM 频率：`17000U` Hz
- 编码器采样周期：`5U` ms
- 编码器采样 PIT 通道：`PIT_CH0`
- `DRIVE_WHEEL_1`：DIR `C9`，PWM `PWM2_MODULE1_CHA_C8`，编码器 `QTIMER1_ENCODER1` `C0/C1`
- `DRIVE_WHEEL_2`：DIR `C7`，PWM `PWM2_MODULE0_CHA_C6`，编码器 `QTIMER1_ENCODER2` `C2/C24`
- `DRIVE_WHEEL_3`：DIR `D2`，PWM `PWM2_MODULE3_CHB_D3`，编码器 `QTIMER2_ENCODER1` `C3/C4`
- `DRIVE_WHEEL_4`：DIR `C10`，PWM `PWM2_MODULE2_CHB_C11`，编码器 `QTIMER2_ENCODER2` `C5/C25`

如果接线或编码器方向发生变化，修改 `inc/drive_config.h`。

## 接口

```c
DriveStatus DriveInit(void);
DriveStatus DriveSetDuty(DriveWheelId id, int16_t dutyPercent);
DriveStatus DriveSetAllDuty(int16_t wheel1Duty,
                            int16_t wheel2Duty,
                            int16_t wheel3Duty,
                            int16_t wheel4Duty);
void DriveStopAll(void);

DriveStatus DriveGetEncoderDelta(DriveWheelId id, int16_t *delta);
DriveStatus DriveGetAllEncoderDelta(DriveEncoderDelta *delta);
void DriveEncoderPitHandler(void);
```

`dutyPercent` 会限制在 `[-100, 100]`。编码器接口返回 PIT 采样中断捕获的原始 count 增量，不做 RPM、线速度或 count/s 换算。
