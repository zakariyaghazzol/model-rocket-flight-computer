#pragma once

#include <cstdint>

#include "rocket/filter.hpp"
#include "rocket/logger.hpp"
#include "rocket/sensors.hpp"
#include "rocket/state_machine.hpp"
#include "rocket/telemetry.hpp"

namespace rocket {

struct AuxiliaryInputs {
  GpsReading gps{};
  float battery_voltage{};
  bool sd_ok{true};
  bool communications_ok{true};
};

class FlightComputer {
 public:
  FlightComputer(IBarometer& barometer, IImu& imu, IDataSink& log_sink,
                 ITelemetryTransport& telemetry_transport, IWatchdog& watchdog,
                 TransitionConfig transitions = {}, FilterConfig filters = {},
                 std::uint32_t initial_packet_sequence = 0U);

  bool begin(std::uint32_t now_ms, ResetCause reset_cause,
             const PersistedFlightState* persisted = nullptr);
  TransitionEvent tick(std::uint32_t now_ms, const AuxiliaryInputs& auxiliary);
  PersistedFlightState persistentState() const;

  FlightState state() const { return state_machine_.state(); }
  std::uint32_t faultFlags() const { return faults_.value(); }
  const FlightData& latestData() const { return latest_data_; }

 private:
  IBarometer& barometer_;
  IImu& imu_;
  IWatchdog& watchdog_;
  FlightDataFilter filter_;
  FlightStateMachine state_machine_;
  CsvLogger logger_;
  TelemetryLink telemetry_;
  FaultRegister faults_;
  FlightData latest_data_{};
  bool started_{};
};

}  // namespace rocket
