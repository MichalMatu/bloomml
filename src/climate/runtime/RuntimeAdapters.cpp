#include "climate/runtime/RuntimeAdapters.h"

#include "climate/runtime/schedule/ScheduleProfile.h"

#include <array>

namespace growbox::app::climate_io::runtime {

bool LockedFakeRoleDriver::apply(ClimateActuatorRole, float, std::uint64_t) noexcept {
  return true;
}

bool Tp357LampSafetySource::sample(
    std::uint64_t monotonic_ms, ::growbox::climate::MeasuredValue& temperature_c) noexcept {
  temperature_c = {};
  native::BleClimateReading tp357{};
  if (!ble_.sampleTp357(monotonic_ms, tp357)) return false;
  temperature_c = {tp357.temperature_c, true, tp357.age_ms};
  return true;
}

namespace {

::growbox::climate::MeasuredValue selectedMeasuredValue(
    const ::growbox::ipo::SignalSelection& selection) noexcept {
  if (!selection.usable()) return {};
  return {static_cast<float>(selection.value), true, selection.age_ms};
}

std::uint64_t sensorTimeout(const RuntimeScheduleProfileOwner* profile) noexcept {
  return profile != nullptr && profile->valid()
             ? profile->active().sensor_timeout_ms
             : ::growbox::climate::kDefaultSensorTimeoutMs;
}

} // namespace

bool RuntimeInsideSource::sample(std::uint64_t monotonic_ms,
                                 InsideEnvironmentSnapshot& output) noexcept {
  output = {};
  const auto& frame = inputs_.sample(monotonic_ms);
  const auto timeout_ms = sensorTimeout(&profile_);
  using ::growbox::ipo::SignalRole;
  last_selections_[0] = frame.select(SignalRole::InsideTemperature, timeout_ms);
  last_selections_[1] = frame.select(SignalRole::InsideHumidity, timeout_ms);
  last_selections_[2] = frame.select(SignalRole::InsideCo2, timeout_ms);
  output.air_temperature_c = selectedMeasuredValue(last_selections_[0]);
  output.relative_humidity_pct = selectedMeasuredValue(last_selections_[1]);
  output.co2_ppm = selectedMeasuredValue(last_selections_[2]);
  return output.air_temperature_c.valid || output.relative_humidity_pct.valid ||
         output.co2_ppm.valid;
}

bool RuntimeNearbySource::sample(std::uint64_t monotonic_ms,
                                 OutsideEnvironmentSnapshot& output) noexcept {
  output = {};
  const auto& frame = inputs_.sample(monotonic_ms);
  const auto timeout_ms = sensorTimeout(&profile_);
  using ::growbox::ipo::SignalRole;
  last_selections_[0] = frame.select(SignalRole::OutsideTemperature, timeout_ms);
  last_selections_[1] = frame.select(SignalRole::OutsideHumidity, timeout_ms);
  output.air_temperature_c = selectedMeasuredValue(last_selections_[0]);
  output.relative_humidity_pct = selectedMeasuredValue(last_selections_[1]);
  return output.air_temperature_c.valid || output.relative_humidity_pct.valid;
}

bool ConfiguredScheduleConfigSource::resolve(
    std::uint64_t, const ClimateWallClockSnapshot& clock,
    ClimateScheduleConfigSnapshot& output) noexcept {
  if (!resolveConfiguredScheduleProfile(clock, owner_.active(), output)) return false;
  output.capabilities = capabilities_;
  return true;
}

bool MintScheduleConfigSource::resolve(std::uint64_t, const ClimateWallClockSnapshot& clock,
                                       ClimateScheduleConfigSnapshot& output) noexcept {
  return resolveMintScheduleProfile(clock, output);
}

} // namespace growbox::app::climate_io::runtime
