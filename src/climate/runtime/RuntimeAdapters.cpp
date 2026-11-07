#include "climate/runtime/RuntimeAdapters.h"

#include "climate/runtime/schedule/ScheduleProfile.h"

#include <array>

namespace growbox::app::climate_io::runtime {

bool LockedFakeRoleDriver::apply(ClimateActuatorRole, float, std::uint64_t) noexcept {
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

RuntimeInsideSource::RuntimeInsideSource(
    native::BleClimateScanner& ble, native::Scd41InsideSource& scd41,
    const ::growbox::ipo::Graph* graph,
    const RuntimeScheduleProfileOwner* profile) noexcept
    : ble_(ble), scd41_(scd41), graph_(graph), profile_(profile) {}

bool RuntimeInsideSource::sample(std::uint64_t monotonic_ms,
                                 InsideEnvironmentSnapshot& output) noexcept {
  output = {};
  native::BleClimateReading tp357{};
  const bool tp357_sampled = ble_.sampleTp357(monotonic_ms, tp357);
  InsideEnvironmentSnapshot scd41{};
  const bool scd41_sampled = scd41_.sample(monotonic_ms, scd41) && scd41.co2_ppm.valid;

  if (graph_ == nullptr) {
    // Backward-compatible path for existing, isolated host fixtures.
    if (tp357_sampled) {
      output.air_temperature_c = {tp357.temperature_c, true, tp357.age_ms};
      output.relative_humidity_pct = {tp357.relative_humidity_pct, true, tp357.age_ms};
    }
    if (scd41_sampled) output.co2_ppm = scd41.co2_ppm;
    return tp357_sampled || scd41_sampled;
  }

  // The currently compiled CrowPanel graph admits exactly these drivers.
  // SCD41 T/RH are intentionally NOT a qualified fallback for lamp safety.
  const std::array<::growbox::ipo::SignalReading, 3U> readings{{
      {1U, ::growbox::ipo::Channel::Temperature, tp357.temperature_c, tp357.age_ms,
       tp357_sampled},
      {1U, ::growbox::ipo::Channel::Humidity, tp357.relative_humidity_pct, tp357.age_ms,
       tp357_sampled},
      {2U, ::growbox::ipo::Channel::Co2, scd41.co2_ppm.value, scd41.co2_ppm.age_ms,
       scd41_sampled},
  }};
  const auto timeout_ms = sensorTimeout(profile_);
  last_selections_[0] = ::growbox::ipo::selectFreshSignal(
      *graph_, ::growbox::ipo::SignalRole::InsideTemperature,
      readings.data(), readings.size(), timeout_ms);
  output.air_temperature_c = selectedMeasuredValue(last_selections_[0]);
  last_selections_[1] = ::growbox::ipo::selectFreshSignal(
      *graph_, ::growbox::ipo::SignalRole::InsideHumidity,
      readings.data(), readings.size(), timeout_ms);
  output.relative_humidity_pct = selectedMeasuredValue(last_selections_[1]);
  last_selections_[2] = ::growbox::ipo::selectFreshSignal(
      *graph_, ::growbox::ipo::SignalRole::InsideCo2,
      readings.data(), readings.size(), timeout_ms);
  output.co2_ppm = selectedMeasuredValue(last_selections_[2]);
  return output.air_temperature_c.valid || output.relative_humidity_pct.valid ||
         output.co2_ppm.valid;
}

RuntimeNearbySource::RuntimeNearbySource(
    native::BleClimateScanner& ble, const ::growbox::ipo::Graph* graph,
    const RuntimeScheduleProfileOwner* profile) noexcept
    : ble_(ble), graph_(graph), profile_(profile) {}

bool RuntimeNearbySource::sample(std::uint64_t monotonic_ms,
                                 OutsideEnvironmentSnapshot& output) noexcept {
  output = {};
  native::BleClimateReading xiaomi{};
  const bool sampled = ble_.sampleXiaomi(monotonic_ms, xiaomi);
  if (graph_ == nullptr) {
    if (!sampled) return false;
    output.air_temperature_c = {xiaomi.temperature_c, true, xiaomi.age_ms};
    output.relative_humidity_pct = {xiaomi.relative_humidity_pct, true, xiaomi.age_ms};
    return true;
  }
  const std::array<::growbox::ipo::SignalReading, 2U> readings{{
      {3U, ::growbox::ipo::Channel::Temperature, xiaomi.temperature_c,
       xiaomi.age_ms, sampled},
      {3U, ::growbox::ipo::Channel::Humidity, xiaomi.relative_humidity_pct,
       xiaomi.age_ms, sampled},
  }};
  const auto timeout_ms = sensorTimeout(profile_);
  last_selections_[0] = ::growbox::ipo::selectFreshSignal(
      *graph_, ::growbox::ipo::SignalRole::OutsideTemperature,
      readings.data(), readings.size(), timeout_ms);
  output.air_temperature_c = selectedMeasuredValue(last_selections_[0]);
  last_selections_[1] = ::growbox::ipo::selectFreshSignal(
      *graph_, ::growbox::ipo::SignalRole::OutsideHumidity,
      readings.data(), readings.size(), timeout_ms);
  output.relative_humidity_pct = selectedMeasuredValue(last_selections_[1]);
  return output.air_temperature_c.valid || output.relative_humidity_pct.valid;
}

bool ConfiguredScheduleConfigSource::resolve(
    std::uint64_t, const ClimateWallClockSnapshot& clock,
    ClimateScheduleConfigSnapshot& output) noexcept {
  return resolveConfiguredScheduleProfile(clock, owner_.active(), output);
}

bool MintScheduleConfigSource::resolve(std::uint64_t, const ClimateWallClockSnapshot& clock,
                                       ClimateScheduleConfigSnapshot& output) noexcept {
  return resolveMintScheduleProfile(clock, output);
}

} // namespace growbox::app::climate_io::runtime
