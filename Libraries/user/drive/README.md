# Drive Library

This library wraps four wheel drive outputs and four quadrature encoder inputs
for the RT1064 vehicle chassis.

## Hardware Defaults

- PWM frequency: `17000U` Hz
- Encoder sample period: `5U` ms on `PIT_CH0`
- `DRIVE_WHEEL_1`: DIR `C9`, PWM `PWM2_MODULE1_CHA_C8`, encoder `QTIMER1_ENCODER1` `C0/C1`
- `DRIVE_WHEEL_2`: DIR `C7`, PWM `PWM2_MODULE0_CHA_C6`, encoder `QTIMER1_ENCODER2` `C2/C24`
- `DRIVE_WHEEL_3`: DIR `D2`, PWM `PWM2_MODULE3_CHB_D3`, encoder `QTIMER2_ENCODER1` `C3/C4`
- `DRIVE_WHEEL_4`: DIR `C10`, PWM `PWM2_MODULE2_CHB_C11`, encoder `QTIMER2_ENCODER2` `C5/C25`

Edit `inc/drive_config.h` if the board wiring or encoder direction changes.

## API

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

`dutyPercent` is clamped to `[-100, 100]`. Encoder deltas are raw count
increments captured by the PIT sampling interrupt.
