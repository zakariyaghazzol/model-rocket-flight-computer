#include "rocket/telemetry.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

namespace rocket {

std::uint8_t flightStateWireCode(FlightState state) {
  switch (state) {
    case FlightState::BOOT: return 0U;
    case FlightState::SELF_TEST: return 1U;
    case FlightState::PAD: return 2U;
    case FlightState::BOOST: return 3U;
    case FlightState::COAST: return 4U;
    case FlightState::APOGEE: return 5U;
    case FlightState::DESCENT: return 6U;
    case FlightState::LANDED: return 7U;
    case FlightState::FAULT: return 8U;
  }
  return 8U;
}

namespace {
long scaled(float value, float scale) {
  const double result = std::round(static_cast<double>(value) * scale);
  if (result > static_cast<double>(std::numeric_limits<long>::max()))
    return std::numeric_limits<long>::max();
  if (result < static_cast<double>(std::numeric_limits<long>::min()))
    return std::numeric_limits<long>::min();
  return static_cast<long>(result);
}
}

bool TelemetryEncoder::encode(const FlightData& data, FlightState state,
                              const FaultRegister& faults,
                              TelemetryPacket& packet,
                              FaultRegister& mutable_faults) {
  // FC1,packet,time_ms,state,alt_cm,vvel_cms,accel_mg,lat_e7,lon_e7,batt_mv,flags
  const long latitude_e7 =
      data.gps.valid ? scaled(data.gps.latitude, 10000000.0F) : 0L;
  const long longitude_e7 =
      data.gps.valid ? scaled(data.gps.longitude, 10000000.0F) : 0L;
  const int length = std::snprintf(
      packet.bytes.data(), packet.bytes.size(),
      "%s,%lu,%lu,%u,%ld,%ld,%ld,%ld,%ld,%ld,%08lX",
      kTelemetryProtocolId,
      static_cast<unsigned long>(sequence_),
      static_cast<unsigned long>(data.timestamp_ms),
      static_cast<unsigned>(flightStateWireCode(state)),
      scaled(data.barometric_altitude_m, 100.0F),
      scaled(data.vertical_velocity_mps, 100.0F),
      scaled(data.acceleration_magnitude_g, 1000.0F),
      latitude_e7, longitude_e7,
      scaled(data.battery_voltage, 1000.0F),
      static_cast<unsigned long>(faults.value()));
  ++sequence_;  // Sequence counts attempted packets, exposing link loss.
  if (length < 0 || static_cast<std::size_t>(length) >= packet.bytes.size()) {
    mutable_faults.set(FAULT_TELEMETRY_OVERFLOW);
    packet.length = 0U;
    return false;
  }
  packet.length = static_cast<std::size_t>(length);
  return true;
}

bool TelemetryLink::send(const FlightData& data, FlightState state,
                         const FaultRegister& faults,
                         FaultRegister& mutable_faults) {
  TelemetryPacket packet{};
  if (!encoder_.encode(data, state, faults, packet, mutable_faults)) return false;
  if (!transport_.send(
          reinterpret_cast<const std::uint8_t*>(packet.bytes.data()),
          packet.length)) {
    mutable_faults.set(FAULT_COMMUNICATIONS);
    return false;
  }
  return true;
}

}  // namespace rocket
