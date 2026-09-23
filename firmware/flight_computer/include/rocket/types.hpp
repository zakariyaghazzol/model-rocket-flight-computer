#pragma once

#include <cstdint>

namespace rocket {

enum class FlightState : std::uint8_t {
  BOOT = 0,
  SELF_TEST = 1,
  PAD = 2,
  BOOST = 3,
  COAST = 4,
  APOGEE = 5,
  DESCENT = 6,
  LANDED = 7,
  FAULT = 8,
};

enum class ResetCause : std::uint8_t {
  POWER_ON,
  EXTERNAL,
  BROWN_OUT,
  WATCHDOG,
  SOFTWARE,
  UNKNOWN,
};

struct BarometerReading {
  std::uint32_t timestamp_ms{};
  float pressure_pa{};
  float altitude_m{};
  bool valid{};
};

struct ImuReading {
  std::uint32_t timestamp_ms{};
  float accel_x_g{};
  float accel_y_g{};
  float accel_z_g{1.0F};
  float gyro_x_dps{};
  float gyro_y_dps{};
  float gyro_z_dps{};
  bool valid{};
};

struct GpsReading {
  float latitude{};
  float longitude{};
  float altitude_m{};
  std::uint8_t satellites{};
  bool valid{};
};

struct RawSample {
  std::uint32_t timestamp_ms{};
  BarometerReading barometer{};
  ImuReading imu{};
  GpsReading gps{};
  float battery_voltage{};
  bool sd_ok{true};
  bool communications_ok{true};
};

struct FlightData {
  std::uint32_t timestamp_ms{};
  float pressure_pa{};
  float barometric_altitude_m{};
  float vertical_velocity_mps{};
  float accel_x_g{};
  float accel_y_g{};
  float accel_z_g{1.0F};
  float acceleration_magnitude_g{1.0F};
  float gyro_x_dps{};
  float gyro_y_dps{};
  float gyro_z_dps{};
  GpsReading gps{};
  float battery_voltage{};
  bool barometer_valid{};
  bool imu_valid{};
  bool sd_ok{true};
  bool communications_ok{true};
};

struct PersistedFlightState {
  FlightState state{FlightState::BOOT};
  std::uint32_t packet_sequence{};
  std::uint32_t marker{0x524F434BU};
};

const char* to_string(FlightState state);
const char* to_string(ResetCause cause);

}  // namespace rocket
