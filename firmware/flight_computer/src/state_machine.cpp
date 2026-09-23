#include "rocket/state_machine.hpp"

#include <algorithm>
#include <cmath>

namespace rocket {

namespace {
void confirm(bool condition, std::uint16_t& counter) {
  counter = condition ? static_cast<std::uint16_t>(
                            std::min<unsigned>(counter + 1U, 0xFFFFU))
                      : 0U;
}
}  // namespace

FlightStateMachine::FlightStateMachine(TransitionConfig config) : config_(config) {}

bool FlightStateMachine::isInFlight(FlightState state) {
  return state == FlightState::BOOST || state == FlightState::COAST ||
         state == FlightState::APOGEE || state == FlightState::DESCENT;
}

void FlightStateMachine::initialize(ResetCause reset_cause,
                                    const PersistedFlightState* persisted,
                                    std::uint32_t now_ms,
                                    FaultRegister& faults) {
  state_ = FlightState::BOOT;
  state_entered_ms_ = now_ms;
  resetConfirmationCounters();
  barometer_missing_count_ = 0U;
  imu_missing_count_ = 0U;
  last_sample_ms_ = now_ms;
  has_last_sample_ = false;
  if (reset_cause == ResetCause::WATCHDOG) faults.set(FAULT_WATCHDOG_RESET);
  if (persisted != nullptr && persisted->marker == 0x524F434BU &&
      isInFlight(persisted->state)) {
    faults.set(FAULT_RESET_IN_FLIGHT);
    state_ = FlightState::FAULT;
  }
}

std::uint32_t FlightStateMachine::elapsed(std::uint32_t now_ms) const {
  return now_ms - state_entered_ms_;  // Unsigned arithmetic tolerates millis() wrap.
}

void FlightStateMachine::resetConfirmationCounters() {
  self_test_count_ = 0U;
  launch_count_ = 0U;
  burnout_count_ = 0U;
  apogee_count_ = 0U;
  descent_count_ = 0U;
  landed_count_ = 0U;
}

TransitionEvent FlightStateMachine::transitionTo(FlightState next,
                                                  std::uint32_t now_ms) {
  const FlightState previous = state_;
  state_ = next;
  state_entered_ms_ = now_ms;
  resetConfirmationCounters();
  return {true, previous, next, now_ms};
}

TransitionEvent FlightStateMachine::update(const FlightData& data,
                                            FaultRegister& faults) {
  TransitionEvent unchanged{false, state_, state_, data.timestamp_ms};
  if (state_ == FlightState::FAULT || state_ == FlightState::LANDED) return unchanged;

  if (has_last_sample_) {
    const std::uint32_t sample_delta_ms = data.timestamp_ms - last_sample_ms_;
    if (sample_delta_ms > 0x80000000U) {
      faults.set(FAULT_TIME_REGRESSION);
      return unchanged;
    }
    if (sample_delta_ms > config_.max_confirmation_gap_ms) {
      resetConfirmationCounters();
    }
  }
  last_sample_ms_ = data.timestamp_ms;
  has_last_sample_ = true;

  confirm(!data.barometer_valid, barometer_missing_count_);
  confirm(!data.imu_valid, imu_missing_count_);
  if (!data.barometer_valid) faults.set(FAULT_BAROMETER_MISSING);
  if (!data.imu_valid) faults.set(FAULT_IMU_MISSING);
  if (!data.sd_ok) faults.set(FAULT_SD_CARD);
  if (!data.communications_ok) faults.set(FAULT_COMMUNICATIONS);
  if (data.battery_voltage > 0.1F &&
      data.battery_voltage < config_.low_battery_voltage) {
    faults.set(FAULT_LOW_BATTERY);
  }

  if (barometer_missing_count_ >= config_.critical_sensor_missing_samples ||
      imu_missing_count_ >= config_.critical_sensor_missing_samples) {
    return transitionTo(FlightState::FAULT, data.timestamp_ms);
  }

  switch (state_) {
    case FlightState::BOOT:
      return transitionTo(FlightState::SELF_TEST, data.timestamp_ms);

    case FlightState::SELF_TEST: {
      confirm(data.barometer_valid && data.imu_valid, self_test_count_);
      if (self_test_count_ >= config_.self_test_samples) {
        pad_altitude_m_ = data.barometric_altitude_m;
        return transitionTo(FlightState::PAD, data.timestamp_ms);
      }
      if (elapsed(data.timestamp_ms) >= config_.self_test_timeout_ms) {
        faults.set(FAULT_SELF_TEST_TIMEOUT);
        return transitionTo(FlightState::FAULT, data.timestamp_ms);
      }
      break;
    }

    case FlightState::PAD: {
      if (data.barometer_valid) {
        // Slow pad-reference tracking removes weather drift but freezes at launch.
        pad_altitude_m_ += 0.005F * (data.barometric_altitude_m - pad_altitude_m_);
      }
      const bool launch = data.barometer_valid && data.imu_valid &&
          data.acceleration_magnitude_g >= config_.launch_acceleration_g &&
          data.vertical_velocity_mps >= config_.launch_velocity_mps &&
          data.barometric_altitude_m - pad_altitude_m_ >= config_.launch_altitude_rise_m;
      confirm(launch, launch_count_);
      if (launch_count_ >= config_.launch_samples) {
        return transitionTo(FlightState::BOOST, data.timestamp_ms);
      }
      break;
    }

    case FlightState::BOOST: {
      const bool burnout = elapsed(data.timestamp_ms) >= config_.burnout_min_time_ms &&
          data.barometer_valid && data.imu_valid &&
          data.acceleration_magnitude_g <= config_.burnout_acceleration_g &&
          data.vertical_velocity_mps >= config_.burnout_min_velocity_mps;
      confirm(burnout, burnout_count_);
      if (burnout_count_ >= config_.burnout_samples ||
          elapsed(data.timestamp_ms) >= config_.boost_timeout_ms) {
        return transitionTo(FlightState::COAST, data.timestamp_ms);
      }
      break;
    }

    case FlightState::COAST: {
      const bool apogee = elapsed(data.timestamp_ms) >= config_.coast_min_time_ms &&
          data.barometer_valid &&
          data.barometric_altitude_m - pad_altitude_m_ >=
              config_.apogee_min_altitude_rise_m &&
          data.vertical_velocity_mps <= config_.apogee_velocity_mps;
      confirm(apogee, apogee_count_);
      if (apogee_count_ >= config_.apogee_samples) {
        return transitionTo(FlightState::APOGEE, data.timestamp_ms);
      }
      break;
    }

    case FlightState::APOGEE: {
      const bool descending = elapsed(data.timestamp_ms) >= config_.apogee_min_time_ms &&
          data.barometer_valid &&
          data.vertical_velocity_mps <= config_.descent_velocity_mps;
      confirm(descending, descent_count_);
      if (descent_count_ >= config_.descent_samples) {
        return transitionTo(FlightState::DESCENT, data.timestamp_ms);
      }
      break;
    }

    case FlightState::DESCENT: {
      const bool landed = elapsed(data.timestamp_ms) >= config_.descent_min_time_ms &&
          data.barometer_valid && data.imu_valid &&
          std::fabs(data.vertical_velocity_mps) <= config_.landed_velocity_abs_mps &&
          data.barometric_altitude_m <=
              pad_altitude_m_ + config_.landed_altitude_margin_m &&
          data.acceleration_magnitude_g >= config_.landed_acceleration_min_g &&
          data.acceleration_magnitude_g <= config_.landed_acceleration_max_g;
      confirm(landed, landed_count_);
      if (landed_count_ >= config_.landed_samples) {
        return transitionTo(FlightState::LANDED, data.timestamp_ms);
      }
      break;
    }

    case FlightState::LANDED:
    case FlightState::FAULT:
      break;
  }
  return unchanged;
}

}  // namespace rocket
