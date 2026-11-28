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

// Per-goal decision evidence, not a prediction or physical achievement claim.
enum ClimateGoal : std::uint8_t {
  GoalTemperature = 1U << 0U,
  GoalHumidity = 1U << 1U, // active RH or VPD target
  GoalCo2 = 1U << 2U,
};
struct ClimateGoalDiagnostics {
  std::uint8_t unmet{0U};
  std::uint8_t measurement_unusable{0U};
  std::uint8_t target_invalid{0U};
  std::uint8_t no_equipment{0U};
  std::uint8_t no_usable_route{0U};
  std::uint8_t constrained{0U};
  std::uint8_t conditional_ventilation{0U};
};

struct ClimatePolicyEvaluation {
  ClimatePolicyRequest raw{};
  ClimatePolicyRequest arbitrated{};
  ClimatePolicyRequest safe{};
  std::uint32_t arbitration_interventions = InterventionNone;
  std::uint32_t safety_interventions = InterventionNone;
};

} // namespace growbox::climate
