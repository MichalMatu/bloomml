#pragma once

#include <cstdint>

namespace growbox::app::climate_io::runtime {

class RuntimeCycleState final {
public:
  explicit RuntimeCycleState(std::uint64_t output_intent_sequence = 0U,
                             std::uint32_t diagnostic_tick = 0U) noexcept
      : output_intent_sequence_(output_intent_sequence), diagnostic_tick_(diagnostic_tick) {}

  std::uint64_t nextOutputIntentSequence() noexcept;
  bool telemetryDue() noexcept;

  std::uint64_t outputIntentSequence() const noexcept {
    return output_intent_sequence_;
  }

  std::uint32_t diagnosticTick() const noexcept {
    return diagnostic_tick_;
  }

private:
  static constexpr std::uint32_t kTelemetryEveryTicks = 10U;

  std::uint64_t output_intent_sequence_{0U};
  std::uint32_t diagnostic_tick_{0U};
};

class RuntimePersistenceRetryState final {
public:
  bool due(std::uint64_t now_ms) const noexcept;
  bool onFailure(std::uint64_t now_ms) noexcept;
  void onSuccess() noexcept;

  bool pending() const noexcept {
    return pending_;
  }

  bool errorActive() const noexcept {
    return error_active_;
  }

  std::uint64_t delayMs() const noexcept {
    return retry_delay_ms_;
  }

  std::uint64_t retryAfterMs() const noexcept {
    return retry_after_ms_;
  }

private:
  static constexpr std::uint64_t kInitialDelayMs = 1'000U;
  static constexpr std::uint64_t kMaximumDelayMs = 60'000U;
  static constexpr std::uint64_t kErrorLogIntervalMs = 60'000U;

  std::uint64_t retry_after_ms_{0U};
  std::uint64_t retry_delay_ms_{0U};
  std::uint64_t last_error_log_ms_{0U};
  bool pending_{false};
  bool error_active_{false};
};

} // namespace growbox::app::climate_io::runtime
