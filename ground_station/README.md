# Ground-side protocol tools

This directory contains reusable protocol utilities, not the separate legacy
Tkinter dashboard or Nano/Uno radio firmware.

- `app/telemetry_parser.py`: legacy GPS payload and `FC1` parsing, receiver
  envelopes, and sequence-loss accounting.
- `app/flight_protocol.py`: Python `FC1` encoder and stable state/fault mappings.
- `tools/hil_serial_replay.py`: finite synthetic or CSV replay to stdout or an
  explicitly selected serial port. Its name describes its intended use; a
  completed hardware-in-the-loop run is not claimed.

Run from the repository root with Python 3.10+:

```sh
python -m unittest discover -s ground_station/tests -v
python -m ground_station.tools.hil_serial_replay --stdout --format B --count 5 --rate-hz 100
```

No third-party package is needed for tests or stdout. Only serial-port output
requires `python -m pip install -r ground_station/requirements.txt`.
No port is opened unless `--port` is supplied. Never connect replay output to
recovery electronics. Do not publish captured location data without review.
