# Speed-loop Bench Summary

- Date: 2026-05-13 01:34:07 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `single`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `8.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Speed filter: `30.0 ms`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T30-filter30-all-single-main-20260513-013407.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s filtered/raw | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `w1_100` | `speed wheel 1 100` | `100,0,0,0` | 124 | 5/5.0/5 | w1: filt_mean=90.9 filt_min=-0.0 filt_max=186.8 raw_mean=90.6 raw_min=-49.7 raw_max=281.7 | w1: mean=3.3 min=-1.5 max=7.7 | w1:0% | PASS |
| `w1_100_stop` | `speed wheel 1 0` | `0,0,0,0` | 47 | 5/5.0/5 | w1: filt_mean=-0.0 filt_min=-0.1 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_-100` | `speed wheel 1 -100` | `-100,0,0,0` | 121 | 5/5.0/5 | w1: filt_mean=-88.4 filt_min=-187.7 filt_max=-6.9 raw_mean=-87.5 raw_min=-281.7 raw_max=33.1 | w1: mean=-3.1 min=-7.2 max=1.9 | w1:0% | PASS |
| `w1_-100_stop` | `speed wheel 1 0` | `0,0,0,0` | 49 | 5/5.0/5 | w1: filt_mean=0.0 filt_min=-0.0 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_200` | `speed wheel 1 200` | `200,0,0,0` | 122 | 5/5.0/5 | w1: filt_mean=204.2 filt_min=66.0 filt_max=351.1 raw_mean=204.1 raw_min=-66.3 raw_max=497.1 | w1: mean=3.2 min=-4.1 max=10.2 | w1:0% | PASS |
| `w1_200_stop` | `speed wheel 1 0` | `0,0,0,0` | 47 | 5/5.0/5 | w1: filt_mean=-0.1 filt_min=-1.7 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_-200` | `speed wheel 1 -200` | `-200,0,0,0` | 120 | 5/5.0/5 | w1: filt_mean=-199.8 filt_min=-344.8 filt_max=-74.7 raw_mean=-197.8 raw_min=-513.7 raw_max=49.7 | w1: mean=-3.4 min=-9.8 max=3.8 | w1:0% | PASS |
| `w1_-200_stop` | `speed wheel 1 0` | `0,0,0,0` | 29 | 5/5.0/5 | w1: filt_mean=0.1 filt_min=-0.8 filt_max=1.7 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w2: filt_mean=0.0 filt_min=-0.0 filt_max=-0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w3: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0<br>w4: filt_mean=0.0 filt_min=0.0 filt_max=0.0 raw_mean=0.0 raw_min=0.0 raw_max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_100` | `speed wheel 2 100` | `0,100,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_100_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_-100` | `speed wheel 2 -100` | `0,-100,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_-100_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_200` | `speed wheel 2 200` | `0,200,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_200_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_-200` | `speed wheel 2 -200` | `0,-200,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_-200_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_100` | `speed wheel 3 100` | `0,0,100,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_100_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_-100` | `speed wheel 3 -100` | `0,0,-100,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_-100_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_200` | `speed wheel 3 200` | `0,0,200,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_200_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_-200` | `speed wheel 3 -200` | `0,0,-200,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_-200_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_100` | `speed wheel 4 100` | `0,0,0,100` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_100_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_-100` | `speed wheel 4 -100` | `0,0,0,-100` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_-100_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_200` | `speed wheel 4 200` | `0,0,0,200` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_200_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_-200` | `speed wheel 4 -200` | `0,0,0,-200` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_-200_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |

Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.
