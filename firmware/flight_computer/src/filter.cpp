#include "rocket/filter.hpp"

#include <algorithm>
#include <cmath>

namespace rocket {

FlightDataFilter::FlightDataFilter(FilterConfig config) : config_(config) {}

bool FlightDataFilter::finiteAndInRange(float value, float minimum,
                                        float maximum) const {
  return std::isfinite(value) && value >= minimum && value <= maximum;
}

float FlightDataFilter::medianAltitude() const {
  auto values = altitude_window_;
  std::sort(values.begin(), values.begin() + altitude_count_);
  return values[altitude_count_ / 2U];
}

FlightData FlightDataFilter::update(const RawSample& raw, FaultRegister& faults) {
  FlightData output{};
  output.timestamp_ms = raw.timestamp_ms;
  output.gps = raw.gps;
  output.battery_voltage = raw.battery_voltage;
  output.sd_ok = raw.sd_ok;
  output.communications_ok = raw.communications_ok;

  const auto& baro = raw.barometer;
  output.barometer_valid = baro.valid &&
      finiteAndInRange(baro.pressure_pa, config_.min_pressure_pa, config_.max_pressure_pa) &&
      finiteAndInRange(baro.altitude_m, config_.min_altitude_m, config_.max_altitude_m);
  if (baro.valid && !output.barometer_valid) faults.set(FAULT_BAROMETER_RANGE);
  if (output.barometer_valid) {
    output.pressure_pa = baro.pressure_pa;
    altitude_window_[altitude_index_] = baro.altitude_m;
    altitude_index_ = (altitude_index_ + 1U) % altitude_window_.size();
    altitude_count_ = std::min(altitude_count_ + 1U, altitude_window_.size());
    const float median = medianAltitude();
    if (!initialized_) {
      filtered_altitude_ = median;
      previous_time_ms_ = raw.timestamp_ms;
      initialized_ = true;
    } else if (raw.timestamp_ms > previous_time_ms_) {
      const float dt = static_cast<float>(raw.timestamp_ms - previous_time_ms_) / 1000.0F;
      const float previous_altitude = filtered_altitude_;
      filtered_altitude_ += config_.altitude_alpha * (median - filtered_altitude_);
      const float raw_velocity = (filtered_altitude_ - previous_altitude) / dt;
      filtered_velocity_ += config_.velocity_alpha * (raw_velocity - filtered_velocity_);
      previous_time_ms_ = raw.timestamp_ms;
    } else if (raw.timestamp_ms < previous_time_ms_) {
      faults.set(FAULT_TIME_REGRESSION);
    }
  }
  output.barometric_altitude_m = filtered_altitude_;
  output.vertical_velocity_mps = filtered_velocity_;

  const auto& imu = raw.imu;
  output.imu_valid = imu.valid &&
      finiteAndInRange(imu.accel_x_g, -config_.max_acceleration_g, config_.max_acceleration_g) &&
      finiteAndInRange(imu.accel_y_g, -config_.max_acceleration_g, config_.max_acceleration_g) &&
      finiteAndInRange(imu.accel_z_g, -config_.max_acceleration_g, config_.max_acceleration_g) &&
      finiteAndInRange(imu.gyro_x_dps, -config_.max_gyro_dps, config_.max_gyro_dps) &&
      finiteAndInRange(imu.gyro_y_dps, -config_.max_gyro_dps, config_.max_gyro_dps) &&
      finiteAndInRange(imu.gyro_z_dps, -config_.max_gyro_dps, config_.max_gyro_dps);
  if (imu.valid && !output.imu_valid) faults.set(FAULT_IMU_RANGE);
  if (output.imu_valid) {
    output.accel_x_g = imu.accel_x_g;
    output.accel_y_g = imu.accel_y_g;
    output.accel_z_g = imu.accel_z_g;
    output.gyro_x_dps = imu.gyro_x_dps;
    output.gyro_y_dps = imu.gyro_y_dps;
    output.gyro_z_dps = imu.gyro_z_dps;
    output.acceleration_magnitude_g = std::sqrt(
        imu.accel_x_g * imu.accel_x_g + imu.accel_y_g * imu.accel_y_g +
        imu.accel_z_g * imu.accel_z_g);
  }
  return output;
}

void FlightDataFilter::reset() {
  altitude_window_.fill(0.0F);
  altitude_count_ = 0U;
  altitude_index_ = 0U;
  filtered_altitude_ = 0.0F;
  filtered_velocity_ = 0.0F;
  previous_time_ms_ = 0U;
  initialized_ = false;
}

}  // namespace rocket
