#include "climate/runtime/schedule/ScheduleProfile.h"

#include "climate/runtime/schedule/EuropeWarsawTime.h"

namespace growbox::app::climate_io::runtime {

bool buildConfiguredScheduleProfile(std::uint8_t local_hour,
                                    const RuntimeScheduleProfile& profile,
                                    ClimateScheduleConfigSnapshot& output) noexcept {
  return projectRuntimeSchedule(local_hour, profile, output);
}

bool resolveConfiguredScheduleProfile(const ClimateWallClockSnapshot& clock,
                                      const RuntimeScheduleProfile& profile,
                                      ClimateScheduleConfigSnapshot& output) noexcept {
  output = {};
  if (!clock.valid || !validateRuntimeScheduleProfile(profile)) return false;
  EuropeWarsawLocalTime local{};
  if (!resolveEuropeWarsawLocalTime(clock.unix_time_s, local)) return false;
  return buildConfiguredScheduleProfile(local.hour, profile, output);
}

bool buildMintScheduleProfile(std::uint8_t local_hour,
                              ClimateScheduleConfigSnapshot& output) noexcept {
  return buildConfiguredScheduleProfile(local_hour, makeMintRuntimeScheduleProfile(), output);
}

bool resolveMintScheduleProfile(const ClimateWallClockSnapshot& clock,
                                ClimateScheduleConfigSnapshot& output) noexcept {
  return resolveConfiguredScheduleProfile(clock, makeMintRuntimeScheduleProfile(), output);
}

} // namespace growbox::app::climate_io::runtime
