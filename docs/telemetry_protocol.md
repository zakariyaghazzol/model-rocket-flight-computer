# Telemetry protocols

## Protocol version 0: existing GPS/LoRa baseline (`GPS0`)

The Nano-to-Uno RF payload remains:

```text
PKT:<number>,LAT:<latitude>,LON:<longitude>,ALT:<meters>,SAT:<count>,HDOP:<value>,SPD:<km/h>
```

The ground-station parser preserves both receiver serial encodings:

```text
Telemetry: PKT:...
LoRa RSSI: -58 dBm
```

```text
DATA,PKT:...,RSSI:-58
```

`GPS0` is the ground-station integration name; it is not inserted into the
actual legacy RF payload. The original bytes therefore remain unchanged.

## Protocol version 1: flight-computer compact frame (`FC1`)

The flight core emits a fixed-buffer ASCII frame to an abstract transport. It
does not directly depend on RadioHead or an RFM95 driver.

```text
FC1,packet,time_ms,state,alt_cm,vvel_cms,accel_mg,lat_e7,lon_e7,batt_mv,flags_hex
```

| Field | Encoding |
|---|---|
| packet | unsigned 32-bit attempted-frame sequence |
| time_ms | unsigned 32-bit monotonic time |
| state | stable state wire code from the table below |
| alt_cm | signed filtered barometric altitude ×100 |
| vvel_cms | signed vertical velocity ×100 |
| accel_mg | acceleration magnitude ×1000 |
| lat_e7 / lon_e7 | signed degrees ×10,000,000; zero if unavailable |
| batt_mv | voltage ×1000 |
| flags_hex | eight hexadecimal fault-mask digits |

The frame buffer is 128 bytes. `snprintf` length is checked before transmission.
The sequence increments for every encoding attempt, including failed sends, so
receivers can observe loss. Persist `nextSequence()` across resets when the
selected platform provides suitable nonvolatile or retained storage.

### Stable state mapping

| Code | State |
|---:|---|
| 0 | BOOT |
| 1 | SELF_TEST |
| 2 | PAD |
| 3 | BOOST |
| 4 | COAST |
| 5 | APOGEE |
| 6 | DESCENT |
| 7 | LANDED |
| 8 | FAULT |

Fault bits use the stable masks in [faults and resets](faults_and_resets.md).
Unknown `FC` versions or unknown version-1 state codes are rejected rather than
misparsed as legacy GPS data.

### Uno serial compatibility

An `FC1` payload can be carried by either existing serial envelope:

```text
Telemetry: FC1,packet,time_ms,state,...
LoRa RSSI: -58 dBm
```

```text
DATA,FC1,packet,time_ms,state,...,RSSI:-58
```

The Python parser accepts these plus both unchanged GPS-only envelopes.
Nano/Uno firmware and the legacy dashboard are outside this showcase. Host
compatibility tests do not establish that the physical radio path carries FC1.
The fixtures here are synthetic; no captured locations are distributed.
