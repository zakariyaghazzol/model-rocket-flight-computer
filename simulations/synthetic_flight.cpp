#include <cmath>
#include <cstdint>
#include <iostream>

#include "rocket/state_machine.hpp"

namespace {
float altitudeAt(float seconds) {
  if (seconds < 2.0F) return 100.0F;
  if (seconds < 4.0F) {
    const float t = seconds - 2.0F;
    return 100.0F + 30.0F * t * t;
  }
  if (seconds < 8.0F) {
    const float t = seconds - 4.0F;
    return 220.0F + 35.0F * t - 4.375F * t * t;
  }
  if (seconds < 28.0F) return 290.0F - 9.5F * (seconds - 8.0F);
  return 100.0F;
}
}

int main() {
  rocket::FlightStateMachine machine;
  rocket::FaultRegister faults;
  machine.initialize(rocket::ResetCause::POWER_ON, nullptr, 0U, faults);
  std::cout << "timestamp_ms,truth_altitude_m,vertical_velocity_mps,acceleration_g,state,flags\n";
  float previous_altitude = altitudeAt(0.0F);
  for (std::uint32_t time_ms = 0U; time_ms <= 32000U; time_ms += 50U) {
    const float seconds = static_cast<float>(time_ms) / 1000.0F;
    const float noise = seconds < 28.0F ? 0.15F * std::sin(seconds * 17.0F) : 0.0F;
    const float altitude = altitudeAt(seconds) + noise;
    const float velocity = (altitude - previous_altitude) / 0.05F;
    previous_altitude = altitude;
    rocket::FlightData data{};
    data.timestamp_ms = time_ms;
    data.pressure_pa = 101325.0F;
    data.barometric_altitude_m = altitude;
    data.vertical_velocity_mps = velocity;
    data.acceleration_magnitude_g = seconds >= 2.0F && seconds < 4.0F ? 3.5F : 1.0F;
    data.accel_z_g = data.acceleration_magnitude_g;
    data.battery_voltage = 4.0F;
    data.barometer_valid = true;
    data.imu_valid = true;
    machine.update(data, faults);
    std::cout << time_ms << ',' << altitude << ',' << velocity << ','
              << data.acceleration_magnitude_g << ',' << rocket::to_string(machine.state())
              << ',' << faults.value() << '\n';
  }
  return machine.state() == rocket::FlightState::LANDED ? 0 : 1;
}
