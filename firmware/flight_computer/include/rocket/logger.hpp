#pragma once

#include <cstddef>
#include <cstdint>

#include "rocket/faults.hpp"
#include "rocket/sensors.hpp"
#include "rocket/types.hpp"

namespace rocket {

class CsvLogger {
 public:
  static constexpr std::size_t kLineCapacity = 384U;
  explicit CsvLogger(IDataSink& sink) : sink_(sink) {}

  bool begin(FaultRegister& faults);
  bool logSample(const FlightData& data, FlightState state,
                 const FaultRegister& faults, FaultRegister& mutable_faults);
  bool logReset(std::uint32_t timestamp_ms, ResetCause cause,
                FlightState previous_state, FaultRegister& faults);
  void flush() { sink_.flush(); }

 private:
  bool writeLine(const char* line, int length, FaultRegister& faults);
  IDataSink& sink_;
};

}  // namespace rocket
