#include "rocket/flight_computer.hpp"

namespace rocket {

FlightComputer::FlightComputer(IBarometer& barometer, IImu& imu,
                               IDataSink& log_sink,
                               ITelemetryTransport& telemetry_transport,
                               IWatchdog& watchdog,
                               TransitionConfig transitions,
                               FilterConfig filters,
                               std::uint32_t initial_packet_sequence)
    : barometer_(barometer),
      imu_(imu),
      watchdog_(watchdog),
      filter_(filters),
      state_machine_(transitions),
      logger_(log_sink),
      telemetry_(telemetry_transport, initial_packet_sequence) {}

bool FlightComputer::begin(std::uint32_t now_ms, ResetCause reset_cause,
                           const PersistedFlightState* persisted) {
  filter_.reset();
  state_machine_.initialize(reset_cause, persisted, now_ms, faults_);
  const bool barometer_ok = barometer_.begin();
  const bool imu_ok = imu_.begin();
  if (!barometer_ok) faults_.set(FAULT_BAROMETER_MISSING);
  if (!imu_ok) faults_.set(FAULT_IMU_MISSING);
  const bool logger_ok = logger_.begin(faults_);
  const FlightState prior = persisted == nullptr ? FlightState::BOOT : persisted->state;
  logger_.logReset(now_ms, reset_cause, prior, faults_);
  watchdog_.begin(1000U);
  watchdog_.feed();
  started_ = true;
  return barometer_ok && imu_ok && logger_ok && state_machine_.state() != FlightState::FAULT;
}

TransitionEvent FlightComputer::tick(std::uint32_t now_ms,
                                     const AuxiliaryInputs& auxiliary) {
  if (!started_) {
    return {false, FlightState::BOOT, FlightState::BOOT, now_ms};
  }
  RawSample raw{};
  raw.timestamp_ms = now_ms;
  raw.barometer = barometer_.read(now_ms);
  raw.imu = imu_.read(now_ms);
  raw.gps = auxiliary.gps;
  raw.battery_voltage = auxiliary.battery_voltage;
  raw.sd_ok = auxiliary.sd_ok;
  raw.communications_ok = auxiliary.communications_ok;

  latest_data_ = filter_.update(raw, faults_);
  const TransitionEvent event = state_machine_.update(latest_data_, faults_);
  logger_.logSample(latest_data_, state_machine_.state(), faults_, faults_);
  telemetry_.send(latest_data_, state_machine_.state(), faults_, faults_);
  watchdog_.feed();  // Feed only after a complete bounded iteration.
  return event;
}

PersistedFlightState FlightComputer::persistentState() const {
  return {state_machine_.state(), telemetry_.nextSequence(), 0x524F434BU};
}

}  // namespace rocket
