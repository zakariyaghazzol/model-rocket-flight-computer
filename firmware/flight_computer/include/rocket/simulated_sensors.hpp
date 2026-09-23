#pragma once

#include "rocket/sensors.hpp"

namespace rocket {

// Deterministic simulated sources for host tests and hardware-independent bring-up.
class SimulatedBarometer final : public IBarometer {
 public:
  bool begin() override { return begin_ok_; }
  BarometerReading read(std::uint32_t now_ms) override {
    reading_.timestamp_ms = now_ms;
    return reading_;
  }
  void set(float pressure_pa, float altitude_m, bool valid = true) {
    reading_.pressure_pa = pressure_pa;
    reading_.altitude_m = altitude_m;
    reading_.valid = valid;
  }
  void setBeginOk(bool value) { begin_ok_ = value; }

 private:
  BarometerReading reading_{0U, 101325.0F, 0.0F, true};
  bool begin_ok_{true};
};

class SimulatedImu final : public IImu {
 public:
  bool begin() override { return begin_ok_; }
  ImuReading read(std::uint32_t now_ms) override {
    reading_.timestamp_ms = now_ms;
    return reading_;
  }
  void setAcceleration(float x_g, float y_g, float z_g, bool valid = true) {
    reading_.accel_x_g = x_g;
    reading_.accel_y_g = y_g;
    reading_.accel_z_g = z_g;
    reading_.valid = valid;
  }
  void setGyro(float x_dps, float y_dps, float z_dps) {
    reading_.gyro_x_dps = x_dps;
    reading_.gyro_y_dps = y_dps;
    reading_.gyro_z_dps = z_dps;
  }
  void setBeginOk(bool value) { begin_ok_ = value; }

 private:
  ImuReading reading_{};
  bool begin_ok_{true};
};

}  // namespace rocket
