#pragma once

#include "climate/ClimateIoPorts.h"
#include "climate/application/ClimateSnapshotContract.h"
#include "climate/application/ClimateRoleContract.h"

#include <cstdint>

namespace growbox::app::climate_io {

class ClimateInputAdapter final : public ::growbox::climate::ClimateInputSource {
public:
  explicit ClimateInputAdapter(ClimateSnapshotProvider& provider) noexcept : provider_(provider) {}

  bool sample(std::uint64_t monotonic_ms,
              ::growbox::climate::ClimateControllerInput& input) noexcept override;

private:
  ClimateSnapshotProvider& provider_;
};

class ClimateActuatorAdapter final : public ::growbox::climate::ClimateActuatorSink {
public:
  explicit ClimateActuatorAdapter(ClimateRoleDriver& driver) noexcept : driver_(driver) {}

  bool apply(const ::growbox::climate::ClimatePolicyRequest& request,
             std::uint64_t monotonic_ms) noexcept override;
  bool
  applyAndReport(const ::growbox::climate::ClimatePolicyRequest& request,
                 std::uint64_t monotonic_ms,
                 ::growbox::climate::ClimatePolicyRequest& confirmed_applied) noexcept override;
  bool applyFailSafeOff(std::uint64_t monotonic_ms) noexcept override;

private:
  ClimateRoleDriver& driver_;
};

} // namespace growbox::app::climate_io
