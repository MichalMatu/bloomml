#pragma once

#include <cstdint>

namespace growbox::app::climate_io {

enum class ClimateActuatorRole : std::uint8_t {
  Heater = 0U,
  Cooler,
  ExhaustFan,
  Humidifier,
  Dehumidifier,
  Co2Doser,
};

class ClimateRoleDriver {
public:
  virtual ~ClimateRoleDriver() = default;
  virtual bool apply(ClimateActuatorRole role, float level,
                     std::uint64_t monotonic_ms) noexcept = 0;

  // Most drivers are exact: accepted level equals requested level. Binary/dwell
  // arbiters override this to report the state that is actually being held.
  virtual float appliedLevel(ClimateActuatorRole, float requested_level) const noexcept {
    return requested_level;
  }

  // Fail-safe OFF bypasses normal arbitration where supported.
  virtual bool forceSafeOff(ClimateActuatorRole role, std::uint64_t monotonic_ms) noexcept {
    return apply(role, 0.0F, monotonic_ms);
  }
};

} // namespace growbox::app::climate_io
