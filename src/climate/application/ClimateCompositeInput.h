#pragma once

#include "climate/application/ClimateInputContracts.h"
#include "climate/application/ClimateSnapshotContract.h"

#include <cstdint>

namespace growbox::app::climate_io {

class CompositeClimateSnapshotProvider final : public ClimateSnapshotProvider {
public:
  CompositeClimateSnapshotProvider(InsideEnvironmentSource& inside_source,
                                   OutsideEnvironmentSource& outside_source,
                                   ClimateClockSource& clock_source,
                                   ClimateScheduleConfigSource& schedule_config_source) noexcept
      : inside_source_(inside_source), outside_source_(outside_source), clock_source_(clock_source),
        schedule_config_source_(schedule_config_source) {}

  bool snapshot(std::uint64_t monotonic_ms, ClimateInputSnapshot& output) noexcept override;

  bool hasLastSnapshot() const noexcept {
    return has_last_snapshot_;
  }
  const ClimateInputSnapshot& lastSnapshot() const noexcept {
    return last_snapshot_;
  }
  const ClimateWallClockSnapshot& lastClock() const noexcept {
    return last_clock_;
  }
  std::uint64_t lastSnapshotMonotonicMs() const noexcept {
    return last_snapshot_monotonic_ms_;
  }

private:
  InsideEnvironmentSource& inside_source_;
  OutsideEnvironmentSource& outside_source_;
  ClimateClockSource& clock_source_;
  ClimateScheduleConfigSource& schedule_config_source_;
  ClimateInputSnapshot last_snapshot_{};
  ClimateWallClockSnapshot last_clock_{};
  std::uint64_t last_snapshot_monotonic_ms_{0U};
  bool has_last_snapshot_{false};
};

} // namespace growbox::app::climate_io
