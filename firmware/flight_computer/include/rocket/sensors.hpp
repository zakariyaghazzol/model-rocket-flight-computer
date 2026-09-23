#pragma once

#include <cstddef>
#include <cstdint>

#include "rocket/types.hpp"

namespace rocket {

class IBarometer {
 public:
  virtual ~IBarometer() = default;
  virtual bool begin() = 0;
  virtual BarometerReading read(std::uint32_t now_ms) = 0;
};

class IImu {
 public:
  virtual ~IImu() = default;
  virtual bool begin() = 0;
  virtual ImuReading read(std::uint32_t now_ms) = 0;
};

class IDataSink {
 public:
  virtual ~IDataSink() = default;
  virtual bool begin() = 0;
  virtual bool write(const char* data, std::size_t length) = 0;
  virtual void flush() = 0;
};

// A byte transport only. A platform may connect this to the separate telemetry
// transmitter over a wired link; the flight core has no RFM95 dependency.
class ITelemetryTransport {
 public:
  virtual ~ITelemetryTransport() = default;
  virtual bool send(const std::uint8_t* data, std::size_t length) = 0;
};

class IWatchdog {
 public:
  virtual ~IWatchdog() = default;
  virtual void begin(std::uint32_t timeout_ms) = 0;
  virtual void feed() = 0;
};

}  // namespace rocket
