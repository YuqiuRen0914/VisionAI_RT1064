# 串口硬件自检指南

本文档用于快速检查当前 `open_loop_test` 固件下的基础硬件状态：串口通信、电机、编码器、麦轮运动和 IMU。

当前固件使用 `UART1`，串口参数：

```text
baud=115200
data=8
stop=1
parity=none
flow=none
```

## 安全准备

1. 将车架起来，让四个轮子离地。
2. 电机电源接好，手边保留断电开关。
3. 先使用小占空比测试，建议 `10` 到 `15`。
4. 当前测试固件限制：

```text
最大占空比：30%
最长单次运行：3000 ms
```

所有电机测试结束后执行：

```text
stream off
stop
```

`stop` 会停车并重新锁定电机；再次测试电机前需要重新发送 `arm`。

## 1. 串口通信检查

打开逐飞助手或任意串口助手，选择正确 COM 口，参数设为 `115200 8N1`。

上电或复位后，应周期性看到：

```text
OK open_loop_test ready uart=UART1 baud=115200 ...
```

发送：

```text
status
```

期望返回：

```text
OK rx status
OK status armed=0 stream=off
```

判定：

- 能收到 `OK rx ...`：串口收发正常。
- 只有启动提示，发送命令无响应：检查串口发送是否带换行，或 TX/RX 是否接反。
- 没有任何输出：检查 COM 口、波特率、板子供电、固件是否已下载运行。

## 2. 电机单轮检查

先解锁：

```text
arm
```

期望返回：

```text
OK arm
```

逐个测试正转：

```text
motor 1 15 800
motor 2 15 800
motor 3 15 800
motor 4 15 800
```

逐个测试反转：

```text
motor 1 -15 800
motor 2 -15 800
motor 3 -15 800
motor 4 -15 800
```

判定：

- 每次应只有对应编号的一个轮子转动。
- 正负占空比时，同一轮应反向转动。
- 某轮不转：检查该路电机、电机驱动、电源、PWM 线和接插件。
- 转错轮：电机通道接线或轮子编号定义不一致。
- 所有命令返回 `ERR motor disarmed, send arm`：先发送 `arm`。

## 3. 编码器检查

启动编码器文本流：

```text
stream enc100
```

期望连续输出：

```text
DATA enc100 w1=0 w2=0 w3=0 w4=0
```

字段含义：

```text
w1 = 1号轮编码器
w2 = 2号轮编码器
w3 = 3号轮编码器
w4 = 4号轮编码器
```

用手分别向“前进方向”转动四个轮子，或者配合单轮电机命令测试：

```text
arm
motor 1 15 1000
motor 2 15 1000
motor 3 15 1000
motor 4 15 1000
```

期望：

```text
1号轮往前转 -> w1 为正
2号轮往前转 -> w2 为正
3号轮往前转 -> w3 为正
4号轮往前转 -> w4 为正
```

反向测试：

```text
motor 1 -15 1000
motor 2 -15 1000
motor 3 -15 1000
motor 4 -15 1000
```

期望对应 `w1/w2/w3/w4` 为负。

判定：

- 电机转但对应 `w` 一直为 0：检查编码器供电、GND、A/B 相和引脚。
- `motor 1` 转但 `w2` 变化：编码器通道接错。
- 往前转为负：修改 `Libraries/user/drive/inc/drive_config.h` 中对应的 `DRIVE_WHEELx_ENCODER_SIGN`。
- 当前约定是四个轮子“往前转”时编码器都为正。

停止刷屏：

```text
stream off
```

## 4. 麦轮组合运动检查

车仍然架空，先解锁：

```text
arm
```

依次测试：

```text
move fwd 15 1000
move back 15 1000
move left 15 1000
move right 15 1000
move ccw 15 1000
move cw 15 1000
```

判定：

- `fwd` 和 `back` 应整体相反。
- `left` 和 `right` 应整体相反。
- `ccw` 和 `cw` 应整体相反。
- 如果单轮电机和编码器都正常，但组合运动方向不对，优先检查四个轮子的物理编号和安装位置；其次检查麦轮解算符号。

可同时查看当前输出占空比：

```text
stream duty
```

示例输出：

```text
DATA duty m1=15 m2=15 m3=15 m4=15
```

字段含义：

```text
m1 = 1号电机占空比
m2 = 2号电机占空比
m3 = 3号电机占空比
m4 = 4号电机占空比
```

## 5. IMU 加速度检查

启动加速度文本流：

```text
stream imu_acc
```

示例输出：

```text
DATA imu_acc ax_mg=12 ay_mg=-30 az_mg=1001 norm_mg=1002
```

字段含义：

```text
ax_mg    = X 轴加速度，单位 mg
ay_mg    = Y 轴加速度，单位 mg
az_mg    = Z 轴加速度，单位 mg
norm_mg  = 三轴加速度模长，单位 mg
```

静止放平时重点看：

```text
norm_mg 接近 1000
```

参考范围：

```text
950 到 1050：通常可接受
明显远离 1000：检查 IMU 初始化、安装、供电或量程配置
```

方向检查：

1. 前后倾斜板子，观察某一轴明显变化。
2. 左右倾斜板子，观察另一轴明显变化。
3. 翻转 180 度，重力所在轴应从约 `+1000` 变为约 `-1000`，或反过来。

停止刷屏：

```text
stream off
```

## 6. IMU 陀螺仪和 yaw 检查

先清零 yaw：

```text
imu zero
```

启动陀螺仪文本流：

```text
stream imu_gyro
```

示例输出：

```text
DATA imu_gyro gx_x10=1 gy_x10=-2 gz_x10=0 yaw_x10=5
```

字段含义：

```text
gx_x10   = X 轴角速度 * 10，单位 deg/s * 10
gy_x10   = Y 轴角速度 * 10，单位 deg/s * 10
gz_x10   = Z 轴角速度 * 10，单位 deg/s * 10
yaw_x10  = yaw 角度 * 10，单位 deg * 10
```

换算示例：

```text
gz_x10=123   表示 12.3 deg/s
yaw_x10=900  表示 90.0 deg
```

静止时：

- `gx_x10/gy_x10/gz_x10` 应接近 0。
- `yaw_x10` 应缓慢变化或基本不变。

方向检查：

1. 执行 `imu zero`。
2. 将车体绕竖直方向旋转约 90 度。
3. 观察 `yaw_x10` 是否接近 `+900` 或 `-900`。

如果方向和控制约定相反，可在 IMU yaw 轴符号配置中统一。

停止刷屏：

```text
stream off
```

## 7. IMU 静态漂移检查

将车静止放平，执行：

```text
stream off
imu zero
imu stat 10000
```

10 秒后期望返回：

```text
OK imu stat samples=... acc_norm_mg=... gyro_x_x10=... gyro_y_x10=... gyro_z_x10=... yaw_drift_x10=...
```

重点字段：

```text
acc_norm_mg     静止加速度模长平均值
gyro_x_x10      X 轴角速度平均值
gyro_y_x10      Y 轴角速度平均值
gyro_z_x10      Z 轴角速度平均值
yaw_drift_x10   yaw 漂移量，单位 deg * 10
```

判定：

- `acc_norm_mg` 应接近 `1000`。
- `gyro_x/y/z_x10` 应接近 `0`。
- `yaw_drift_x10` 越小越好，`10` 表示漂移 `1.0 deg`。

## 8. 快速完整自检顺序

建议每次改硬件或下载新固件后按下面顺序执行：

```text
status
stream enc100
```

手转四个轮子，确认 `w1/w2/w3/w4` 对应正确且往前为正。

```text
stream off
arm
motor 1 15 800
motor 2 15 800
motor 3 15 800
motor 4 15 800
```

确认四个电机编号和方向。

```text
move fwd 15 1000
move back 15 1000
move left 15 1000
move right 15 1000
move ccw 15 1000
move cw 15 1000
```

确认麦轮组合运动。

```text
stream imu_acc
```

确认 `norm_mg` 接近 `1000`。

```text
stream off
imu zero
imu stat 10000
```

确认 IMU 静态漂移可接受。

最后：

```text
stream off
stop
```

