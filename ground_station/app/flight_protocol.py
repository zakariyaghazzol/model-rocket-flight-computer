"""Versioned flight-computer telemetry protocol helpers.

Legacy GPS packets are protocol version 0 and are intentionally encoded only by
the verified Nano baseline. This module emits FC1 integration/test frames.
"""

from __future__ import annotations

import math
from typing import Optional


PROTOCOL_ID = "FC1"
PROTOCOL_VERSION = 1
MAX_FRAME_BYTES = 128

STATE_TO_CODE = {
    "BOOT": 0,
    "SELF_TEST": 1,
    "PAD": 2,
    "BOOST": 3,
    "COAST": 4,
    "APOGEE": 5,
    "DESCENT": 6,
    "LANDED": 7,
    "FAULT": 8,
}
CODE_TO_STATE = {code: state for state, code in STATE_TO_CODE.items()}

FAULT_FLAGS = {
    "BAROMETER_MISSING": 1 << 0,
    "IMU_MISSING": 1 << 1,
    "SD_CARD": 1 << 2,
    "COMMUNICATIONS": 1 << 3,
    "LOW_BATTERY": 1 << 4,
    "BAROMETER_RANGE": 1 << 5,
    "IMU_RANGE": 1 << 6,
    "TIME_REGRESSION": 1 << 7,
    "SELF_TEST_TIMEOUT": 1 << 8,
    "RESET_IN_FLIGHT": 1 << 9,
    "WATCHDOG_RESET": 1 << 10,
    "LOG_OVERFLOW": 1 << 11,
    "TELEMETRY_OVERFLOW": 1 << 12,
}


def _scaled(value: float, scale: float) -> int:
    scaled = value * scale
    return math.floor(scaled + 0.5) if scaled >= 0 else math.ceil(scaled - 0.5)


def encode_fc1(
    *,
    packet: int,
    time_ms: int,
    state: str | int,
    altitude_m: float,
    vertical_velocity_mps: float,
    acceleration_g: float,
    latitude: Optional[float] = None,
    longitude: Optional[float] = None,
    battery_voltage: float = 0.0,
    fault_flags: int = 0,
) -> str:
    """Encode the exact bounded ASCII frame emitted by the C++ core."""
    if isinstance(state, str):
        try:
            state_code = STATE_TO_CODE[state.upper()]
        except KeyError as error:
            raise ValueError(f"unknown flight state: {state}") from error
    else:
        state_code = int(state)
        if state_code not in CODE_TO_STATE:
            raise ValueError(f"unknown flight state code: {state_code}")
    if not 0 <= packet <= 0xFFFFFFFF or not 0 <= time_ms <= 0xFFFFFFFF:
        raise ValueError("packet and time_ms must fit unsigned 32-bit values")
    if not 0 <= fault_flags <= 0xFFFFFFFF:
        raise ValueError("fault_flags must fit an unsigned 32-bit value")

    latitude_e7 = 0 if latitude is None else _scaled(latitude, 10_000_000.0)
    longitude_e7 = 0 if longitude is None else _scaled(longitude, 10_000_000.0)
    frame = (
        f"{PROTOCOL_ID},{packet},{time_ms},{state_code},"
        f"{_scaled(altitude_m, 100.0)},{_scaled(vertical_velocity_mps, 100.0)},"
        f"{_scaled(acceleration_g, 1000.0)},{latitude_e7},{longitude_e7},"
        f"{_scaled(battery_voltage, 1000.0)},{fault_flags:08X}"
    )
    if len(frame.encode("ascii")) >= MAX_FRAME_BYTES:
        raise ValueError("FC1 frame exceeds the 127-byte payload limit")
    return frame


def wrap_receiver_serial(frame: str, serial_format: str, rssi_dbm: int) -> list[str]:
    """Wrap an RF payload in either verified Uno serial representation."""
    normalized = serial_format.upper()
    if normalized == "A":
        return [f"Telemetry: {frame}", f"LoRa RSSI: {rssi_dbm} dBm"]
    if normalized == "B":
        return [f"DATA,{frame},RSSI:{rssi_dbm}"]
    if normalized == "RAW":
        return [frame]
    raise ValueError("serial_format must be A, B, or RAW")
