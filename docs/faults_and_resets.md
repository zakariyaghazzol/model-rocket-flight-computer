# Fault, reset, and watchdog behavior

Fault flags are latched for the run so the log preserves intermittent failures.

| Bit | Hex | Meaning | State effect |
|---:|---:|---|---|
| 0 | `0x00000001` | barometer missing/invalid | `FAULT` after 20 consecutive samples |
| 1 | `0x00000002` | IMU missing/invalid | `FAULT` after 20 consecutive samples |
| 2 | `0x00000004` | SD/log sink failure | log/telemetry flag only |
| 3 | `0x00000008` | communication failure | log/telemetry flag only |
| 4 | `0x00000010` | low battery | warning flag |
| 5 | `0x00000020` | pressure/altitude outside validated range or nonfinite | sample rejected |
| 6 | `0x00000040` | acceleration/gyro outside validated range or nonfinite | sample rejected |
| 7 | `0x00000080` | timestamp regression | sample timing rejected |
| 8 | `0x00000100` | self-test timeout | immediate `FAULT` |
| 9 | `0x00000200` | reset while persisted state was in flight | immediate `FAULT` |
| 10 | `0x00000400` | watchdog was the reset cause | logged; combined with prior state policy |
| 11 | `0x00000800` | CSV line overflow | line rejected |
| 12 | `0x00001000` | telemetry frame overflow | frame rejected |

Platform startup must decode and pass the hardware reset cause. A `#RESET`
metadata record is written after the CSV header with time, cause, prior state,
and flags. Sample CSV consumers must explicitly ignore `#` metadata lines.
The watchdog is configured for 1000 ms by the portable orchestrator and fed only
after a complete acquire/filter/classify/log/transmit iteration. The selected
MCU adapter must verify that this timeout exceeds measured worst-case SD and
transport latency while still detecting a stalled loop.

The retained-state marker prevents random memory from being interpreted as a
valid state. A production adapter should add integrity protection and an atomic
update strategy appropriate to the final MCU.
