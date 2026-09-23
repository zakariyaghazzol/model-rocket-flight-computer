#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "rocket/faults.hpp"
#include "rocket/sensors.hpp"
#include "rocket/types.hpp"

namespace rocket {

constexpr std::uint8_t kTelemetryProtocolVersion = 1U;
constexpr char kTelemetryProtocolId[] = "FC1";

// Stable FC1 wire mapping. Keep this explicit even if FlightState is reordered.
std::uint8_t flightStateWireCode(FlightState state);

struct TelemetryPacket {
  static constexpr std::size_t kCapacity = 128U;
  std::array<char, kCapacity> bytes{};
  std::size_t length{};
};

class TelemetryEncoder {
 public:
  explicit TelemetryEncoder(std::uint32_t initial_sequence = 0U)
      : sequence_(initial_sequence) {}

  bool encode(const FlightData& data, FlightState state,
              const FaultRegister& faults, TelemetryPacket& packet,
              FaultRegister& mutable_faults);
  std::uint32_t nextSequence() const { return sequence_; }

 private:
  std::uint32_t sequence_{};
};

class TelemetryLink {
 public:
  explicit TelemetryLink(ITelemetryTransport& transport,
                         std::uint32_t initial_sequence = 0U)
      : transport_(transport), encoder_(initial_sequence) {}

  bool send(const FlightData& data, FlightState state,
            const FaultRegister& faults, FaultRegister& mutable_faults);
  std::uint32_t nextSequence() const { return encoder_.nextSequence(); }

 private:
  ITelemetryTransport& transport_;
  TelemetryEncoder encoder_;
};

}  // namespace rocket
