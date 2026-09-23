#pragma once

#include <cstdint>

#include "rocket/faults.hpp"
#include "rocket/types.hpp"

namespace rocket {

struct TransitionConfig {
  std::uint32_t max_confirmation_gap_ms{250};
  std::uint16_t self_test_samples{10};
  std::uint32_t self_test_timeout_ms{10000};
  float launch_acceleration_g{2.2F};
  float launch_velocity_mps{3.0F};
  float launch_altitude_rise_m{2.0F};
  std::uint16_t launch_samples{3};
  float burnout_acceleration_g{1.3F};
  float burnout_min_velocity_mps{5.0F};
  std::uint32_t burnout_min_time_ms{150};
  std::uint32_t boost_timeout_ms{10000};
  std::uint16_t burnout_samples{5};
  float apogee_velocity_mps{0.5F};
  float apogee_min_altitude_rise_m{20.0F};
  std::uint32_t coast_min_time_ms{300};
  std::uint16_t apogee_samples{5};
  float descent_velocity_mps{-1.5F};
  std::uint32_t apogee_min_time_ms{200};
  std::uint16_t descent_samples{5};
  float landed_velocity_abs_mps{0.6F};
  float landed_altitude_margin_m{8.0F};
  float landed_acceleration_min_g{0.75F};
  float landed_acceleration_max_g{1.25F};
  std::uint32_t descent_min_time_ms{3000};
  std::uint16_t landed_samples{50};
  std::uint16_t critical_sensor_missing_samples{20};
  float low_battery_voltage{3.3F};
};

struct TransitionEvent {
  bool changed{};
  FlightState previous{FlightState::BOOT};
  FlightState current{FlightState::BOOT};
  std::uint32_t timestamp_ms{};
};

class FlightStateMachine {
 public:
  explicit FlightStateMachine(TransitionConfig config = {});

  void initialize(ResetCause reset_cause, const PersistedFlightState* persisted,
                  std::uint32_t now_ms, FaultRegister& faults);
  TransitionEvent update(const FlightData& data, FaultRegister& faults);

  FlightState state() const { return state_; }
  float padAltitudeM() const { return pad_altitude_m_; }
  std::uint32_t stateEnteredMs() const { return state_entered_ms_; }

 private:
  TransitionEvent transitionTo(FlightState next, std::uint32_t now_ms);
  static bool isInFlight(FlightState state);
  void resetConfirmationCounters();
  std::uint32_t elapsed(std::uint32_t now_ms) const;

  TransitionConfig config_;
  FlightState state_{FlightState::BOOT};
  std::uint32_t state_entered_ms_{};
  float pad_altitude_m_{};
  std::uint16_t self_test_count_{};
  std::uint16_t launch_count_{};
  std::uint16_t burnout_count_{};
  std::uint16_t apogee_count_{};
  std::uint16_t descent_count_{};
  std::uint16_t landed_count_{};
  std::uint16_t barometer_missing_count_{};
  std::uint16_t imu_missing_count_{};
  std::uint32_t last_sample_ms_{};
  bool has_last_sample_{};
};

}  // namespace rocket
