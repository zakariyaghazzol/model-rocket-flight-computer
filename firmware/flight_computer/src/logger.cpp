#include "rocket/logger.hpp"

#include <cstdio>

namespace rocket {

namespace {
constexpr char kHeader[] =
    "timestamp_ms,state,pressure_pa,barometric_altitude_m,"
    "vertical_velocity_mps,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,"
    "gyro_y_dps,gyro_z_dps,latitude,longitude,gps_altitude_m,satellites,"
    "battery_voltage,error_flags\n";
}

bool CsvLogger::writeLine(const char* line, int length, FaultRegister& faults) {
  if (length < 0 || static_cast<std::size_t>(length) >= kLineCapacity) {
    faults.set(FAULT_LOG_OVERFLOW);
    return false;
  }
  if (!sink_.write(line, static_cast<std::size_t>(length))) {
    faults.set(FAULT_SD_CARD);
    return false;
  }
  return true;
}

bool CsvLogger::begin(FaultRegister& faults) {
  if (!sink_.begin()) {
    faults.set(FAULT_SD_CARD);
    return false;
  }
  if (!sink_.write(kHeader, sizeof(kHeader) - 1U)) {
    faults.set(FAULT_SD_CARD);
    return false;
  }
  return true;
}

bool CsvLogger::logSample(const FlightData& data, FlightState state,
                          const FaultRegister& faults,
                          FaultRegister& mutable_faults) {
  char line[kLineCapacity]{};
  const int length = std::snprintf(
      line, sizeof(line),
      "%lu,%s,%.1f,%.3f,%.3f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,"
      "%.7f,%.7f,%.2f,%u,%.3f,%lu\n",
      static_cast<unsigned long>(data.timestamp_ms), to_string(state),
      static_cast<double>(data.pressure_pa),
      static_cast<double>(data.barometric_altitude_m),
      static_cast<double>(data.vertical_velocity_mps),
      static_cast<double>(data.accel_x_g), static_cast<double>(data.accel_y_g),
      static_cast<double>(data.accel_z_g), static_cast<double>(data.gyro_x_dps),
      static_cast<double>(data.gyro_y_dps), static_cast<double>(data.gyro_z_dps),
      static_cast<double>(data.gps.latitude),
      static_cast<double>(data.gps.longitude),
      static_cast<double>(data.gps.altitude_m),
      static_cast<unsigned>(data.gps.satellites),
      static_cast<double>(data.battery_voltage),
      static_cast<unsigned long>(faults.value()));
  return writeLine(line, length, mutable_faults);
}

bool CsvLogger::logReset(std::uint32_t timestamp_ms, ResetCause cause,
                         FlightState previous_state, FaultRegister& faults) {
  char line[kLineCapacity]{};
  // Metadata extension: consumers of the sample CSV must skip lines beginning '#'.
  const int length = std::snprintf(
      line, sizeof(line), "#RESET,%lu,%s,%s,%lu\n",
      static_cast<unsigned long>(timestamp_ms), to_string(cause),
      to_string(previous_state), static_cast<unsigned long>(faults.value()));
  return writeLine(line, length, faults);
}

}  // namespace rocket
