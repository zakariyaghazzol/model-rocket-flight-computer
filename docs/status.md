# Status and limitations

Status as of 2026-09-23: host-tested software prototype and candidate hardware
design; no completed physical flight.

## Demonstrated by this snapshot

- Portable C++ filtering, classification, logging, telemetry, and fault/reset
  policy tested with simulated inputs and in-memory adapters.
- A synthetic classifier trajectory reaching `LANDED`; this bypasses the filter
  and full `FlightComputer::tick()` path and is not a physical dynamics model.
- Python parsing/encoding and synthetic replay tests covering the supported
  packet and receiver formats. These are host tests, not a completed HIL test.

See [testing](testing.md) for reproducible commands and observed results.

## Not demonstrated

Integrated operation of the Pico 2, IMU, barometer, SD card and radio; measured
loop/SD timing; real watchdog/reset behavior; power-interruption robustness;
physical pressure/vibration testing; HIL; routed/fabricated carrier PCB; flight
performance or altitude accuracy. Module-header soldering does not establish
any of those results.

## Known software constraints

- State thresholds assume the 20 Hz test cadence and require measured-data
  tuning. No altitude/velocity error budget or flight envelope is validated.
- State-machine timing uses unsigned deltas, but the altitude filter treats a
  lower timestamp as regression. Full-system timer rollover is not supported
  by the current filter and has not been qualified.
- Barometer/IMU values are checked; GPS and battery inputs do not have equivalent
  finite/range validation before C++ telemetry conversion. Adapters must supply
  valid values. Invalid auxiliary inputs need dedicated validation/tests.
- `FC1` has no explicit GPS-valid bit; `(0, 0)` is interpreted as no fix by the
  Python parser. Integer units do not remove the C++ single-precision GPS input
  limitation. There is no application-layer checksum or authentication.
- The legacy parser is permissive for compatibility, not a hardened parser for
  hostile traffic. Packet-loss accounting assumes forward-ordered arrivals;
  duplicates, reordering, and transmitter restarts need additional policy.
- Retained-state storage and atomic integrity protection are platform duties;
  the marker alone is not a checksum. The saved sequence must be supplied to
  the new `FlightComputer` constructor by the platform adapter.
- Fixed formatting buffers bound message size, not real execution time. Sensor,
  storage and transport adapters must bound their own latency before hardware use.

`APOGEE`, `DESCENT`, and `FAULT` remain observer states only. There is no
deployment-control path to validate or enable.
