#pragma once

#include "IpoSignalSelector.h"
#include "climate/ClimateRuntimeController.h"
#include "climate/application/ClimateCompositeInput.h"
#include "climate/input/ble/BleClimateScanner.h"
#include "climate/input/sensors/Scd41InsideSource.h"
#include "climate/runtime/schedule/RuntimeScheduleProfile.h"

#include <array>
#include <cstdint>

namespace growbox::app::climate_io::runtime {

class LockedFakeRoleDriver final : public ClimateRoleDriver {
public:
  bool apply(ClimateActuatorRole role, float level, std::uint64_t monotonic_ms) noexcept override;
};

class RuntimeInsideSource final : public InsideEnvironmentSource {
public:
  RuntimeInsideSource(native::BleClimateScanner& ble, native::Scd41InsideSource& scd41,
                      const ::growbox::ipo::Graph* graph = nullptr,
                      const RuntimeScheduleProfileOwner* profile = nullptr) noexcept;

  bool sample(std::uint64_t monotonic_ms, InsideEnvironmentSnapshot& output) noexcept override;
  const std::array<::growbox::ipo::SignalSelection, 3U>& lastSelections() const noexcept {
    return last_selections_;
  }

private:
  native::BleClimateScanner& ble_;
  native::Scd41InsideSource& scd41_;
  const ::growbox::ipo::Graph* graph_{nullptr};
  const RuntimeScheduleProfileOwner* profile_{nullptr};
  std::array<::growbox::ipo::SignalSelection, 3U> last_selections_{};
};

class RuntimeNearbySource final : public OutsideEnvironmentSource {
public:
  explicit RuntimeNearbySource(native::BleClimateScanner& ble,
                               const ::growbox::ipo::Graph* graph = nullptr,
                               const RuntimeScheduleProfileOwner* profile = nullptr) noexcept;

  bool sample(std::uint64_t monotonic_ms, OutsideEnvironmentSnapshot& output) noexcept override;
  const std::array<::growbox::ipo::SignalSelection, 2U>& lastSelections() const noexcept {
    return last_selections_;
  }

private:
  native::BleClimateScanner& ble_;
  const ::growbox::ipo::Graph* graph_{nullptr};
  const RuntimeScheduleProfileOwner* profile_{nullptr};
  std::array<::growbox::ipo::SignalSelection, 2U> last_selections_{};
};

// The real runtime reads the same admitted profile as the lamp scheduler.
class ConfiguredScheduleConfigSource final : public ClimateScheduleConfigSource {
public:
  explicit ConfiguredScheduleConfigSource(const RuntimeScheduleProfileOwner& owner) noexcept
      : owner_(owner) {}

  bool resolve(std::uint64_t monotonic_ms, const ClimateWallClockSnapshot& clock,
               ClimateScheduleConfigSnapshot& output) noexcept override;

private:
  const RuntimeScheduleProfileOwner& owner_;
};

// Legacy Mint source is preserved for previous host fixture callers.
class MintScheduleConfigSource final : public ClimateScheduleConfigSource {
public:
  bool resolve(std::uint64_t monotonic_ms, const ClimateWallClockSnapshot& clock,
               ClimateScheduleConfigSnapshot& output) noexcept override;
};

inline constexpr ::growbox::climate::ClimateRuntimeConfig productionRuntimeConfig() noexcept {
  ::growbox::climate::ClimateRuntimeConfig config{};
  config.mode = ::growbox::climate::ClimatePolicyMode::Rule;
  config.sensor_timeout_ms = ::growbox::climate::kDefaultSensorTimeoutMs;
  config.timestep_s = 1.0F;
  config.allow_unqualified_ml_active = false;
  return config;
}

static_assert(productionRuntimeConfig().mode == ::growbox::climate::ClimatePolicyMode::Rule,
              "Production runtime must keep deterministic rule control authoritative");
static_assert(!productionRuntimeConfig().allow_unqualified_ml_active,
              "Production runtime must not enable ML active authority");

} // namespace growbox::app::climate_io::runtime
