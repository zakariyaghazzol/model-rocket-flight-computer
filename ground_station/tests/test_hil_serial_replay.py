import unittest

from ground_station.app.telemetry_parser import parse_payload
from ground_station.tools.hil_serial_replay import (
    frames_from_rows,
    generate_synthetic_rows,
)


class HilSerialReplayTests(unittest.TestCase):
    def test_default_profile_covers_all_nonfault_flight_states(self) -> None:
        rows = list(generate_synthetic_rows(200, 50))
        self.assertEqual(
            {row["state"] for row in rows},
            {
                "BOOT",
                "SELF_TEST",
                "PAD",
                "BOOST",
                "COAST",
                "APOGEE",
                "DESCENT",
                "LANDED",
            },
        )

    def test_generated_frames_round_trip_through_fc1_parser(self) -> None:
        rows = generate_synthetic_rows(20, 50)
        # Artificial protocol coordinates, not a recorded location.
        frames = list(frames_from_rows(rows, 1.0, -2.0, 4.05))
        self.assertEqual(len(frames), 20)
        packets = [parse_payload(frame) for frame in frames]
        self.assertTrue(all(packet is not None for packet in packets))
        self.assertEqual(packets[0].packet, 0)
        self.assertEqual(packets[-1].flight_state, "LANDED")
        self.assertAlmostEqual(packets[-1].battery_voltage, 4.05)


if __name__ == "__main__":
    unittest.main()
