#pragma once

#include "climate/application/ClimateCompositeInput.h"
#include "climate/output/OutputExecutionTelemetry.h"

#include <cstdint>

namespace growbox::app::display {

struct DisplaySnapshot final {
  std::uint64_t captured_monotonic_ms{0U};

  bool climate_available{false};
  climate_io::ClimateInputSnapshot climate{};
  climate_io::ClimateWallClockSnapshot clock{};

  bool outputs_available{false};
  output::OutputExecutionTelemetrySnapshot outputs{};

  bool storage_enabled{false};
  bool storage_ready{false};
  const char* firmware_sha{"unknown"};
};

bool buildDisplaySnapshot(
    const climate_io::CompositeClimateSnapshotProvider& climate_provider,
    const output::OutputExecutionTelemetrySnapshot* output_snapshot, bool storage_enabled,
    bool storage_ready, const char* firmware_sha, std::uint64_t captured_monotonic_ms,
    DisplaySnapshot& output) noexcept;

} // namespace growbox::app::display
