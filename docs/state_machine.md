# Flight-state classifier

All thresholds are initial engineering values for simulation, not flight-proven
parameters. They live in `TransitionConfig`, must be tuned from recorded sensor
data, and must not be interpreted as recovery commands.

The barometric path uses a five-sample median, then an altitude EMA (α 0.25).
Vertical velocity is derived from filtered altitude and receives a second EMA
(α 0.20). Invalid samples are excluded rather than replaced by invented data.

## Transition criteria

| From → to | Criteria required on every confirming sample | Confirmation |
|---|---|---|
| BOOT → SELF_TEST | Initialization call completed | Immediate administrative transition; no flight inference |
| SELF_TEST → PAD | Barometer and IMU both valid | 10 consecutive samples |
| PAD → BOOST | acceleration magnitude ≥ 2.2 g **and** vertical velocity ≥ 3.0 m/s **and** filtered altitude ≥ pad + 2.0 m | 3 consecutive samples |
| BOOST → COAST | state time ≥ 150 ms **and** acceleration ≤ 1.3 g **and** vertical velocity ≥ 5.0 m/s | 5 consecutive samples; 10 s boost timeout is a bounded fallback |
| COAST → APOGEE | state time ≥ 300 ms **and** filtered altitude ≥ pad + 20 m **and** vertical velocity ≤ 0.5 m/s | 5 consecutive samples |
| APOGEE → DESCENT | state time ≥ 200 ms **and** vertical velocity ≤ −1.5 m/s | 5 consecutive samples |
| DESCENT → LANDED | state time ≥ 3 s **and** absolute vertical velocity ≤ 0.6 m/s **and** altitude ≤ pad + 8 m **and** acceleration is 0.75–1.25 g | 50 consecutive samples |

If any condition becomes false, that transition's confirmation count resets to
zero. A gap longer than 250 ms also clears transition confirmation, and a
backward timestamp is rejected (ordinary unsigned wrap remains valid in the
state-machine timing logic, but not the separate altitude filter). A single
impulse or noisy altitude point therefore cannot cause a
flight transition. The slow pad reference follows weather drift only while in
`PAD`. Default confirmation counts assume the 20 Hz cadence used by the host
tests; a platform must use a measured, scheduled cadence or retune them.

`LANDED` and `FAULT` are terminal until reset. Twenty consecutive missing or
invalid readings from either classification sensor lead to `FAULT`. A reset
whose valid persisted marker says the previous state was BOOST, COAST, APOGEE,
or DESCENT also enters `FAULT`; it does not guess where the vehicle is.

SD-card and communication faults are recorded but do not force classifier
`FAULT`, because classification and recovery must not depend on either service.
