#pragma once

#include "climate/application/ClimateCompositeInput.h"
#include "climate/output/OutputIntents.h"
#include "climate/runtime/schedule/RuntimeScheduleProfile.h"

#include <cstdint>

namespace growbox::app::climate_io::runtime {

bool buildScheduleIntent(std::uint64_t monotonic_ms, const ClimateWallClockSnapshot& clock,
                         std::uint64_t sequence,
                         ::growbox::app::output::ScheduleIntent& output) noexcept;

// The real-input coordinator uses the SAME accepted profile for schedule and Rule.
bool buildScheduleIntent(std::uint64_t monotonic_ms, const ClimateWallClockSnapshot& clock,
                         std::uint64_t sequence, const RuntimeScheduleProfile& profile,
                         ::growbox::app::output::ScheduleIntent& output) noexcept;

} // namespace growbox::app::climate_io::runtime
