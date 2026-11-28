#pragma once

#include "climate/ClimateTypes.h"

#include <cstdint>

namespace growbox::app::climate_io {

struct InsideEnvironmentSnapshot {
  ::growbox::climate::MeasuredValue air_temperature_c{};
  ::growbox::climate::MeasuredValue relative_humidity_pct{};
  ::growbox::climate::MeasuredValue co2_ppm{};
};

struct OutsideEnvironmentSnapshot {
  ::growbox::climate::MeasuredValue air_temperature_c{};
  ::growbox::climate::MeasuredValue relative_humidity_pct{};
};

struct ClimateWallClockSnapshot {
  bool valid = false;
  std::uint64_t unix_time_s = 0U;
};

struct ClimateScheduleConfigSnapshot {
  ::growbox::climate::HumidityControlMode humidity_control_mode =
      ::growbox::climate::HumidityControlMode::Rh;
  ::growbox::climate::ClimateTargets targets{};
  ::growbox::climate::ClimateSchedule schedule{};
  ::growbox::climate::ClimateCapabilities capabilities{};
  std::uint64_t sensor_timeout_ms = ::growbox::climate::kDefaultSensorTimeoutMs;
};

class InsideEnvironmentSource {
public:
  virtual ~InsideEnvironmentSource() = default;
  virtual bool sample(std::uint64_t monotonic_ms, InsideEnvironmentSnapshot& output) noexcept = 0;
};

class OutsideEnvironmentSource {
public:
  virtual ~OutsideEnvironmentSource() = default;
  virtual bool sample(std::uint64_t monotonic_ms, OutsideEnvironmentSnapshot& output) noexcept = 0;
};

class ClimateClockSource {
public:
  virtual ~ClimateClockSource() = default;
  virtual bool sample(std::uint64_t monotonic_ms, ClimateWallClockSnapshot& output) noexcept = 0;
};

class ClimateScheduleConfigSource {
public:
  virtual ~ClimateScheduleConfigSource() = default;
  virtual bool resolve(std::uint64_t monotonic_ms, const ClimateWallClockSnapshot& clock,
                       ClimateScheduleConfigSnapshot& output) noexcept = 0;
};

// Composition binds only the independently qualified lamp safety temperature.
class LampSafetyTemperatureSource {
public:
  virtual ~LampSafetyTemperatureSource() = default;
  virtual bool sample(std::uint64_t monotonic_ms,
                      ::growbox::climate::MeasuredValue& temperature_c) noexcept = 0;
};

// Optional independent electrical input; the same physical device can also
// implement an output transport. Power is not an acknowledgement of a command.
struct DeviceElectricalSnapshot {
  std::uint16_t device_id{0U};
  ::growbox::climate::MeasuredValue active_power_w{};
  ::growbox::climate::MeasuredValue energy_wh{};
};

class DeviceElectricalSource {
public:
  virtual ~DeviceElectricalSource() = default;
  virtual bool sampleElectrical(std::uint64_t monotonic_ms,
                                DeviceElectricalSnapshot& output) noexcept = 0;
};

} // namespace growbox::app::climate_io
