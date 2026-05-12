# Speed-loop Bench Summary

- Date: 2026-05-13 01:08:58 CST
- Host: `Windows@10.211.55.3`
- Port: `COM3` at `115200`
- Profile: `all`
- Stage: `normal`
- PID: `kp=0.05 ki=0.02 kd=0.0`
- Limits: `duty=50.0% max_speed=400.0 mm/s`
- Static: `8.0 duty`, threshold `60.0 mm/s`
- Feedforward: `0.02`
- Raw log: `/Volumes/[C] Windows 11/Users/jjp/Documents/Keil-work/AI_Vision_RT1064/.scratch/closed-loop-control/serial-logs/speed-bench-T29-allwheel-mixed-main-bounded8t60-20260513-010858.log`

| Step | Command | Targets | Samples | dt min/mean/max ms | Measured mm/s | Duty % | Saturation | Conclusion |
| ---- | ------- | ------- | ------- | ------------------ | ------------- | ------ | ---------- | ---------- |
| `all_100` | `speed all 100 100 100 100` | `100,100,100,100` | 128 | 5/5.0/5 | w1: mean=89.8 min=-33.1 max=265.1<br>w2: mean=86.7 min=-49.7 max=265.0<br>w3: mean=93.1 min=-49.7 max=265.1<br>w4: mean=91.8 min=-33.1 max=248.4 | w1: mean=3.0 min=-5.6 max=9.4<br>w2: mean=4.1 min=-4.8 max=11.0<br>w3: mean=2.3 min=-6.1 max=9.1<br>w4: mean=2.7 min=-4.9 max=9.2 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_100_stop` | `speed all 0 0 0 0` | `0,0,0,0` | 53 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_-100` | `speed all -100 -100 -100 -100` | `-100,-100,-100,-100` | 123 | 5/5.0/5 | w1: mean=-88.4 min=-215.4 max=0.0<br>w2: mean=-86.3 min=-248.4 max=33.1<br>w3: mean=-87.5 min=-281.7 max=49.7<br>w4: mean=-91.3 min=-215.3 max=16.6 | w1: mean=-3.1 min=-7.7 max=3.1<br>w2: mean=-3.5 min=-9.4 max=4.4<br>w3: mean=-2.9 min=-10.0 max=6.6<br>w4: mean=-2.7 min=-8.2 max=3.4 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_-100_stop` | `speed all 0 0 0 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_200` | `speed all 200 200 200 200` | `200,200,200,200` | 126 | 5/5.0/5 | w1: mean=197.9 min=49.7 max=397.7<br>w2: mean=194.8 min=66.2 max=331.2<br>w3: mean=211.5 min=-182.3 max=646.2<br>w4: mean=204.1 min=0.0 max=463.7 | w1: mean=3.8 min=-6.2 max=11.3<br>w2: mean=4.4 min=-2.3 max=11.0<br>w3: mean=2.5 min=-19.6 max=22.0<br>w4: mean=3.4 min=-9.6 max=13.7 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_200_stop` | `speed all 0 0 0 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_-200` | `speed all -200 -200 -200 -200` | `-200,-200,-200,-200` | 123 | 5/5.0/5 | w1: mean=-206.7 min=-447.4 max=33.1<br>w2: mean=-201.6 min=-397.5 max=-16.6<br>w3: mean=-205.0 min=-497.1 max=99.4<br>w4: mean=-208.6 min=-1093.0 max=645.9 | w1: mean=-3.2 min=-15.2 max=8.8<br>w2: mean=-3.8 min=-13.1 max=5.9<br>w3: mean=-3.2 min=-18.4 max=11.4<br>w4: mean=-2.6 min=-45.7 max=41.3 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `all_-200_stop` | `speed all 0 0 0 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `mixed_1` | `speed all 100 -100 100 -100` | `100,-100,100,-100` | 127 | 5/5.0/5 | w1: mean=87.6 min=-16.6 max=215.4<br>w2: mean=-85.7 min=-231.9 max=16.6<br>w3: mean=89.3 min=-248.5 max=480.5<br>w4: mean=-90.1 min=-265.0 max=66.2 | w1: mean=2.9 min=-3.2 max=8.3<br>w2: mean=-3.4 min=-8.7 max=3.7<br>w3: mean=2.4 min=-17.4 max=19.1<br>w4: mean=-2.7 min=-10.8 max=5.8 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `mixed_1_stop` | `speed all 0 0 0 0` | `0,0,0,0` | 51 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `mixed_2` | `speed all -100 100 -100 100` | `-100,100,-100,100` | 127 | 5/5.0/5 | w1: mean=-88.9 min=-232.0 max=16.6<br>w2: mean=86.7 min=-33.1 max=248.4<br>w3: mean=-88.5 min=-265.1 max=49.7<br>w4: mean=89.3 min=-33.1 max=265.0 | w1: mean=-2.8 min=-8.2 max=4.0<br>w2: mean=3.4 min=-4.5 max=9.6<br>w3: mean=-2.7 min=-10.0 max=5.8<br>w4: mean=2.9 min=-5.7 max=9.3 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |
| `mixed_2_stop` | `speed all 0 0 0 0` | `0,0,0,0` | 52 | 5/5.0/5 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1: mean=0.0 min=0.0 max=0.0<br>w2: mean=0.0 min=0.0 max=0.0<br>w3: mean=0.0 min=0.0 max=0.0<br>w4: mean=0.0 min=0.0 max=0.0 | w1:0%, w2:0%, w3:0%, w4:0% | PASS |

Final safety check should include `OK status mode=action_closed_loop armed=0 speed_armed=0 stream=off` in the raw log.
