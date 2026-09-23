"""Replay bounded FC1 synthetic telemetry through a serial port.

Run as a module from the repository root. This tool only exercises the
telemetry/ground-station path; it does not connect to recovery electronics.
"""

from __future__ import annotations

import argparse
import csv
import sys
import time
from collections.abc import Iterable, Iterator
from pathlib import Path

from ground_station.app.flight_protocol import encode_fc1, wrap_receiver_serial


def _parse_flags(value: str) -> int:
    text = value.strip()
    try:
        return int(text, 0)
    except ValueError:
        return int(text, 16)


def generate_synthetic_rows(count: int, interval_ms: int) -> Iterator[dict[str, object]]:
    """Generate a deterministic full-state telemetry profile for link testing."""
    for packet in range(count):
        progress = packet / max(1, count - 1)
        if progress < 0.05:
            state = "BOOT"
        elif progress < 0.15:
            state = "SELF_TEST"
        elif progress < 0.25:
            state = "PAD"
        elif progress < 0.40:
            state = "BOOST"
        elif progress < 0.55:
            state = "COAST"
        elif progress < 0.60:
            state = "APOGEE"
        elif progress < 0.88:
            state = "DESCENT"
        else:
            state = "LANDED"

        if progress < 0.25:
            altitude = 0.0
            velocity = 0.0
        elif progress < 0.60:
            altitude = 200.0 * (progress - 0.25) / 0.35
            velocity = 28.0 * (1.0 - (progress - 0.25) / 0.35)
        elif progress < 0.88:
            altitude = 200.0 * (1.0 - (progress - 0.60) / 0.28)
            velocity = -12.0
        else:
            altitude = 0.0
            velocity = 0.0
        yield {
            "packet": packet,
            "timestamp_ms": packet * interval_ms,
            "state": state,
            "altitude_m": altitude,
            "vertical_velocity_mps": velocity,
            "acceleration_g": 3.5 if state == "BOOST" else 1.0,
            "flags": 0,
        }


def load_simulation_csv(path: Path) -> Iterator[dict[str, object]]:
    """Load either the C++ simulator CSV or an equivalent recorded fixture."""
    with path.open("r", newline="", encoding="utf-8") as source:
        for packet, row in enumerate(csv.DictReader(source)):
            yield {
                "packet": packet,
                "timestamp_ms": int(row["timestamp_ms"]),
                "state": row["state"],
                "altitude_m": float(
                    row.get("barometric_altitude_m", row.get("truth_altitude_m", "0"))
                ),
                "vertical_velocity_mps": float(row["vertical_velocity_mps"]),
                "acceleration_g": float(row.get("acceleration_g", "1")),
                "flags": _parse_flags(row.get("flags", row.get("error_flags", "0"))),
            }


def frames_from_rows(
    rows: Iterable[dict[str, object]],
    latitude: float | None,
    longitude: float | None,
    battery_voltage: float,
) -> Iterator[str]:
    for row in rows:
        yield encode_fc1(
            packet=int(row["packet"]),
            time_ms=int(row["timestamp_ms"]),
            state=str(row["state"]),
            altitude_m=float(row["altitude_m"]),
            vertical_velocity_mps=float(row["vertical_velocity_mps"]),
            acceleration_g=float(row["acceleration_g"]),
            latitude=latitude,
            longitude=longitude,
            battery_voltage=battery_voltage,
            fault_flags=int(row["flags"]),
        )


def _arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    destination = parser.add_mutually_exclusive_group(required=True)
    destination.add_argument("--port", help="serial port, for example COM5")
    destination.add_argument(
        "--stdout", action="store_true", help="preview output without opening hardware"
    )
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--format", choices=("A", "B", "RAW"), default="B")
    parser.add_argument("--rate-hz", type=float, default=20.0)
    parser.add_argument("--count", type=int, default=200)
    parser.add_argument("--input", type=Path, help="optional synthetic-flight CSV")
    parser.add_argument("--rssi", type=int, default=-58)
    parser.add_argument("--latitude", type=float)
    parser.add_argument("--longitude", type=float)
    parser.add_argument("--battery-voltage", type=float, default=4.0)
    return parser.parse_args()


def main() -> int:
    args = _arguments()
    if args.rate_hz <= 0 or args.count <= 0 or args.baud <= 0:
        raise SystemExit("rate-hz, count, and baud must be positive")
    interval_ms = round(1000.0 / args.rate_hz)
    rows = (
        load_simulation_csv(args.input)
        if args.input is not None
        else generate_synthetic_rows(args.count, interval_ms)
    )
    frames = frames_from_rows(
        rows, args.latitude, args.longitude, args.battery_voltage
    )

    serial_port = None
    if args.port:
        try:
            import serial
        except ImportError as error:
            raise SystemExit(
                "PySerial is required for COM output; install ground_station/requirements.txt"
            ) from error
        serial_port = serial.Serial(
            args.port,
            baudrate=args.baud,
            timeout=1.0,
            write_timeout=2.0,
        )

    try:
        for frame in frames:
            lines = wrap_receiver_serial(frame, args.format, args.rssi)
            for line in lines:
                encoded = (line + "\r\n").encode("ascii")
                if serial_port is None:
                    sys.stdout.buffer.write(encoded)
                    sys.stdout.buffer.flush()
                else:
                    serial_port.write(encoded)
                    serial_port.flush()
            time.sleep(1.0 / args.rate_hz)
    except KeyboardInterrupt:
        return 130
    finally:
        if serial_port is not None:
            serial_port.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
