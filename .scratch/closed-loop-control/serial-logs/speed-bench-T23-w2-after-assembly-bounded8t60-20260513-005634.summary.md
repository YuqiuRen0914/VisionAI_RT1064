# Speed-loop Bench Summary

- Date: 2026-05-13 00:56:34 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `single`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `8.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T23-w2-after-assembly-bounded8t60-20260513-005634.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `w2_100` | `speed wheel 2 100` | `0,100,0,0` | 133 | 5/5.0/5 | w2: mean=0.0 min=0.0 max=0.0 | w2: mean=11.7 min=8.5 max=15.0 | w2:0% | CHECK w2:SIGN_CHECK, w2:WEAK_RESPONSE |
| `w2_100_stop` | `speed wheel 2 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_-100` | `speed wheel 2 -100` | `0,-100,0,0` | 132 | 5/5.0/5 | w2: mean=0.0 min=0.0 max=0.0 | w2: mean=-11.8 min=-15.0 max=-8.5 | w2:0% | CHECK w2:SIGN_CHECK, w2:WEAK_RESPONSE |
| `w2_-100_stop` | `speed wheel 2 0` | `0,0,0,0` | 54 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_200` | `speed wheel 2 200` | `0,200,0,0` | 132 | 5/5.0/5 | w2: mean=0.0 min=0.0 max=0.0 | w2: mean=23.6 min=17.1 max=30.1 | w2:0% | CHECK w2:SIGN_CHECK, w2:WEAK_RESPONSE |
| `w2_200_stop` | `speed wheel 2 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_-200` | `speed wheel 2 -200` | `0,-200,0,0` | 130 | 5/5.0/5 | w2: mean=0.0 min=0.0 max=0.0 | w2: mean=-23.5 min=-29.9 max=-17.1 | w2:0% | CHECK w2:SIGN_CHECK, w2:WEAK_RESPONSE |
| `w2_-200_stop` | `speed wheel 2 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |

Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.
