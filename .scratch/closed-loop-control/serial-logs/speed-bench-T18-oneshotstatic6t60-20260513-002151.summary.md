# Speed-loop Bench Summary

- Date: 2026-05-13 00:21:51 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `single`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `6.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T18-oneshotstatic6t60-20260513-002151.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `w1_50` | `speed wheel 1 50` | `50,0,0,0` | 168 | 5/5.0/5 | w1: mean=18.0 min=-33.1 max=182.3 | w1: mean=5.3 min=-2.0 max=8.7 | w1:0% | CHECK w1:SIGN_CHECK |
| `w1_50_stop` | `speed wheel 1 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_100` | `speed wheel 1 100` | `100,0,0,0` | 130 | 5/5.0/5 | w1: mean=92.7 min=-33.1 max=215.4 | w1: mean=3.0 min=-3.0 max=9.4 | w1:0% | PASS |
| `w1_100_stop` | `speed wheel 1 0` | `0,0,0,0` | 53 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_-100` | `speed wheel 1 -100` | `-100,0,0,0` | 131 | 5/5.0/5 | w1: mean=-97.7 min=-215.4 max=0.0 | w1: mean=-2.6 min=-7.6 max=3.4 | w1:0% | PASS |
| `w1_-100_stop` | `speed wheel 1 0` | `0,0,0,0` | 54 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_200` | `speed wheel 1 200` | `200,0,0,0` | 132 | 5/5.0/5 | w1: mean=202.5 min=49.7 max=364.6 | w1: mean=3.6 min=-4.6 max=11.3 | w1:0% | PASS |
| `w1_200_stop` | `speed wheel 1 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w1_-200` | `speed wheel 1 -200` | `-200,0,0,0` | 131 | 5/5.0/5 | w1: mean=-207.1 min=-563.4 max=116.0 | w1: mean=-3.2 min=-19.1 max=14.9 | w1:0% | PASS |
| `w1_-200_stop` | `speed wheel 1 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `w2_50` | `speed wheel 2 50` | `0,50,0,0` | 169 | 5/5.0/5 | w2: mean=-0.1 min=-16.6 max=0.0 | w2: mean=6.4 min=4.4 max=8.5 | w2:0% | CHECK w2:SIGN_CHECK, w2:WEAK_RESPONSE |
| `w2_50_stop` | `speed wheel 2 0` | `0,0,0,0` | 35 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
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
