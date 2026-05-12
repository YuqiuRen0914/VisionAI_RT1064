# 上位机 ↔ 下位机 UART 通信协议 v1.3

> **背景**：上位机 = OpenART Plus 视觉决策板，运行 SAGE-PR 模型；下位机 = 电机/底盘 MCU。
> **场景**：推箱子赛题，决策是**离散**的 — 上位机每次发一条运动命令，等下位机执行完再发下一条。
> **目标**：协议简单、定长、二进制、有校验、有握手、能从字节流失步中恢复。

---

## 1. 物理层

| 项         | 值                                                                |
| ---------- | ----------------------------------------------------------------- |
| 接口       | UART (TTL 3.3V)                                                   |
| 波特率     | **115200**                                                  |
| 数据位     | 8                                                                 |
| 停止位     | 1                                                                 |
| 校验       | None (协议层做 XOR)                                               |
| 流控       | 无                                                                |
| 上位机引脚 | OpenART Plus `UART(12)` 用户串口 (`from machine import UART`) |

> 共地必须接好。线序由下位机硬件决定，建议在板子标签上印 `RX/TX`。

---

## 2. 帧格式 (定长 7 字节)

```
偏移   0      1     2     3     4     5     6
     +------+-----+-----+-----+-----+-----+------+
     | 0xAA | SEQ | CMD | DIR | VAL | CHK | 0x55 |
     +------+-----+-----+-----+-----+-----+------+
       SOF   seq   cmd   dir   val   chk   EOF
```

| 字段          | 字节   | 说明                                                              |
| ------------- | ------ | ----------------------------------------------------------------- |
| **SOF** | 0xAA   | 起始魔数，恒定                                                    |
| **SEQ** | 0–255 | 序列号，发送方每发一帧 +1 (溢出回 0)；ACK/DONE/ERROR 必须回原 SEQ |
| **CMD** | 见 §3 | 命令类型                                                          |
| **DIR** | 见 §3 | 方向/子命令，语义取决于 CMD                                       |
| **VAL** | 0–255 | 参数 (距离/角度/错误码/状态码)，语义取决于 CMD                    |
| **CHK** | XOR    | `SEQ ^ CMD ^ DIR ^ VAL` (不含 SOF/EOF)                          |
| **EOF** | 0x55   | 结束字节，恒定                                                    |

**为什么够用**：4 字节有效载荷已经能编码本任务所有动作；定长无需长度字段；XOR 1 字节足以检出常见单 bit / 字节错。

---

## 3. 命令字典

### 3.1 上行 (上位机 → 下位机)

| CMD      | 名称             | DIR         | VAL                                  | 含义                     |
| -------- | ---------------- | ----------- | ------------------------------------ | ------------------------ |
| `0x01` | **MOVE**   | 方向 (见下) | 距离 cm (1–255)                     | 沿指定方向直线移动       |
| `0x02` | **ROTATE** | 旋向 (见下) | 角度 `0x5A`=90° 或 `0xB4`=180° | 原地旋转                 |
| `0x03` | **STOP**   | 0x00        | 0x00                                 | 紧急停止，打断当前动作并进入 ERROR 状态 |
| `0x04` | **QUERY**  | 0x00        | 0x00                                 | 查询状态 (心跳/超时确认) |
| `0x05` | **RESET**  | 0x00        | 0x00                                 | 协议复位：清错并回到 IDLE |

**MOVE 的 DIR 取值**：

| DIR      | 方向  | 备注                                       |
| -------- | ----- | ------------------------------------------ |
| `0x00` | UP    | 沿当前朝向定义的 "上" — 详见 §6 坐标约定 |
| `0x01` | DOWN  |                                            |
| `0x02` | LEFT  |                                            |
| `0x03` | RIGHT |                                            |

**ROTATE 的 DIR 取值**：

| DIR      | 含义               |
| -------- | ------------------ |
| `0x00` | CCW (逆时针，左转) |
| `0x01` | CW (顺时针，右转)  |

### 3.2 下行 (下位机 → 上位机)

| CMD      | 名称             | DIR           | VAL                             | 含义                                      |
| -------- | ---------------- | ------------- | ------------------------------- | ----------------------------------------- |
| `0x81` | **ACK**    | 原命令 DIR    | 原命令 VAL                      | "我收到了，正在执行" — 必须在 30 ms 内回 |
| `0x82` | **DONE**   | 原命令 DIR    | 实际耗时 (10ms 单位, 0–2550ms) | "执行完成，可发下一条"                    |
| `0x83` | **ERROR**  | 错误码 (见下) | 上下文字节                      | "执行失败"                                |
| `0x84` | **STATUS** | 状态码 (见下) | 当前 SEQ                        | 对 QUERY 的回复 / 主动上报                |

**ERROR 的 DIR (错误码)**：

| DIR      | 含义                                                     |
| -------- | -------------------------------------------------------- |
| `0x01` | OBSTRUCTED 车被卡住 / 推不动                             |
| `0x02` | MOTOR_FAULT 电机或编码器故障                             |
| `0x03` | BAD_FRAME 帧解析失败 (CHK 错 / SOF/EOF 错)               |
| `0x04` | BAD_CMD 未知 CMD / 非法 DIR-VAL 组合                     |
| `0x05` | TIMEOUT 电机超时未完成                                   |
| `0x06` | BUSY 上一条还没 DONE 就又来 MOVE/ROTATE                  |
| `0x07` | SENSOR_INVALID = 0x07 编码器/IMU 数据无效或长时间不更新  |
| `0x08` | CONTROL_UNSTABLE = 0x08 闭环振荡、超调或无法进入完成窗口 |

错误码边界：

- `TIMEOUT`：动作超过最大允许时间仍未完成。
- `OBSTRUCTED`：电机有输出，但编码器进展不足，疑似车被卡住或推不动。
- `SENSOR_INVALID`：编码器或 IMU 数据本身不可信，例如未初始化、长时间不更新、跳变异常。
- `CONTROL_UNSTABLE`：传感器有效且动作未超时，但闭环无法稳定进入完成窗口。
- `MOTOR_FAULT`：电机驱动或输出接口异常。

**STATUS 的 DIR (状态码)**：

| DIR      | 含义                        |
| -------- | --------------------------- |
| `0x00` | IDLE 空闲                   |
| `0x01` | BUSY 正在执行某动作         |
| `0x02` | ERROR 处于错误态 (需 RESET) |

**STOP 语义**：

- `STOP` 是紧急停车，不是普通取消。
- 下位机收到合法 `STOP` 后必须立即停止电机并回 `ACK`。
- 若当时有动作正在执行，该动作被打断，不再回 `DONE`。
- `STOP` 后状态变为 `ERROR`，普通 `MOVE` / `ROTATE` 保持拒绝，直到收到合法 `RESET`。
- `RESET` 清除错误态和最近动作重放状态，使下位机回到 `IDLE`。

**RESET 语义**：

- `RESET` 是协议状态复位，不是 MCU 硬件复位。
- 下位机收到合法 `RESET` 后必须停止电机、清空当前动作控制状态、清除 `ERROR` 状态、清除最近动作重放记录，并回 `ACK`。
- `RESET` 不清除编译进固件的校准常量，也不修改运行参数持久化状态。
- `RESET` 后 `QUERY` 应返回 `STATUS / IDLE`。

---

## 4. 握手时序

**单条命令的完整生命周期**：

```
上位机                                    下位机
  |  MOVE/ROTATE  (SEQ=N) ----------->     |
  |                                        | 解析 → 校验 CHK
  |  <----------- ACK (SEQ=N)              | (≤30 ms 内回)
  |                                        | 开始执行电机
  |        ... 等待执行 ...                |
  |  <----------- DONE (SEQ=N)             | 完成
  |                                        |
  |  MOVE/ROTATE  (SEQ=N+1) --------->     |
```

**关键时间窗**：

| 事件             | 上限                        | 超时处理                                                    |
| ---------------- | --------------------------- | ----------------------------------------------------------- |
| ACK 等待         | 30 ms                       | 重发同一帧 (SEQ 不变)，最多 3 次，仍无 ACK 则视为下位机离线 |
| DONE 等待        | **可配置，默认 10 s** | 超时则发 QUERY；STATUS 若仍为 BUSY 再等 5s；再不行报警停车  |
| 连续两帧最小间隔 | 0 ms (理论) / 5 ms (建议)   | 下位机收到 BUSY 时回 `ERROR / BUSY`                       |

**重发规则**：

- 上位机重发的帧 **SEQ 不变**，且 `CMD/DIR/VAL` 必须与原帧完全相同。
- 下位机识别到重复 `SEQ + CMD + DIR + VAL` 时，**绝不重复执行动作**。
- 若原动作仍在执行，重发 `ACK`。
- 若原动作已经结束，重发该动作之前的最终结果：`DONE` 或 `ERROR`。
- 若同一 `SEQ` 搭配了不同的 `CMD/DIR/VAL`，视为协议误用，下位机回 `ERROR / BAD_CMD`，不执行新动作。
- 这要求下位机维护最近一次动作帧，以及该动作最近一次回复状态：`ACK`、`DONE` 或 `ERROR`。

---

## 5. 帧同步与错误恢复

下位机解析状态机：

```
状态 WAIT_SOF:
    读一字节
    if 字节 == 0xAA: 进入 READ_BODY
    else: 丢弃
状态 READ_BODY:
    再读 6 字节 → SEQ, CMD, DIR, VAL, CHK, EOF
    校验:
      EOF == 0x55  ?
      CHK == SEQ ^ CMD ^ DIR ^ VAL  ?
    都通过 → 分发处理
    任一失败 → 回 ERROR / BAD_FRAME (SEQ 用读到的，DIR=0x03)
              → 回到 WAIT_SOF，从下一字节继续
```

> **不要**用读到的"长度"决定继续读多少字节 — 定长帧就是为了避免这个。
> 若总是从 0xAA 后再读 6 字节，丢一帧最多损失 7 字节，绝不会一路歪下去。

---

## 6. 坐标约定 (UP/DOWN/LEFT/RIGHT 的精确含义)

上位机视角下：

- 摄像头朝前 = 车头朝前 = 全局坐标系 **+X 方向**
- **UP** = "向前走" (车头方向)
- **DOWN** = "向后走" (车尾方向)
- **LEFT** = "向左平移" (横向左)
- **RIGHT** = "向右平移" (横向右)

> 本协议假设下位机能做**横向平移**。如果实际底盘只能"转向 + 前进"，下位机内部要把 LEFT/RIGHT 拆成 `ROTATE 90° + MOVE + ROTATE -90°` 的复合动作 — 这一层封装由下位机完成，上位机不感知。
> 如果你们底盘是麦克纳姆轮 / 全向轮，原生支持平移，那就直接平移即可。
> **请下位机同学在 reply 里明确告知是哪一种**，方便上位机决定是否要主动避免发横向 MOVE。

ROTATE 的旋向 (CCW/CW) 以**俯视角度**为准：从上往下看，CCW = 逆时针 = 车头向左偏。

---

## 7. 编码示例

### 7.1 上位机发 "向前 (UP) 走 40cm"

```
SEQ = 0
CMD = 0x01 (MOVE)
DIR = 0x00 (UP)
VAL = 0x28 (40 cm)
CHK = 0x00 ^ 0x01 ^ 0x00 ^ 0x28 = 0x29

帧: AA 00 01 00 28 29 55
```

### 7.2 上位机发 "右转 90°"

```
SEQ = 1
CMD = 0x02 (ROTATE)
DIR = 0x01 (CW)
VAL = 0x5A (90°)
CHK = 0x01 ^ 0x02 ^ 0x01 ^ 0x5A = 0x58

帧: AA 01 02 01 5A 58 55
```

### 7.3 下位机回 "SEQ=0 的命令完成，用时 1.2 s"

```
SEQ = 0
CMD = 0x82 (DONE)
DIR = 0x00 (原 MOVE 的 DIR)
VAL = 0x78 (120 × 10ms = 1200 ms)
CHK = 0x00 ^ 0x82 ^ 0x00 ^ 0x78 = 0xFA

帧: AA 00 82 00 78 FA 55
```

### 7.4 下位机回 "SEQ=2 的命令被卡住"

```
SEQ = 2
CMD = 0x83 (ERROR)
DIR = 0x01 (OBSTRUCTED)
VAL = 0x00
CHK = 0x02 ^ 0x83 ^ 0x01 ^ 0x00 = 0x80

帧: AA 02 83 01 00 80 55
```

---

## 8. 参考代码

### 8.1 上位机 (MicroPython, 仅供下位机参考)

```python
from machine import UART
uart = UART(12, baudrate=115200)

def send_frame(seq, cmd, dir_, val):
    chk = seq ^ cmd ^ dir_ ^ val
    uart.write(bytearray([0xAA, seq, cmd, dir_, val, chk, 0x55]))

def move(seq, direction, dist_cm):
    send_frame(seq, 0x01, direction, dist_cm)

def rotate(seq, ccw_or_cw, deg):
    val = 0x5A if deg == 90 else 0xB4
    send_frame(seq, 0x02, ccw_or_cw, val)
```

### 8.2 下位机 (C 伪代码)

```c
#define SOF 0xAA
#define EOF_BYTE 0x55

typedef struct { uint8_t seq, cmd, dir, val; } Frame;
typedef struct { uint8_t cmd, dir, val; } Reply;

enum { WAIT_SOF, READ_BODY } state = WAIT_SOF;
uint8_t buf[6];
int idx = 0;
Frame last_action;
Reply last_reply;
uint8_t has_last_action = 0;
uint8_t last_reply_valid = 0;

int same_action(Frame a, Frame b) {
    return a.seq == b.seq && a.cmd == b.cmd && a.dir == b.dir && a.val == b.val;
}

void remember_reply(uint8_t cmd, uint8_t dir, uint8_t val) {
    last_reply.cmd = cmd;
    last_reply.dir = dir;
    last_reply.val = val;
    last_reply_valid = 1;
}

void replay_last_reply(Frame f) {
    if (last_reply_valid) {
        send_frame(f.seq, last_reply.cmd, last_reply.dir, last_reply.val);
    } else {
        send_ack(f);
    }
}

void on_byte(uint8_t b) {
    if (state == WAIT_SOF) {
        if (b == SOF) { state = READ_BODY; idx = 0; }
        return;
    }
    buf[idx++] = b;
    if (idx < 6) return;
    state = WAIT_SOF;

    // buf = [SEQ, CMD, DIR, VAL, CHK, EOF]
    if (buf[5] != EOF_BYTE) { send_error(buf[0], 0x03); return; }
    uint8_t chk = buf[0] ^ buf[1] ^ buf[2] ^ buf[3];
    if (chk != buf[4]) { send_error(buf[0], 0x03); return; }

    Frame f = { buf[0], buf[1], buf[2], buf[3] };
    handle_cmd(f);
}

void handle_cmd(Frame f) {
    // 重复 SEQ 的幂等处理
    if (has_last_action && f.seq == last_action.seq) {
        if (same_action(f, last_action)) {
            replay_last_reply(f);
        } else {
            send_error(f.seq, 0x04);
        }
        return;
    }

    switch (f.cmd) {
        case 0x01: /* MOVE   */ last_action = f; has_last_action = 1; start_move(f.dir, f.val); break;
        case 0x02: /* ROTATE */ last_action = f; has_last_action = 1; start_rotate(f.dir, f.val); break;
        case 0x03: /* STOP   */ emergency_stop(); break;
        case 0x04: /* QUERY  */ send_status(); return;  // 不需要 ACK
        case 0x05: /* RESET  */ has_last_action = 0; last_reply_valid = 0; clear_error(); break;
        default:                send_error(f.seq, 0x04); return;
    }
    send_ack(f);
    remember_reply(0x81, f.dir, f.val);
    // 当 start_move/start_rotate 完成时, 在电机回调里调 send_done/send_error,
    // 并用 remember_reply(0x82/0x83, dir, val) 记录最终结果用于重复帧重放。
}
```

---

## 9. 联调检查清单

下位机同学拿到协议后，建议按以下顺序验证：

- 上电后能从串口收到任意字节 (基础 UART 通)
- 能正确解析单帧 (用 §7 的样例帧手动 dump 进 RX)
- ACK 在 30 ms 内回出
- MOVE 40cm 实测误差 (建议 ≤ ±2 cm)
- ROTATE 90° 实测误差 (建议 ≤ ±3°)
- DONE 时间字段准确反映实际耗时
- 同 SEQ 同 payload 重发只执行一次；执行中重发 ACK，结束后重发 DONE/ERROR
- 同 SEQ 不同 payload 会回 BAD_CMD，不执行新动作
- 故意发坏 CHK / EOF，能正确回 BAD_FRAME 并继续工作
- STOP 能立即停车
- 连续 100 条命令无丢帧 / 漂移

---

## 10. 留待对齐的开放问题

本项目对齐结论如下：

1. **底盘类型**：麦克纳姆，全向轮。`LEFT` / `RIGHT` 原生支持，不需要上位机拆动作。
2. **车头方向定义**：`UP` 定义为车体物理前方。不开额外的初始化朝向命令；上位机若使用场地图，自己维护初始朝向和坐标换算。
3. **DONE 是否带误差反馈**：不需要。`DONE` 仍只回耗时，误差反馈留给后续版本或单独查询。
4. **是否需要主动上报**：不需要。本版只做命令响应和 `QUERY` 拉取状态，不做异步 push。
5. **网格步长**：不把固定网格步长写进协议。上位机按厘米发送 `MOVE`，下位机按 `counts_per_rev_x100` 和共享轮径把厘米换成编码器目标；格距是上层规划参数，不是协议参数。

---

## 11. 版本

| 版本 | 日期       | 变更                                                                                                                                                                                  |
| ---- | ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| v1.3 | 2026-05-12 | 明确 `STOP` 是紧急停车：收到后立即停车并回 `ACK`，进入 `ERROR` 状态，需 `RESET` 后恢复普通动作；明确 `RESET` 是协议状态复位，不是 MCU 硬件复位 |
| v1.2 | 2026-05-12 | 明确重复 `SEQ + CMD + DIR + VAL` 的幂等处理：执行中重发 `ACK`，结束后重发此前最终 `DONE` / `ERROR`；同 SEQ 不同 payload 视为 `BAD_CMD` |
| v1.1 | 2026-05-12 | 增加 `SENSOR_INVALID=0x07` 和 `CONTROL_UNSTABLE=0x08`；明确 `TIMEOUT` / `OBSTRUCTED` / `SENSOR_INVALID` / `CONTROL_UNSTABLE` / `MOTOR_FAULT` 边界；原有命令和值保持不变 |
| v1.0 | 2026-05-12 | 初版：7 字节帧 + ACK/DONE 两段握手 + XOR 校验                                                                                                                                         |
