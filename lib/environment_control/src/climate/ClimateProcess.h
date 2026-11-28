#pragma once

#include "ClimatePolicyTypes.h"

#include <cstdint>

namespace growbox::climate {

enum class ClimatePolicyMode : std::uint8_t { Rule = 0U, MlShadow = 1U, MlActive = 2U };

enum class ClimateRuntimeStatus : std::uint8_t {
  Ok = 0U,
  MlProviderMissing,
  MlInferenceFailed,
  MlActiveNotAllowed,
};

struct ClimateRuntimeDecision {
  ClimateRuntimeStatus status = ClimateRuntimeStatus::Ok;
  ClimatePolicyMode mode = ClimatePolicyMode::Rule;
  bool authoritative_ml = false;
  bool ml_evaluated = false;
  ClimatePolicyEvaluation rule{};
  ClimateGoalDiagnostics rule_goals{};
  ClimatePolicyEvaluation ml{};
  ClimateFeatureVector ml_features{};
  ClimateEncoderReport encoder_report{};
  ClimateTrends trends{};
  EstimatedEffectiveClimateActions effective_before{};
  ClimatePolicyRequest applied{};
  ClimateExecutionProjection execution{};
  EstimatedEffectiveClimateActions effective_after{};
};

// The process owns its estimator and decision policy; the loop owns orchestration.
class ClimateProcess {
public:
  virtual ~ClimateProcess() = default;
  virtual ClimateRuntimeStatus step(const ClimateControllerInput& input, std::uint64_t monotonic_ms,
                                    ClimateRuntimeDecision& decision) noexcept = 0;
  virtual void reconcileExecution(const ClimateExecutionProjection& execution,
                                  const ClimateCapabilities& capabilities,
                                  ClimateRuntimeDecision& decision) noexcept = 0;
  virtual void reset() noexcept = 0;
};

} // namespace growbox::climate
