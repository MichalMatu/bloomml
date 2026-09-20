#pragma once

#include "ClimateActuatorStateEstimator.h"
#include "ClimateFeatureEncoder.h"
#include "ClimatePolicy.h"
#include "ClimateTrendEstimator.h"
#include "ClimateTypes.h"

#include <cstdint>

namespace growbox::climate {

enum class ClimatePolicyMode : std::uint8_t { Rule = 0U, MlShadow = 1U, MlActive = 2U };

enum class ClimateRuntimeStatus : std::uint8_t {
  Ok = 0U,
  MlProviderMissing,
  MlInferenceFailed,
  MlActiveNotAllowed,
};

struct ClimateRuntimeConfig {
  ClimatePolicyMode mode = ClimatePolicyMode::Rule;
  std::uint64_t sensor_timeout_ms = kDefaultSensorTimeoutMs;
  float timestep_s = 10.0F;
  float ml_deadzone = 0.05F;
  bool allow_unqualified_ml_active = false;
};

class ClimateInferenceProvider {
public:
  virtual ~ClimateInferenceProvider() = default;
  virtual bool infer(const ClimateFeatureVector& features,
                     ClimatePolicyRequest& output) noexcept = 0;
};

struct ClimateRuntimeDecision {
  ClimateRuntimeStatus status = ClimateRuntimeStatus::Ok;
  ClimatePolicyMode mode = ClimatePolicyMode::Rule;
  bool authoritative_ml = false;
  bool ml_evaluated = false;
  ClimatePolicyEvaluation rule{};
  ClimatePolicyEvaluation ml{};
  ClimateFeatureVector ml_features{};
  ClimateEncoderReport encoder_report{};
  ClimateTrends trends{};
  EstimatedEffectiveClimateActions effective_before{};
  ClimatePolicyRequest applied{};
  ClimateExecutionProjection execution{};
  EstimatedEffectiveClimateActions effective_after{};
};

class ClimateRuntimeController {
public:
  explicit ClimateRuntimeController(ClimateInferenceProvider* ml_provider = nullptr,
                                    ClimateRuntimeConfig config = {}) noexcept;

  ClimateRuntimeStatus step(const ClimateControllerInput& input, std::uint64_t monotonic_ms,
                            ClimateRuntimeDecision& decision) noexcept;

  // Reconcile estimator state from execution truth. Known roles use the
  // executed command projection; unknown roles hold effective_before rather
  // than being fabricated as OFF. No physical acknowledgement is implied.
  void reconcileExecution(const ClimateExecutionProjection& execution,
                          const ClimateCapabilities& capabilities,
                          ClimateRuntimeDecision& decision) noexcept;

  // Short migration wrapper for sinks/tests that still report a complete request.
  void reconcileApplied(const ClimatePolicyRequest& confirmed_applied,
                        const ClimateCapabilities& capabilities,
                        ClimateRuntimeDecision& decision) noexcept;

  void reset() noexcept;

private:
  ClimateInferenceProvider* ml_provider_ = nullptr;
  ClimateRuntimeConfig config_{};
  ClimateTrendEstimator trend_estimator_{};
  ClimateActuatorStateEstimator effective_estimator_{};
};

} // namespace growbox::climate
