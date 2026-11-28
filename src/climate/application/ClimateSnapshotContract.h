#pragma once

#include "climate/ClimateTypes.h"

#include <cstdint>

namespace growbox::app::climate_io {

struct ClimateInputSnapshot {
  ::growbox::climate::ClimateMeasurements measurements{};
  ::growbox::climate::HumidityControlMode humidity_control_mode =
      ::growbox::climate::HumidityControlMode::Rh;
  ::growbox::climate::ClimateTargets targets{};
  ::growbox::climate::ClimateSchedule schedule{};
  ::growbox::climate::ClimateCapabilities capabilities{};
  std::uint64_t sensor_timeout_ms = ::growbox::climate::kDefaultSensorTimeoutMs;
};

class ClimateSnapshotProvider {
public:
  virtual ~ClimateSnapshotProvider() = default;
  virtual bool snapshot(std::uint64_t monotonic_ms, ClimateInputSnapshot& output) noexcept = 0;
};

} // namespace growbox::app::climate_io
