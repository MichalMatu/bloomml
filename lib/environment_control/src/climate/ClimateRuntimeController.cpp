#include "ClimateRuntimeController.h"

#include <cmath>

namespace growbox::climate {

ClimateRuntimeController::ClimateRuntimeController(ClimateInferenceProvider* ml_provider,
                                                   ClimateRuntimeConfig config) noexcept
    : ml_provider_(ml_provider), config_(config) {}

ClimateRuntimeStatus ClimateRuntimeController::step(const ClimateControllerInput& input,
                                                    std::uint64_t monotonic_ms,
                                                    ClimateRuntimeDecision& decision) noexcept {
  decision = {};
  decision.mode = config_.mode;

  ClimateControllerInput runtime_input = input;
  runtime_input.sensor_timeout_ms = config_.sensor_timeout_ms;
  decision.trends = trend_estimator_.update(runtime_input.state.measurements, monotonic_ms,
                                            config_.sensor_timeout_ms);
  runtime_input.state.trends = decision.trends;
  decision.effective_before = effective_estimator_.state();
  runtime_input.estimated_effective = decision.effective_before;

  policy::evaluate(policy::ruleRequest(runtime_input), runtime_input, decision.rule);
  decision.applied = decision.rule.safe;

  if (config_.mode == ClimatePolicyMode::MlActive && !config_.allow_unqualified_ml_active) {
    decision.status = ClimateRuntimeStatus::MlActiveNotAllowed;
  } else if (config_.mode != ClimatePolicyMode::Rule) {
    if (ml_provider_ == nullptr) {
      decision.status = ClimateRuntimeStatus::MlProviderMissing;
    } else {
      decision.ml_features = ClimateFeatureEncoder::encode(runtime_input, &decision.encoder_report);
      ClimatePolicyRequest ml_raw{};
      if (!ml_provider_->infer(decision.ml_features, ml_raw) || !policy::finiteRequest(ml_raw)) {
        decision.status = ClimateRuntimeStatus::MlInferenceFailed;
      } else {
        decision.ml_evaluated = true;
        policy::evaluate(policy::applyDeadzone(ml_raw, config_.ml_deadzone), runtime_input,
                         decision.ml);
        if (config_.mode == ClimatePolicyMode::MlActive) {
          decision.authoritative_ml = true;
          decision.applied = decision.ml.safe;
        }
      }
    }
  }

  const float timestep =
      std::isfinite(config_.timestep_s) && config_.timestep_s > 0.0F ? config_.timestep_s : 10.0F;
  decision.effective_after =
      effective_estimator_.update(decision.applied, timestep, runtime_input.capabilities);
  return decision.status;
}

void ClimateRuntimeController::reconcileExecution(const ClimateExecutionProjection& execution,
                                                  const ClimateCapabilities& capabilities,
                                                  ClimateRuntimeDecision& decision) noexcept {
  const float timestep =
      std::isfinite(config_.timestep_s) && config_.timestep_s > 0.0F ? config_.timestep_s : 10.0F;

  const ClimatePolicyRequest bounded = policy::clipped(execution.executed);
  ClimatePolicyRequest estimator_request = bounded;
  ClimatePolicyRequest compatibility_applied = decision.applied;

  const auto reconcile_role = [&](ClimateExecutionKnownMask mask, float executed,
                                  float effective_before, float& estimator_value,
                                  float& applied_value) noexcept {
    if (execution.known(mask)) {
      estimator_value = executed;
      applied_value = executed;
    } else {
      // Holding the estimator target at its previous effective value preserves
      // the estimate exactly for an unreported role without inventing OFF/ON.
      estimator_value = effective_before;
    }
  };

  reconcile_role(ClimateExecutionKnownHeater, bounded.heater, decision.effective_before.heater,
                 estimator_request.heater, compatibility_applied.heater);
  reconcile_role(ClimateExecutionKnownCooler, bounded.cooler, decision.effective_before.cooler,
                 estimator_request.cooler, compatibility_applied.cooler);
  reconcile_role(ClimateExecutionKnownExhaustFan, bounded.exhaust_fan,
                 decision.effective_before.exhaust_fan, estimator_request.exhaust_fan,
                 compatibility_applied.exhaust_fan);
  reconcile_role(ClimateExecutionKnownHumidifier, bounded.humidifier,
                 decision.effective_before.humidifier, estimator_request.humidifier,
                 compatibility_applied.humidifier);
  reconcile_role(ClimateExecutionKnownDehumidifier, bounded.dehumidifier,
                 decision.effective_before.dehumidifier, estimator_request.dehumidifier,
                 compatibility_applied.dehumidifier);
  reconcile_role(ClimateExecutionKnownCo2Doser, bounded.co2_doser,
                 decision.effective_before.co2_doser, estimator_request.co2_doser,
                 compatibility_applied.co2_doser);

  effective_estimator_.setState(decision.effective_before);
  decision.execution = execution;
  decision.applied = policy::clipped(compatibility_applied);
  decision.effective_after = effective_estimator_.update(estimator_request, timestep, capabilities);
}

void ClimateRuntimeController::reconcileApplied(const ClimatePolicyRequest& confirmed_applied,
                                                const ClimateCapabilities& capabilities,
                                                ClimateRuntimeDecision& decision) noexcept {
  ClimateExecutionProjection execution{};
  execution.executed = confirmed_applied;
  execution.known_mask = ClimateExecutionKnownAll;
  reconcileExecution(execution, capabilities, decision);
}

void ClimateRuntimeController::reset() noexcept {
  trend_estimator_.reset();
  effective_estimator_.reset();
}

} // namespace growbox::climate
