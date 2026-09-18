#include "climate/runtime/core/RuntimeCycleState.h"

#include <limits>

namespace growbox::app::climate_io::runtime {

std::uint64_t RuntimeCycleState::nextOutputIntentSequence() noexcept {
  ++output_intent_sequence_;
  if (output_intent_sequence_ == 0U) {
    ++output_intent_sequence_;
  }
  return output_intent_sequence_;
}

bool RuntimeCycleState::telemetryDue() noexcept {
  const bool due = (diagnostic_tick_ % kTelemetryEveryTicks) == 0U;
  ++diagnostic_tick_;
  return due;
}

bool RuntimePersistenceRetryState::due(std::uint64_t now_ms) const noexcept {
  return !pending_ || now_ms >= retry_after_ms_;
}

bool RuntimePersistenceRetryState::onFailure(std::uint64_t now_ms) noexcept {
  const bool log_due = !error_active_ ||
                       (now_ms - last_error_log_ms_) >= kErrorLogIntervalMs;

  if (retry_delay_ms_ == 0U) {
    retry_delay_ms_ = kInitialDelayMs;
  } else if (retry_delay_ms_ >= (kMaximumDelayMs / 2U)) {
    retry_delay_ms_ = kMaximumDelayMs;
  } else {
    retry_delay_ms_ *= 2U;
  }

  retry_after_ms_ = now_ms + retry_delay_ms_;
  pending_ = true;
  error_active_ = true;
  if (log_due) {
    last_error_log_ms_ = now_ms;
  }
  return log_due;
}

void RuntimePersistenceRetryState::onSuccess() noexcept {
  retry_after_ms_ = 0U;
  retry_delay_ms_ = 0U;
  pending_ = false;
  error_active_ = false;
}

} // namespace growbox::app::climate_io::runtime
