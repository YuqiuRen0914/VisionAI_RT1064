# Speed-loop Bench Summary

- Date: 2026-05-13 00:14:24 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `single`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `6.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T17-measstatic6t60-20260513-001424.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `w1_50` | `speed wheel 1 50` | `50,0,0,0` | 165 | 5/5.0/5 | w1: mean=64.9 min=-348.0 max=447.4 | w1: mean=1.0 min=-19.6 max=20.2 | w1:0% | PASS |
| `w1_50_stop` | `speed wheel 1 0` | `0,0,0,0` | 53 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_100` | `speed wheel 1 100` | `100,0,0,0` | 131 | 5/5.0/5 | w1: mean=108.7 min=-348.0 max=513.7 | w1: mean=1.6 min=-20.2 max=22.9 | w1:0% | PASS |
| `w1_100_stop` | `speed wheel 1 0` | `0,0,0,0` | 53 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_-100` | `speed wheel 1 -100` | `-100,0,0,0` | 131 | 5/5.0/5 | w1: mean=-115.2 min=-646.3 max=497.1 | w1: mean=-1.4 min=-30.7 max=26.5 | w1:0% | PASS |
| `w1_-100_stop` | `speed wheel 1 0` | `0,0,0,0` | 53 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_200` | `speed wheel 1 200` | `200,0,0,0` | 131 | 5/5.0/5 | w1: mean=214.9 min=-480.6 max=911.4 | w1: mean=2.4 min=-32.5 max=37.4 | w1:0% | PASS |
| `w1_200_stop` | `speed wheel 1 0` | `0,0,0,0` | 30 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_-200` | `speed wheel 1 -200` | `-200,0,0,0` | 0 | none | w1: no samples | w1: no samples |  | NO_DATA |
| `w1_-200_stop` | `speed wheel 1 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_50` | `speed wheel 2 50` | `0,50,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_50_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_100` | `speed wheel 2 100` | `0,100,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_100_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_-100` | `speed wheel 2 -100` | `0,-100,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_-100_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_200` | `speed wheel 2 200` | `0,200,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_200_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w2_-200` | `speed wheel 2 -200` | `0,-200,0,0` | 0 | none | w2: no samples | w2: no samples |  | NO_DATA |
| `w2_-200_stop` | `speed wheel 2 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_50` | `speed wheel 3 50` | `0,0,50,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_50_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_100` | `speed wheel 3 100` | `0,0,100,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_100_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_-100` | `speed wheel 3 -100` | `0,0,-100,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_-100_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_200` | `speed wheel 3 200` | `0,0,200,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_200_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w3_-200` | `speed wheel 3 -200` | `0,0,-200,0` | 0 | none | w3: no samples | w3: no samples |  | NO_DATA |
| `w3_-200_stop` | `speed wheel 3 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_50` | `speed wheel 4 50` | `0,0,0,50` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_50_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_100` | `speed wheel 4 100` | `0,0,0,100` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_100_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_-100` | `speed wheel 4 -100` | `0,0,0,-100` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_-100_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_200` | `speed wheel 4 200` | `0,0,0,200` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_200_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |
| `w4_-200` | `speed wheel 4 -200` | `0,0,0,-200` | 0 | none | w4: no samples | w4: no samples |  | NO_DATA |
| `w4_-200_stop` | `speed wheel 4 0` | `0,0,0,0` | 0 | none | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples | w1: no samples<br>w2: no samples<br>w3: no samples<br>w4: no samples |  | NO_DATA |

Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.
