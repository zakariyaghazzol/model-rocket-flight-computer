# Build, test and replay

## Requirements and commands

Use CMake 3.16+ with a C++17 compiler and Python 3.10+. On Windows, use a Visual
Studio developer shell or an installed Visual Studio CMake generator. No MCU
toolchain, hardware, network access, or third-party Python package is required
for these host tests.

From the repository root:

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
python -m unittest discover -s ground_station/tests -v
```

The first CTest entry runs ten C++ scenarios: nominal flight progression,
noise/false launch, missing sensors, noncritical failures, in-flight reset,
impossible measurements, CSV/SD failure, telemetry sequence/bounds, wire-state
mapping, and orchestration/watchdog/persistence. The second entry runs the
synthetic classifier and requires it to reach `LANDED`.

The twelve Python tests cover both receiver serial formats, legacy packets,
`NO_FIX`, malformed diagnostics, `FC1` encode/parse agreement, stable state
mapping, sequence gaps/wrap, and a synthetic telemetry profile.

## Synthetic output

For single-configuration generators such as Ninja or Unix Makefiles:

```sh
./build/firmware/flight_computer/synthetic_flight
```

For Visual Studio or another multi-configuration generator, the executable is
under the selected configuration. In PowerShell:

```powershell
.\build\firmware\flight_computer\Debug\synthetic_flight.exe
```

The program writes CSV to stdout. It directly supplies synthetic `FlightData`
to the classifier, bypassing the filter and platform interfaces. The trajectory
is a deterministic test fixture, not a rocket performance prediction.

## Ground replay

Preview finite synthetic frames without opening hardware:

```sh
python -m ground_station.tools.hil_serial_replay --stdout --format B --count 5 --rate-hz 100
```

`--format A` selects separate `Telemetry:` and `LoRa RSSI:` lines; `B` selects the
combined envelope; `RAW` emits only the frame. `--input flight.csv` accepts a
saved simulator CSV instead of the generated profile.

For optional serial output, install `ground_station/requirements.txt` and supply
an explicit `--port` with a verified virtual COM pair or dedicated test path.
The default baud rate is 9600. Choose a send rate that fits the encoded frame
length and serial link; the 100 Hz example above is stdout-only. Never connect
this utility to recovery electronics. A serial/HIL run is not part of the
verification recorded below.

## Verification record — 2026-09-23

Fresh build of this curated copy on Windows, CMake 4.2.3, Visual Studio 18 2026
generator, MSVC 19.50.35728.0, Debug configuration, Python 3.14.3:

| Check | Observed result |
| --- | --- |
| Configure and C++ build | Passed |
| `rocket_flight_tests` | Passed; all ten scenario functions completed |
| `synthetic_flight_smoke` | Passed; exit code 0 |
| Python unittest discovery | 12 passed |
| Five-frame stdout replay | Passed; five combined-format lines, exit code 0 |
| Simulator output inspection | 641 samples, 0–32,000 ms; final state `LANDED`; zero nonzero-fault rows |

The simulator output showed `SELF_TEST`, `PAD`, `BOOST`, `COAST`, `APOGEE`,
`DESCENT`, and `LANDED` in order. It transitions out of `BOOT` before the first
CSV row. These results establish host behavior only; they do not establish
physical flight, sensor accuracy, power integrity or flight readiness. No Linux,
macOS, MCU-target or physical HIL test was run during this curation.
