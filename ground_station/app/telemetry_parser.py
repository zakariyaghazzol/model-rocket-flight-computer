"""Backward-compatible parser for verified GPS0 and versioned FC1 packets."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

from ground_station.app.flight_protocol import (
    CODE_TO_STATE,
    PROTOCOL_ID,
    PROTOCOL_VERSION,
)


@dataclass(frozen=True)
class TelemetryPacket:
    packet: int
    latitude: Optional[float] = None
    longitude: Optional[float] = None
    altitude_m: Optional[float] = None
    satellites: Optional[int] = None
    hdop: Optional[float] = None
    speed_kmh: Optional[float] = None
    rssi_dbm: Optional[int] = None
    fix_valid: bool = True
    # New fields are appended so positional construction used by older callers
    # retains the original TelemetryPacket argument order.
    protocol_version: int = 0
    protocol_name: str = "GPS0"
    time_ms: Optional[int] = None
    flight_state: Optional[str] = None
    flight_state_code: Optional[int] = None
    vertical_velocity_mps: Optional[float] = None
    acceleration_g: Optional[float] = None
    battery_voltage: Optional[float] = None
    fault_flags: Optional[int] = None


@dataclass(frozen=True)
class SerialEvent:
    kind: str
    packet: Optional[TelemetryPacket] = None
    rssi_dbm: Optional[int] = None


_FLOAT_FIELDS = {"LAT", "LON", "ALT", "HDOP", "SPD"}
_INT_FIELDS = {"PKT", "SAT", "RSSI"}


def _fields(payload: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in payload.split(","):
        token = token.strip()
        if not token or token == "DATA" or token == "NO_FIX":
            continue
        key, separator, value = token.partition(":")
        key = key.strip().upper()
        if separator and key in _FLOAT_FIELDS | _INT_FIELDS:
            result[key] = value.strip()
    return result


def _parse_legacy_payload(payload: str) -> Optional[TelemetryPacket]:
    values = _fields(payload)
    if "PKT" not in values:
        return None
    try:
        return TelemetryPacket(
            packet=int(values["PKT"]),
            latitude=float(values["LAT"]) if "LAT" in values else None,
            longitude=float(values["LON"]) if "LON" in values else None,
            altitude_m=float(values["ALT"]) if "ALT" in values else None,
            satellites=int(values["SAT"]) if "SAT" in values else None,
            hdop=float(values["HDOP"]) if "HDOP" in values else None,
            speed_kmh=float(values["SPD"]) if "SPD" in values else None,
            rssi_dbm=int(values["RSSI"]) if "RSSI" in values else None,
            fix_valid="NO_FIX" not in payload,
        )
    except ValueError:
        return None


def _parse_fc1_payload(payload: str) -> Optional[TelemetryPacket]:
    parts = [part.strip() for part in payload.split(",")]
    if len(parts) < 11 or parts[0] != PROTOCOL_ID:
        return None
    try:
        packet = int(parts[1])
        time_ms = int(parts[2])
        state_code = int(parts[3])
        altitude_cm = int(parts[4])
        velocity_cms = int(parts[5])
        acceleration_mg = int(parts[6])
        battery_mv = int(parts[9])
        fault_flags = int(parts[10], 16)
        if (
            not 0 <= packet <= 0xFFFFFFFF
            or not 0 <= time_ms <= 0xFFFFFFFF
            or state_code not in CODE_TO_STATE
            or not 0 <= fault_flags <= 0xFFFFFFFF
        ):
            return None
        latitude_e7 = int(parts[7])
        longitude_e7 = int(parts[8])
        rssi = None
        for extra in parts[11:]:
            if extra.upper().startswith("RSSI:"):
                rssi = int(extra.partition(":")[2])
            else:
                return None
        return TelemetryPacket(
            packet=packet,
            protocol_version=PROTOCOL_VERSION,
            protocol_name=PROTOCOL_ID,
            latitude=latitude_e7 / 10_000_000.0,
            longitude=longitude_e7 / 10_000_000.0,
            altitude_m=altitude_cm / 100.0,
            rssi_dbm=rssi,
            fix_valid=latitude_e7 != 0 or longitude_e7 != 0,
            time_ms=time_ms,
            flight_state=CODE_TO_STATE[state_code],
            flight_state_code=state_code,
            vertical_velocity_mps=velocity_cms / 100.0,
            acceleration_g=acceleration_mg / 1000.0,
            battery_voltage=battery_mv / 1000.0,
            fault_flags=fault_flags,
        )
    except ValueError:
        return None


def parse_payload(payload: str) -> Optional[TelemetryPacket]:
    """Parse an unwrapped LoRa payload without changing the legacy grammar."""
    text = payload.strip()
    if not text or len(text.encode("ascii", errors="ignore")) > 251:
        return None
    if text.startswith(f"{PROTOCOL_ID},"):
        return _parse_fc1_payload(text)
    if text.startswith("PKT:"):
        return _parse_legacy_payload(text)
    return None


def parse_serial_line(line: str) -> Optional[SerialEvent]:
    """Parse one line, returning None for diagnostics or malformed input."""
    text = line.strip()
    if not text:
        return None

    if text.startswith("LoRa RSSI:"):
        value = text.removeprefix("LoRa RSSI:").strip().removesuffix("dBm").strip()
        try:
            return SerialEvent(kind="rssi", rssi_dbm=int(value))
        except ValueError:
            return None

    if text.startswith("Telemetry:"):
        payload = text.removeprefix("Telemetry:").strip()
    elif text.startswith("DATA,"):
        payload = text.removeprefix("DATA,").strip()
    else:
        return None

    packet = parse_payload(payload)
    if packet is None:
        return None
    return SerialEvent(kind="telemetry", packet=packet)


class PacketLossTracker:
    """Sequence-based loss tracker with 32-bit wrap support."""

    def __init__(self) -> None:
        self._last: Optional[int] = None
        self.received = 0
        self.missed = 0

    def observe(self, sequence: int) -> None:
        sequence &= 0xFFFFFFFF
        if self._last is not None:
            delta = (sequence - self._last) & 0xFFFFFFFF
            if 0 < delta < 0x80000000:
                self.missed += max(0, delta - 1)
        self._last = sequence
        self.received += 1

    @property
    def loss_percent(self) -> float:
        total = self.received + self.missed
        return 0.0 if total == 0 else 100.0 * self.missed / total
