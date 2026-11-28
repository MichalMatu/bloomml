#pragma once

#include "ClimateActuatorStateEstimator.h"
#include "ClimateFeatureEncoder.h"
#include "ClimatePolicy.h"
#include "ClimateTrendEstimator.h"
#include "ClimateProcess.h"

#include <cstdint>

namespace growbox::climate {

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

class ClimateRuntimeController final : public ClimateProcess {
public:
  explicit ClimateRuntimeController(ClimateInferenceProvider* ml_provider = nullptr,
                                    ClimateRuntimeConfig config = {}) noexcept;

  ClimateRuntimeStatus step(const ClimateControllerInput& input, std::uint64_t monotonic_ms,
                            ClimateRuntimeDecision& decision) noexcept override;

  // Reconcile estimator state from execution truth. Known roles use the
  // executed command projection; unknown roles hold effective_before rather
  // than being fabricated as OFF. No physical acknowledgement is implied.
  void reconcileExecution(const ClimateExecutionProjection& execution,
                          const ClimateCapabilities& capabilities,
                          ClimateRuntimeDecision& decision) noexcept override;

  // Short migration wrapper for sinks/tests that still report a complete request.
  void reconcileApplied(const ClimatePolicyRequest& confirmed_applied,
                        const ClimateCapabilities& capabilities,
                        ClimateRuntimeDecision& decision) noexcept;

  void reset() noexcept override;

private:
  ClimateInferenceProvider* ml_provider_ = nullptr;
  ClimateRuntimeConfig config_{};
  ClimateTrendEstimator trend_estimator_{};
  ClimateActuatorStateEstimator effective_estimator_{};
};

} // namespace growbox::climate
