#pragma once

#include <array>
#include <cstddef>

#include "rocket/faults.hpp"
#include "rocket/types.hpp"

namespace rocket {

struct FilterConfig {
  float altitude_alpha{0.25F};
  float velocity_alpha{0.20F};
  float min_pressure_pa{1000.0F};
  float max_pressure_pa{120000.0F};
  float min_altitude_m{-500.0F};
  float max_altitude_m{50000.0F};
  float max_acceleration_g{32.0F};
  float max_gyro_dps{4000.0F};
};

class FlightDataFilter {
 public:
  explicit FlightDataFilter(FilterConfig config = {});
  FlightData update(const RawSample& raw, FaultRegister& faults);
  void reset();

 private:
  float medianAltitude() const;
  bool finiteAndInRange(float value, float minimum, float maximum) const;

  FilterConfig config_;
  std::array<float, 5> altitude_window_{};
  std::size_t altitude_count_{};
  std::size_t altitude_index_{};
  float filtered_altitude_{};
  float filtered_velocity_{};
  std::uint32_t previous_time_ms_{};
  bool initialized_{};
};

}  // namespace rocket
