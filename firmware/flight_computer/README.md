# Portable flight core

This C++17 library is independent of MCU SDKs, RTOSes and sensor drivers.
Platform code must implement `IBarometer`, `IImu`, `IDataSink`,
`ITelemetryTransport`, and `IWatchdog`.

`FlightComputer::tick()` acquires, filters, classifies, logs and encodes telemetry,
then feeds the watchdog. Formatting uses fixed 384-byte CSV and 128-byte
telemetry buffers. Core source performs no explicit heap allocation; platform
adapters and standard-library implementations still require target review.

`SimulatedBarometer` and `SimulatedImu` provide host-only test inputs. No physical
Pico 2 adapter is supplied. See [status](../../docs/status.md) for known gaps.
No recovery-deployment interface exists.
