# Speed-loop Bench Summary

- Date: 2026-05-13 01:37:13 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `single`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `8.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Speed filter: `30.0 ms`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T31-filter30-w2-main-20260513-013713.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s filtered/raw | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `w2_100` | `speed wheel 2 100` | `0,100,0,0` | 123 | 5/5.0/5 | w2: filt_mean=76.7 filt_min=-1.8 filt_max=223.7 raw_mean=76.8 raw_min=-66.2 raw_max=314.7 | w2: mean=4.5 min=-2.7 max=8.6 | w2:0% | PASS |
| `w2_100_stop` | `speed wheel 2 0` | `0,0,0,0` | 48 | 5/5.0/5 | w1: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_-100` | `speed wheel 2 -100` | `0,-100,0,0` | 120 | 5/5.0/5 | w2: filt_mean=-81.4 filt_min=-193.7 filt_max=-2.6 raw_mean=-79.9 raw_min=-281.6 raw_max=49.7 | w2: mean=-3.8 min=-7.5 max=1.4 | w2:0% | PASS |
| `w2_-100_stop` | `speed wheel 2 0` | `0,0,0,0` | 49 | 5/5.0/5 | w1: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=-0.0 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_200` | `speed wheel 2 200` | `0,200,0,0` | 123 | 5/5.0/5 | w2: filt_mean=198.1 filt_min=56.8 filt_max=356.0 raw_mean=200.4 raw_min=-49.7 raw_max=447.2 | w2: mean=4.0 min=-4.0 max=11.1 | w2:0% | PASS |
| `w2_200_stop` | `speed wheel 2 0` | `0,0,0,0` | 48 | 5/5.0/5 | w1: filt_mean=0.0 filt_min=-0.0 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_-200` | `speed wheel 2 -200` | `0,-200,0,0` | 120 | 5/5.0/5 | w2: filt_mean=-196.4 filt_min=-377.1 filt_max=0.0 raw_mean=-196.3 raw_min=-530.0 raw_max=66.2 | w2: mean=-3.9 min=-11.1 max=5.0 | w2:0% | PASS |
| `w2_-200_stop` | `speed wheel 2 0` | `0,0,0,0` | 48 | 5/5.0/5 | w1: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=-0.0 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |

Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.
