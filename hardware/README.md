# Candidate hardware architecture

This is an unfinished observation/logging carrier concept, not fabrication-
released hardware. A rendered schematic is included; editable carrier CAD,
manufacturer footprints, and third-party 3D models are not distributed here.

## Schematic snapshot

[View the full-size schematic](images/flight-computer-schematic.svg).

Exported on 2026-09-23 from the existing 2026-08-12 KiCad schematic using KiCad
10.0.5. No component values, connections, or circuit design were changed for
this publication. Crossed-out parts are marked DNP (do not populate); they are
not completed hardware assemblies. Some source-sheet annotations are crowded;
this is the existing engineering drawing, not a redrawn or fabricated result.

A fresh ERC run on 2026-09-23 reported 0 errors and 0 warnings under the project
configuration. Ignored check categories were: global label appearing only once,
four-way junction, SPICE model issue, and assigned-footprint/filter mismatch.
This is a configured schematic check, not an independent electrical-design
review, hardware measurement, or proof of flight readiness.

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

The development design is a two-layer, unrouted carrier placement. The recorded
review reported zero tracks/vias/zones and 59 unconnected pads; clean placement
or schematic checks do not mean a routed, manufacturable, or electrically
verified board. Placement DRC and schematic-to-PCB parity were not rerun for
this publication; only the schematic ERC was rerun as noted above.

Headers were hand-soldered onto the Pico 2, barometer, IMU and radio modules for
bench integration. Integrated sensor operation and carrier-board fabrication,
assembly, continuity, power and flight tests remain unverified.

Before a carrier release, resolve physical module/footprint fit, pin orientation,
sensor-to-body axes, battery polarity/protection and power isolation, supply
transients, card service/retention, antenna integration, and mechanical mounting.
Routing, full DRC, fabrication-output inspection and powered bench testing are
also unfinished. There are no ignition or recovery-deployment circuits in scope.

The separate LoRa tracker and ESP32 camera payload are not evidence that this
flight-computer carrier is integrated or flight-ready. Editable CAD publication requires
a separate manufacturer-geometry attribution/license review; see
[provenance](../PROVENANCE.md).
