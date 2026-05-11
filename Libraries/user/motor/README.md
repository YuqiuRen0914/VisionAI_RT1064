# Motor Library

This library wraps four RC380 motor outputs driven by DRV8701E boards on the
SeekFree RT1064 motherboard.

## Hardware Defaults

- PWM frequency: `17000U` Hz
- `MOTOR1`: DIR `C9`, PWM `PWM2_MODULE1_CHA_C8`
- `MOTOR2`: DIR `C7`, PWM `PWM2_MODULE0_CHA_C6`
- `MOTOR3`: DIR `D2`, PWM `PWM2_MODULE3_CHB_D3`
- `MOTOR4`: DIR `C10`, PWM `PWM2_MODULE2_CHB_C11`
- Positive duty uses `GPIO_HIGH`; negative duty uses `GPIO_LOW`.

Edit `inc/motor_config.h` if the board wiring changes.

## API

```c
MotorStatus MotorInit(void);
MotorStatus MotorSetDuty(MotorId id, int16_t dutyPercent);
MotorStatus MotorSetAllDuty(int16_t motor1Duty,
                            int16_t motor2Duty,
                            int16_t motor3Duty,
                            int16_t motor4Duty);
MotorStatus MotorSetLeftRightDuty(int16_t leftDuty, int16_t rightDuty);
void MotorStopAll(void);
```

`dutyPercent` is clamped to `[-100, 100]`.
