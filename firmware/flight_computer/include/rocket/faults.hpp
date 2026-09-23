#pragma once

#include <cstdint>

namespace rocket {

enum FaultFlag : std::uint32_t {
  FAULT_NONE = 0,
  FAULT_BAROMETER_MISSING = 1UL << 0,
  FAULT_IMU_MISSING = 1UL << 1,
  FAULT_SD_CARD = 1UL << 2,
  FAULT_COMMUNICATIONS = 1UL << 3,
  FAULT_LOW_BATTERY = 1UL << 4,
  FAULT_BAROMETER_RANGE = 1UL << 5,
  FAULT_IMU_RANGE = 1UL << 6,
  FAULT_TIME_REGRESSION = 1UL << 7,
  FAULT_SELF_TEST_TIMEOUT = 1UL << 8,
  FAULT_RESET_IN_FLIGHT = 1UL << 9,
  FAULT_WATCHDOG_RESET = 1UL << 10,
  FAULT_LOG_OVERFLOW = 1UL << 11,
  FAULT_TELEMETRY_OVERFLOW = 1UL << 12,
};

class FaultRegister {
 public:
  void set(FaultFlag flag) { flags_ |= static_cast<std::uint32_t>(flag); }
  void clear(FaultFlag flag) { flags_ &= ~static_cast<std::uint32_t>(flag); }
  bool has(FaultFlag flag) const {
    return (flags_ & static_cast<std::uint32_t>(flag)) != 0U;
  }
  std::uint32_t value() const { return flags_; }
  void restore(std::uint32_t flags) { flags_ = flags; }

 private:
  std::uint32_t flags_{};
};

}  // namespace rocket
