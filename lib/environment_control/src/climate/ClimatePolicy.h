#pragma once

#include "ClimateTypes.h"

#include <cstdint>

namespace growbox::climate {

enum ClimateIntervention : std::uint32_t {
  InterventionNone = 0U,
  UnavailableHeater = 1U << 0U,
  UnavailableCooler = 1U << 1U,
  UnavailableExhaustFan = 1U << 2U,
  UnavailableHumidifier = 1U << 3U,
  UnavailableDehumidifier = 1U << 4U,
  UnavailableCo2Doser = 1U << 5U,
  OppositionHeaterCooler = 1U << 6U,
  OppositionHumidifierDehumidifier = 1U << 7U,
  RequiredSensorUnusable = 1U << 8U,
  Co2DosingInhibited = 1U << 9U,
  HighTemperature = 1U << 10U,
  LowTemperature = 1U << 11U,
  HighHumidity = 1U << 12U,
  HighCo2 = 1U << 13U,
};

struct ClimatePolicyEvaluation {
  ClimatePolicyRequest raw{};
  ClimatePolicyRequest arbitrated{};
  ClimatePolicyRequest safe{};
  std::uint32_t arbitration_interventions = InterventionNone;
  std::uint32_t safety_interventions = InterventionNone;
};

namespace policy {
[[nodiscard]] ClimatePolicyRequest ruleRequest(const ClimateControllerInput& input) noexcept;
void evaluate(const ClimatePolicyRequest& raw, const ClimateControllerInput& input,
              ClimatePolicyEvaluation& evaluation) noexcept;
[[nodiscard]] bool finiteRequest(const ClimatePolicyRequest& request) noexcept;
[[nodiscard]] ClimatePolicyRequest applyDeadzone(const ClimatePolicyRequest& raw,
                                                 float threshold) noexcept;
[[nodiscard]] ClimatePolicyRequest clipped(const ClimatePolicyRequest& request) noexcept;
} // namespace policy

} // namespace growbox::climate
