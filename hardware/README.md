# Candidate hardware architecture

This is an unfinished observation/logging carrier concept, not fabrication-
released hardware. A rendered schematic is included; editable carrier CAD,
manufacturer footprints, and third-party 3D models are not distributed here.

## Schematic snapshot

[View the full-size schematic](images/flight-computer-schematic.svg).

Exported on 2026-09-23 from `ManualV1.kicad_sch` using KiCad. The sheet is labeled
"Architecture B Power Tree (Unrouted / Staging Only)." No component values,
connections, or circuit design were changed for publication.

A fresh ERC run on 2026-09-23 reported **0 errors and 3 warnings** under the
project configuration. All three are pin-type warnings involving the
open-collector ST pins of U1, U2, and U3 connected to the Pico 2 GND pin, which
is typed as a power output in the symbol. These warnings require review; this
check does not determine whether the connections are electrically correct.

Ignored check categories were: global label appearing only once, four-way
junction, SPICE model issue, and assigned-footprint/filter mismatch. This is a
configured schematic check, not an independent electrical-design review,
hardware measurement, or proof of flight readiness. The earlier schematic's
zero-warning result does not apply to this Manual V1 snapshot.

The drawing uses project module-interface symbols and standard KiCad symbols.
Module names identify Raspberry Pi and Adafruit products, not components
designed by this project. Standard symbols are from the KiCad libraries under
their [design-output exception](https://www.kicad.org/libraries/license/).
No manufacturer footprint geometry or STEP model is embedded in the SVG.

| Function | Candidate module |
| --- | --- |
| Processing | Raspberry Pi Pico 2 |
| Pressure/altitude input | Adafruit BMP581 breakout |
| Inertial input | Adafruit LSM6DSO32 breakout |
| Radio telemetry | Adafruit RFM95W 915 MHz breakout |
| Storage | microSD SPI/SDIO breakout |

The carrier remains unrouted. Placement DRC and schematic-to-PCB parity were
not rerun for this snapshot; only the schematic ERC was rerun as noted above.
Results from an earlier carrier placement do not establish the routing or
manufacturability of this revision.

Headers were hand-soldered onto the Pico 2, barometer, IMU and radio modules for
bench integration. Integrated sensor operation and carrier-board fabrication,
assembly, continuity, power and flight tests remain unverified.

Before a carrier release, resolve the ERC warnings, physical module/footprint
fit, pin orientation, sensor-to-body axes, power protection and isolation,
supply transients, card service/retention, antenna integration, and mechanical
mounting. J1 assigns battery polarity in the schematic; the physical connector
and wiring still need verification. Master-switch part/footprint selection and
fuse/polyfuse selection remain unfinished; no fuse is implemented in this sheet.
Routing, full DRC, fabrication-output inspection and powered bench testing are
also unfinished. There are no ignition or recovery-deployment circuits in scope.

Editable CAD publication requires a separate manufacturer-geometry
attribution/license review; see
[provenance](../PROVENANCE.md).
