# Provenance and publication scope

Project owner: Zakariya Ghazzol. This is the personal model-rocket flight-computer
project, not Project Neptune team work.

This showcase was curated on 2026-09-23 from the local development snapshot at
commit `11240e3b89b53a74ee06d8fe96481c8c1b6f0d26`. The source working tree was
clean at inspection. Original files and history were not changed or copied as
Git history.

## Included material

The C++ headers, core implementation, synthetic simulator, Python protocol and
replay implementation, and tests were retained from that snapshot. Curation
changes are limited to:

- standalone build/test documentation and a CTest simulator smoke-test entry;
- replacement of coordinate fixtures with explicitly synthetic values;
- correction of stale integration wording and clarification of prototype limits;
- a minimal optional PySerial dependency list, without the omitted GUI packages.

No existing source attribution or license header in the included implementation
files was removed.

### Schematic image added on 2026-09-23

`hardware/images/flight-computer-schematic.svg` is an unmodified KiCad SVG export
of the existing candidate schematic, not a newly invented design or measurement.
Only the README and hardware-status documentation were expanded to present it.
The original schematic's component values and connections were not edited.
The export contains project module-interface symbols and standard KiCad symbols;
it does not distribute manufacturer footprints, 3D models, or editable board CAD.
See [hardware status](hardware/README.md) for the configured ERC result and limits.

## Excluded material

- Raw GPS logs, generated CSV/build outputs, private file paths and local reports.
- Imported Nano/Uno/LoRa dashboard bundles and ESP32 camera projects, which are
  separate projects or third-party baselines, not new flight-core authorship.
- Editable KiCad files, manufacturer-derived footprints, and 3D models pending a complete
  redistribution/attribution review. The private hardware source cites Raspberry
  Pi and Adafruit geometry; those models/designs are not authored wholesale by
  the project owner. A Raspberry Pi STEP model has its own MIT notice, which
  does not license the entire carrier project or this software.
- Private development notes, vendor snapshots, and Git history.

## License status

No project-wide license was found for the included flight-core/protocol code,
and no open-source license has been added during curation. A license selection
and any contributor/third-party permission checks remain the owner's decision
before advertising this as open source. The standard C++/Python libraries and
optional separately installed PySerial package are dependencies, not vendored
source in this repository.
