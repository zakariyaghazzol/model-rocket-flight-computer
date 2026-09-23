#include "rocket/types.hpp"

namespace rocket {

const char* to_string(FlightState state) {
  switch (state) {
    case FlightState::BOOT: return "BOOT";
    case FlightState::SELF_TEST: return "SELF_TEST";
    case FlightState::PAD: return "PAD";
    case FlightState::BOOST: return "BOOST";
    case FlightState::COAST: return "COAST";
    case FlightState::APOGEE: return "APOGEE";
    case FlightState::DESCENT: return "DESCENT";
    case FlightState::LANDED: return "LANDED";
    case FlightState::FAULT: return "FAULT";
  }
  return "UNKNOWN";
}

const char* to_string(ResetCause cause) {
  switch (cause) {
    case ResetCause::POWER_ON: return "POWER_ON";
    case ResetCause::EXTERNAL: return "EXTERNAL";
    case ResetCause::BROWN_OUT: return "BROWN_OUT";
    case ResetCause::WATCHDOG: return "WATCHDOG";
    case ResetCause::SOFTWARE: return "SOFTWARE";
    case ResetCause::UNKNOWN: return "UNKNOWN";
  }
  return "UNKNOWN";
}

}  // namespace rocket
