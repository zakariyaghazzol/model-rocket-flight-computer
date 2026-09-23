import unittest

from ground_station.app.flight_protocol import (
    CODE_TO_STATE,
    STATE_TO_CODE,
    encode_fc1,
    wrap_receiver_serial,
)
from ground_station.app.telemetry_parser import (
    PacketLossTracker,
    parse_payload,
    parse_serial_line,
)


class TelemetryParserTests(unittest.TestCase):
    """Coordinate values below are artificial fixtures, not captured GPS data."""

    def test_verified_two_line_format_a(self) -> None:
        event = parse_serial_line(
            "Telemetry: PKT:588,LAT:1.123456,LON:-2.987654,ALT:50.7,"
            "SAT:10,HDOP:2.87,SPD:0.1"
        )
        self.assertIsNotNone(event)
        self.assertEqual(event.kind, "telemetry")
        self.assertEqual(event.packet.packet, 588)
        self.assertEqual(event.packet.protocol_version, 0)
        self.assertEqual(event.packet.protocol_name, "GPS0")
        self.assertAlmostEqual(event.packet.altitude_m, 50.7)
        rssi = parse_serial_line("LoRa RSSI: -47 dBm")
        self.assertEqual(rssi.kind, "rssi")
        self.assertEqual(rssi.rssi_dbm, -47)

    def test_verified_combined_format_b(self) -> None:
        event = parse_serial_line(
            "DATA,PKT:589,LAT:1.1,LON:-2.2,ALT:51.0,SAT:9,"
            "HDOP:1.2,SPD:3.4,RSSI:-58"
        )
        self.assertEqual(event.packet.packet, 589)
        self.assertEqual(event.packet.rssi_dbm, -58)

    def test_no_fix_is_preserved(self) -> None:
        event = parse_serial_line("DATA,PKT:590,NO_FIX,RSSI:-70")
        self.assertFalse(event.packet.fix_valid)
        self.assertIsNone(event.packet.latitude)

    def test_malformed_diagnostic_line_is_ignored(self) -> None:
        self.assertIsNone(parse_serial_line("RFM95 init failed"))
        self.assertIsNone(parse_serial_line("DATA,PKT:not-a-number"))

    def test_raw_legacy_gps_payload_remains_supported(self) -> None:
        packet = parse_payload(
            "PKT:77,LAT:1.1,LON:-2.2,ALT:150.5,SAT:8,HDOP:1.1,SPD:12.3"
        )
        self.assertEqual(packet.packet, 77)
        self.assertEqual(packet.protocol_version, 0)
        self.assertEqual(packet.satellites, 8)
        self.assertIsNone(packet.flight_state)

    def test_fc1_encoder_and_parser_match_wire_contract(self) -> None:
        frame = encode_fc1(
            packet=42,
            time_ms=2000,
            state="COAST",
            altitude_m=100.0,
            vertical_velocity_mps=0.0,
            acceleration_g=1.0,
            latitude=1.125,
            longitude=-2.25,
            battery_voltage=4.0,
            fault_flags=0x0000000C,
        )
        self.assertEqual(
            frame,
            "FC1,42,2000,4,10000,0,1000,11250000,-22500000,4000,0000000C",
        )
        packet = parse_payload(frame)
        self.assertEqual(packet.protocol_version, 1)
        self.assertEqual(packet.flight_state, "COAST")
        self.assertAlmostEqual(packet.vertical_velocity_mps, 0.0)
        self.assertEqual(packet.fault_flags, 0x0C)

    def test_fc1_uses_both_verified_serial_wrappers(self) -> None:
        frame = encode_fc1(
            packet=3,
            time_ms=150,
            state="BOOST",
            altitude_m=4.0,
            vertical_velocity_mps=8.0,
            acceleration_g=3.2,
            battery_voltage=3.9,
        )
        format_a = wrap_receiver_serial(frame, "A", -47)
        packet_a = parse_serial_line(format_a[0]).packet
        rssi_a = parse_serial_line(format_a[1])
        self.assertEqual(packet_a.flight_state, "BOOST")
        self.assertEqual(rssi_a.rssi_dbm, -47)

        format_b = wrap_receiver_serial(frame, "B", -58)[0]
        packet_b = parse_serial_line(format_b).packet
        self.assertEqual(packet_b.protocol_name, "FC1")
        self.assertEqual(packet_b.rssi_dbm, -58)

    def test_unknown_protocol_version_is_not_misparsed_as_legacy(self) -> None:
        self.assertIsNone(parse_payload("FC2,1,2,3"))

    def test_state_mapping_is_complete_and_stable(self) -> None:
        self.assertEqual(len(STATE_TO_CODE), 9)
        self.assertEqual(CODE_TO_STATE[0], "BOOT")
        self.assertEqual(CODE_TO_STATE[8], "FAULT")

    def test_packet_loss_and_wrap(self) -> None:
        tracker = PacketLossTracker()
        for sequence in (10, 11, 14):
            tracker.observe(sequence)
        self.assertEqual(tracker.missed, 2)
        self.assertAlmostEqual(tracker.loss_percent, 40.0)
        wrapped = PacketLossTracker()
        for sequence in (0xFFFFFFFE, 0xFFFFFFFF, 0):
            wrapped.observe(sequence)
        self.assertEqual(wrapped.missed, 0)


if __name__ == "__main__":
    unittest.main()
