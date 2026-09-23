#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "rocket/filter.hpp"
#include "rocket/flight_computer.hpp"
#include "rocket/logger.hpp"
#include "rocket/simulated_sensors.hpp"
#include "rocket/state_machine.hpp"
#include "rocket/telemetry.hpp"

namespace {

int failures = 0;

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::cerr << __func__ << ":" << __LINE__ << " CHECK failed: "           \
                << #condition << "\n";                                         \
      ++failures;                                                               \
    }                                                                           \
  } while (false)

rocket::FlightData validData(std::uint32_t time_ms) {
  rocket::FlightData data{};
  data.timestamp_ms = time_ms;
  data.pressure_pa = 101325.0F;
  data.barometric_altitude_m = 100.0F;
  data.vertical_velocity_mps = 0.0F;
  data.accel_z_g = 1.0F;
  data.acceleration_magnitude_g = 1.0F;
  data.battery_voltage = 4.0F;
  data.barometer_valid = true;
  data.imu_valid = true;
  return data;
}

std::uint32_t driveToPad(rocket::FlightStateMachine& machine,
                         rocket::FaultRegister& faults) {
  machine.initialize(rocket::ResetCause::POWER_ON, nullptr, 0U, faults);
  auto data = validData(0U);
  machine.update(data, faults);
  for (unsigned i = 1; i <= 10; ++i) {
    data.timestamp_ms = i * 50U;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::PAD);
  return 500U;
}

void testNominalFlight() {
  rocket::FlightStateMachine machine;
  rocket::FaultRegister faults;
  std::uint32_t time = driveToPad(machine, faults);

  auto data = validData(time);
  data.barometric_altitude_m = 103.0F;
  data.vertical_velocity_mps = 12.0F;
  data.acceleration_magnitude_g = 3.5F;
  for (unsigned i = 0; i < 3; ++i) {
    data.timestamp_ms = time += 50U;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::BOOST);

  time += 200U;
  data.acceleration_magnitude_g = 1.0F;
  data.vertical_velocity_mps = 25.0F;
  for (unsigned i = 0; i < 5; ++i) {
    data.timestamp_ms = time += 50U;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::COAST);

  time += 350U;
  data.barometric_altitude_m = 210.0F;
  data.vertical_velocity_mps = 0.2F;
  for (unsigned i = 0; i < 5; ++i) {
    data.timestamp_ms = time += 50U;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::APOGEE);

  time += 250U;
  data.vertical_velocity_mps = -8.0F;
  for (unsigned i = 0; i < 5; ++i) {
    data.timestamp_ms = time += 50U;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::DESCENT);

  time += 3100U;
  data.barometric_altitude_m = 101.0F;
  data.vertical_velocity_mps = 0.1F;
  data.acceleration_magnitude_g = 1.0F;
  for (unsigned i = 0; i < 50; ++i) {
    data.timestamp_ms = time += 50U;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::LANDED);
}

void testNoiseAndFalseLaunch() {
  rocket::FlightStateMachine machine;
  rocket::FaultRegister faults;
  std::uint32_t time = driveToPad(machine, faults);
  auto data = validData(time);
  for (unsigned i = 0; i < 200; ++i) {
    data.timestamp_ms = time += 50U;
    data.barometric_altitude_m =
        100.0F + static_cast<float>(static_cast<int>(i % 7) - 3) * 0.1F;
    data.vertical_velocity_mps =
        static_cast<float>(static_cast<int>(i % 5) - 2) * 0.2F;
    data.acceleration_magnitude_g = 0.95F + static_cast<float>(i % 3) * 0.05F;
    machine.update(data, faults);
  }
  CHECK(machine.state() == rocket::FlightState::PAD);

  data.timestamp_ms = time += 50U;
  data.barometric_altitude_m = 104.0F;
  data.vertical_velocity_mps = 8.0F;
  data.acceleration_magnitude_g = 4.0F;
  machine.update(data, faults);  // One impulse is deliberately insufficient.
  data.timestamp_ms = time += 50U;
  data.barometric_altitude_m = 100.0F;
  data.vertical_velocity_mps = 0.0F;
  data.acceleration_magnitude_g = 1.0F;
  machine.update(data, faults);
  CHECK(machine.state() == rocket::FlightState::PAD);

  // Two qualifying samples followed by a scheduling gap cannot be combined
  // with a later sample to satisfy the three-sample launch confirmation.
  data.barometric_altitude_m = 104.0F;
  data.vertical_velocity_mps = 8.0F;
  data.acceleration_magnitude_g = 4.0F;
  data.timestamp_ms = time += 50U;
  machine.update(data, faults);
  data.timestamp_ms = time += 50U;
  machine.update(data, faults);
  data.timestamp_ms = time += 500U;
  machine.update(data, faults);
  CHECK(machine.state() == rocket::FlightState::PAD);
}

void testMissingAndFailedSensors() {
  {
    rocket::FlightStateMachine machine;
    rocket::FaultRegister faults;
    std::uint32_t time = driveToPad(machine, faults);
    auto data = validData(time);
    data.barometer_valid = false;
    for (unsigned i = 0; i < 8; ++i) {
      data.timestamp_ms = time += 50U;
      machine.update(data, faults);
    }
    data.barometer_valid = true;
    data.timestamp_ms = time += 50U;
    machine.update(data, faults);
    CHECK(machine.state() == rocket::FlightState::PAD);
    CHECK(faults.has(rocket::FAULT_BAROMETER_MISSING));
  }
  {
    rocket::FlightStateMachine machine;
    rocket::FaultRegister faults;
    std::uint32_t time = driveToPad(machine, faults);
    auto data = validData(time);
    data.barometer_valid = false;
    for (unsigned i = 0; i < 20; ++i) {
      data.timestamp_ms = time += 50U;
      machine.update(data, faults);
    }
    CHECK(machine.state() == rocket::FlightState::FAULT);
  }
  {
    rocket::FlightStateMachine machine;
    rocket::FaultRegister faults;
    std::uint32_t time = driveToPad(machine, faults);
    auto data = validData(time);
    data.imu_valid = false;
    for (unsigned i = 0; i < 20; ++i) {
      data.timestamp_ms = time += 50U;
      machine.update(data, faults);
    }
    CHECK(machine.state() == rocket::FlightState::FAULT);
  }
}

void testNoncriticalFailures() {
  rocket::FlightStateMachine machine;
  rocket::FaultRegister faults;
  std::uint32_t time = driveToPad(machine, faults);
  auto data = validData(time + 50U);
  data.sd_ok = false;
  data.communications_ok = false;
  machine.update(data, faults);
  CHECK(machine.state() == rocket::FlightState::PAD);
  CHECK(faults.has(rocket::FAULT_SD_CARD));
  CHECK(faults.has(rocket::FAULT_COMMUNICATIONS));
}

void testResetDuringFlight() {
  rocket::FlightStateMachine machine;
  rocket::FaultRegister faults;
  const rocket::PersistedFlightState persisted{rocket::FlightState::BOOST, 81U,
                                                0x524F434BU};
  machine.initialize(rocket::ResetCause::WATCHDOG, &persisted, 5000U, faults);
  CHECK(machine.state() == rocket::FlightState::FAULT);
  CHECK(faults.has(rocket::FAULT_RESET_IN_FLIGHT));
  CHECK(faults.has(rocket::FAULT_WATCHDOG_RESET));
}

void testImpossibleMeasurements() {
  rocket::FlightDataFilter filter;
  rocket::FaultRegister faults;
  rocket::RawSample raw{};
  raw.timestamp_ms = 100U;
  raw.barometer = {100U, 101325.0F,
                   std::numeric_limits<float>::quiet_NaN(), true};
  raw.imu = {100U, 99.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, true};
  const auto data = filter.update(raw, faults);
  CHECK(!data.barometer_valid);
  CHECK(!data.imu_valid);
  CHECK(faults.has(rocket::FAULT_BAROMETER_RANGE));
  CHECK(faults.has(rocket::FAULT_IMU_RANGE));
}

class MemorySink final : public rocket::IDataSink {
 public:
  bool begin() override { return begin_ok; }
  bool write(const char* data, std::size_t length) override {
    if (!write_ok) return false;
    contents.append(data, length);
    return true;
  }
  void flush() override {}
  bool begin_ok{true};
  bool write_ok{true};
  std::string contents;
};

class MemoryTransport final : public rocket::ITelemetryTransport {
 public:
  bool send(const std::uint8_t* data, std::size_t length) override {
    if (!send_ok) return false;
    bytes.assign(reinterpret_cast<const char*>(data), length);
    return true;
  }
  bool send_ok{true};
  std::string bytes;
};

class CountingWatchdog final : public rocket::IWatchdog {
 public:
  void begin(std::uint32_t timeout_ms) override { timeout = timeout_ms; }
  void feed() override { ++feeds; }
  std::uint32_t timeout{};
  unsigned feeds{};
};

void testCsvAndSdFailure() {
  MemorySink sink;
  rocket::CsvLogger logger(sink);
  rocket::FaultRegister faults;
  CHECK(logger.begin(faults));
  auto data = validData(123U);
  CHECK(logger.logSample(data, rocket::FlightState::PAD, faults, faults));
  CHECK(sink.contents.find("timestamp_ms,state,pressure_pa") == 0U);
  CHECK(sink.contents.find("123,PAD,") != std::string::npos);
  sink.write_ok = false;
  CHECK(!logger.logSample(data, rocket::FlightState::PAD, faults, faults));
  CHECK(faults.has(rocket::FAULT_SD_CARD));
}

void testTelemetrySequenceAndBounds() {
  rocket::TelemetryEncoder encoder(42U);
  rocket::TelemetryPacket packet;
  rocket::FaultRegister faults;
  auto data = validData(2000U);
  // Artificial protocol fixture, not a recorded location.
  data.gps = {1.125F, -2.25F, 120.0F, 9U, true};
  faults.set(rocket::FAULT_SD_CARD);
  faults.set(rocket::FAULT_COMMUNICATIONS);
  CHECK(encoder.encode(data, rocket::FlightState::COAST, faults, packet, faults));
  CHECK(packet.length < rocket::TelemetryPacket::kCapacity);
  CHECK(std::string(packet.bytes.data(), packet.length) ==
        "FC1,42,2000,4,10000,0,1000,11250000,-22500000,4000,0000000C");
  CHECK(encoder.nextSequence() == 43U);
}

void testStableFlightStateWireMapping() {
  CHECK(rocket::kTelemetryProtocolVersion == 1U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::BOOT) == 0U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::SELF_TEST) == 1U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::PAD) == 2U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::BOOST) == 3U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::COAST) == 4U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::APOGEE) == 5U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::DESCENT) == 6U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::LANDED) == 7U);
  CHECK(rocket::flightStateWireCode(rocket::FlightState::FAULT) == 8U);
}

void testRuntimeWatchdogAndPersistence() {
  rocket::SimulatedBarometer barometer;
  rocket::SimulatedImu imu;
  MemorySink sink;
  MemoryTransport transport;
  CountingWatchdog watchdog;
  rocket::FlightComputer computer(barometer, imu, sink, transport, watchdog,
                                  {}, {}, 77U);
  CHECK(computer.begin(0U, rocket::ResetCause::POWER_ON));
  CHECK(watchdog.timeout == 1000U);
  CHECK(watchdog.feeds == 1U);
  rocket::AuxiliaryInputs inputs;
  inputs.battery_voltage = 4.1F;
  computer.tick(50U, inputs);
  CHECK(watchdog.feeds == 2U);
  CHECK(transport.bytes.rfind("FC1,77,50,", 0U) == 0U);
  CHECK(sink.contents.find("#RESET,0,POWER_ON,BOOT,0") != std::string::npos);
  transport.send_ok = false;
  computer.tick(100U, inputs);
  CHECK(watchdog.feeds == 3U);
  CHECK((computer.faultFlags() & rocket::FAULT_COMMUNICATIONS) != 0U);
  CHECK(computer.state() == rocket::FlightState::SELF_TEST);
  const auto persisted = computer.persistentState();
  CHECK(persisted.packet_sequence == 79U);
  CHECK(persisted.state == rocket::FlightState::SELF_TEST);
}

}  // namespace

int main() {
  testNominalFlight();
  testNoiseAndFalseLaunch();
  testMissingAndFailedSensors();
  testNoncriticalFailures();
  testResetDuringFlight();
  testImpossibleMeasurements();
  testCsvAndSdFailure();
  testTelemetrySequenceAndBounds();
  testStableFlightStateWireMapping();
  testRuntimeWatchdogAndPersistence();
  if (failures == 0) {
    std::cout << "All flight-core tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test assertion(s) failed.\n";
  return 1;
}
