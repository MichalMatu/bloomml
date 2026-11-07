#pragma once

#include "climate/application/ClimateCompositeInput.h"
#include "climate/runtime/schedule/RuntimeScheduleProfile.h"

#include <cstdint>

namespace growbox::app::climate_io::runtime {

bool buildConfiguredScheduleProfile(
    std::uint8_t local_hour, const RuntimeScheduleProfile& profile,
    ClimateScheduleConfigSnapshot& output) noexcept;

bool resolveConfiguredScheduleProfile(
    const ClimateWallClockSnapshot& clock, const RuntimeScheduleProfile& profile,
    ClimateScheduleConfigSnapshot& output) noexcept;

// Compatibility entry points; identical Mint defaults for existing callers.
bool buildMintScheduleProfile(std::uint8_t local_hour,
                              ClimateScheduleConfigSnapshot& output) noexcept;

bool resolveMintScheduleProfile(const ClimateWallClockSnapshot& clock,
                                ClimateScheduleConfigSnapshot& output) noexcept;

} // namespace growbox::app::climate_io::runtime
