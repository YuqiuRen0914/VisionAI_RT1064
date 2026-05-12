# Speed-loop Bench Summary

- Date: 2026-05-13 01:04:56 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `single`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `8.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T27-w3-pos200-repeat-bounded8t60-20260513-010456.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `w3_200` | `speed wheel 3 200` | `0,0,200,0` | 197 | 5/5.0/5 | w3: mean=199.5 min=-414.2 max=778.7 | w3: mean=2.6 min=-26.0 max=33.7 | w3:0% | PASS |
| `w3_200_stop` | `speed wheel 3 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |

Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.
