# Candidate hardware architecture

This is an unfinished observation/logging carrier concept, not fabrication-
released hardware. No carrier CAD or third-party model is distributed here.

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
verified board. Those historical CAD checks were not rerun for this software
showcase.

Headers were hand-soldered onto the Pico 2, barometer, IMU and radio modules for
bench integration. Integrated sensor operation and carrier-board fabrication,
assembly, continuity, power and flight tests remain unverified.

Before a carrier release, resolve physical module/footprint fit, pin orientation,
sensor-to-body axes, battery polarity/protection and power isolation, supply
transients, card service/retention, antenna integration, and mechanical mounting.
Routing, full DRC, fabrication-output inspection and powered bench testing are
also unfinished. There are no ignition or recovery-deployment circuits in scope.

The separate LoRa tracker and ESP32 camera payload are not evidence that this
flight-computer carrier is integrated or flight-ready. CAD publication requires
a separate manufacturer-geometry attribution/license review; see
[provenance](../PROVENANCE.md).
