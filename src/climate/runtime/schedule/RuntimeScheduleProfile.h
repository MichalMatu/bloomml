#pragma once

#include "climate/application/ClimateCompositeInput.h"

#include <cmath>
#include <cstring>
#include <cstdint>

namespace growbox::app::climate_io::runtime {

// A bounded, typed step between a future validated DeviceProfile v2 and the
// existing climate-v6 Rule/schedule runtime. No JSON, NVS or output transport.
// The fixed Mint instance is the ONLY profile installed in production today.
struct RuntimeTargetSet {
  float temperature_c{24.5F};
  float relative_humidity_pct{58.0F};
  float air_vpd_kpa{1.2F};
};

struct RuntimeScheduleProfile {
  std::uint16_t version{1U};
  std::uint8_t day_start_hour{6U};
  std::uint8_t night_start_hour{22U};
  ::growbox::climate::HumidityControlMode humidity_mode =
      ::growbox::climate::HumidityControlMode::Rh;
  std::uint64_t sensor_timeout_ms{::growbox::climate::kDefaultSensorTimeoutMs};
  RuntimeTargetSet day{};
  RuntimeTargetSet night{21.5F, 65.0F, 0.9F};
  bool exhaust_fan_available{true};
  bool humidifier_available{true};
};

inline constexpr RuntimeScheduleProfile makeMintRuntimeScheduleProfile() noexcept {
  return {};
}

inline bool validateRuntimeTargetSet(const RuntimeTargetSet& targets) noexcept {
  return std::isfinite(targets.temperature_c) && targets.temperature_c >= 10.0F &&
         targets.temperature_c <= 40.0F && std::isfinite(targets.relative_humidity_pct) &&
         targets.relative_humidity_pct >= 20.0F && targets.relative_humidity_pct <= 90.0F &&
         std::isfinite(targets.air_vpd_kpa) && targets.air_vpd_kpa >= 0.2F &&
         targets.air_vpd_kpa <= 3.5F;
}

inline bool validateRuntimeScheduleProfile(const RuntimeScheduleProfile& profile) noexcept {
  return profile.version == 1U && profile.day_start_hour < 24U &&
         profile.night_start_hour < 24U &&
         profile.day_start_hour != profile.night_start_hour &&
         (profile.humidity_mode == ::growbox::climate::HumidityControlMode::Rh ||
          profile.humidity_mode == ::growbox::climate::HumidityControlMode::Vpd) &&
         profile.sensor_timeout_ms >= 5'000ULL &&
         profile.sensor_timeout_ms <= 300'000ULL &&
         validateRuntimeTargetSet(profile.day) && validateRuntimeTargetSet(profile.night);
}

inline bool isRuntimeDaytime(std::uint8_t hour,
                             const RuntimeScheduleProfile& profile) noexcept {
  if (hour >= 24U || !validateRuntimeScheduleProfile(profile)) return false;
  if (profile.day_start_hour < profile.night_start_hour) {
    return hour >= profile.day_start_hour && hour < profile.night_start_hour;
  }
  return hour >= profile.day_start_hour || hour < profile.night_start_hour;
}

// Generation/readback represents accepted in-RAM state ONLY; it is NOT an
// assertion of NVS durability, live hardware profile activation or RF truth.
class RuntimeScheduleProfileOwner final {
public:
  RuntimeScheduleProfileOwner() noexcept = default;

  bool valid() const noexcept { return validateRuntimeScheduleProfile(active_); }
  const RuntimeScheduleProfile& active() const noexcept { return active_; }
  std::uint32_t generation() const noexcept { return generation_; }
  bool bootDefaultMint() const noexcept { return generation_ == 0U; }

  // Reserved for the future privileged profile coordinator after full graph,
  // board capability, NVS and rollback admission. Do not expose over UART/UI.
  bool replaceValidated(const RuntimeScheduleProfile& candidate,
                        std::uint32_t expected_generation) noexcept {
    if (expected_generation != generation_ || !validateRuntimeScheduleProfile(candidate) ||
        generation_ == UINT32_MAX) {
      return false;
    }
    active_ = candidate;
    ++generation_;
    return true;
  }

private:
  RuntimeScheduleProfile active_ = makeMintRuntimeScheduleProfile();
  std::uint32_t generation_{0U};
};

// Stable fingerprint of the *active schedule settings* (not a v2 graph
// digest, not evidence of flash persistence or physical output state).
inline std::uint32_t runtimeScheduleFingerprint(
    const RuntimeScheduleProfile& profile) noexcept {
  std::uint32_t hash = 2166136261U;
  const auto mix = [&hash](std::uint32_t bits) noexcept {
    for (std::uint8_t i = 0U; i < 4U; ++i) {
      hash = (hash ^ static_cast<std::uint8_t>(bits >> (i * 8U))) * 16777619U;
    }
  };
  const auto mixFloat = [&mix](float value) noexcept {
    std::uint32_t bits = 0U;
    std::memcpy(&bits, &value, sizeof(value));
    mix(bits);
  };
  mix(profile.version);
  mix(profile.day_start_hour);
  mix(profile.night_start_hour);
  mix(static_cast<std::uint8_t>(profile.humidity_mode));
  mix(static_cast<std::uint32_t>(profile.sensor_timeout_ms));
  mix(static_cast<std::uint32_t>(profile.sensor_timeout_ms >> 32U));
  mixFloat(profile.day.temperature_c);
  mixFloat(profile.day.relative_humidity_pct);
  mixFloat(profile.day.air_vpd_kpa);
  mixFloat(profile.night.temperature_c);
  mixFloat(profile.night.relative_humidity_pct);
  mixFloat(profile.night.air_vpd_kpa);
  mix(profile.exhaust_fan_available ? 1U : 0U);
  mix(profile.humidifier_available ? 1U : 0U);
  return hash;
}

inline bool projectRuntimeSchedule(std::uint8_t hour,
                                   const RuntimeScheduleProfile& profile,
                                   ClimateScheduleConfigSnapshot& output) noexcept {
  output = {};
  if (hour >= 24U || !validateRuntimeScheduleProfile(profile)) return false;
  const bool day = isRuntimeDaytime(hour, profile);
  const RuntimeTargetSet& targets = day ? profile.day : profile.night;

  output.humidity_control_mode = profile.humidity_mode;
  output.sensor_timeout_ms = profile.sensor_timeout_ms;
  output.capabilities.exhaust_fan = profile.exhaust_fan_available;
  output.capabilities.humidifier = profile.humidifier_available;
  output.capabilities.heater = false;
  output.capabilities.cooler = false;
  output.capabilities.dehumidifier = false;
  output.capabilities.co2_doser = false;
  output.targets.air_temperature_c = targets.temperature_c;
  output.targets.relative_humidity_pct = targets.relative_humidity_pct;
  output.targets.air_vpd_kpa = targets.air_vpd_kpa;
  output.targets.co2_enabled = false; // No physical CO2 doser configured.
  output.targets.co2_ppm = 0.0F;
  output.schedule.light_level = day ? 1.0F : 0.0F;
  return true;
}

} // namespace growbox::app::climate_io::runtime
