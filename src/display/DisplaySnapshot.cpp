#include "display/DisplaySnapshot.h"

namespace growbox::app::display {

bool buildDisplaySnapshot(
    const climate_io::CompositeClimateSnapshotProvider& climate_provider,
    const output::OutputExecutionTelemetrySnapshot* output_snapshot, bool storage_enabled,
    bool storage_ready, const char* firmware_sha, std::uint64_t captured_monotonic_ms,
    DisplaySnapshot& output) noexcept {
  output = {};
  output.captured_monotonic_ms = captured_monotonic_ms;
  output.storage_enabled = storage_enabled;
  output.storage_ready = storage_ready;
  output.firmware_sha = firmware_sha != nullptr ? firmware_sha : "unknown";

  if (climate_provider.hasLastSnapshot()) {
    output.climate_available = true;
    output.climate = climate_provider.lastSnapshot();
    output.clock = climate_provider.lastClock();
  }

  if (output_snapshot != nullptr) {
    output.outputs_available = true;
    output.outputs = *output_snapshot;
  }

  return output.climate_available || output.outputs_available;
}

} // namespace growbox::app::display
