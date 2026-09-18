#include "climate/runtime/core/RuntimeCycleState.h"

#include <cassert>
#include <cstdint>
#include <limits>

using growbox::app::climate_io::runtime::RuntimeCycleState;
using growbox::app::climate_io::runtime::RuntimePersistenceRetryState;

namespace {

void testIntentSequenceSkipsZeroOnWrap() {
  RuntimeCycleState state(std::numeric_limits<std::uint64_t>::max());
  assert(state.nextOutputIntentSequence() == 1U);
  assert(state.outputIntentSequence() == 1U);
}

void testIntentSequenceIncrementsMonotonically() {
  RuntimeCycleState state;
  assert(state.nextOutputIntentSequence() == 1U);
  assert(state.nextOutputIntentSequence() == 2U);
}

void testTelemetryCadenceStartsImmediatelyAndRepeatsEveryTenTicks() {
  RuntimeCycleState state;
  assert(state.telemetryDue());
  for (std::uint32_t tick = 1U; tick < 10U; ++tick) {
    assert(!state.telemetryDue());
  }
  assert(state.telemetryDue());
  assert(state.diagnosticTick() == 11U);
}

void testPersistenceRetryBackoffAndRecovery() {
  RuntimePersistenceRetryState state;
  assert(state.due(100U));
  assert(!state.pending());
  assert(!state.errorActive());

  assert(state.onFailure(100U));
  assert(state.pending());
  assert(state.errorActive());
  assert(state.delayMs() == 1'000U);
  assert(state.retryAfterMs() == 1'100U);
  assert(!state.due(1'099U));
  assert(state.due(1'100U));

  assert(!state.onFailure(1'100U));
  assert(state.delayMs() == 2'000U);
  assert(state.retryAfterMs() == 3'100U);

  std::uint64_t now_ms = state.retryAfterMs();
  while (state.delayMs() < 60'000U) {
    assert(state.due(now_ms));
    static_cast<void>(state.onFailure(now_ms));
    now_ms = state.retryAfterMs();
  }
  assert(state.delayMs() == 60'000U);
  assert(state.retryAfterMs() == now_ms);

  const std::uint64_t capped_retry_ms = state.retryAfterMs();
  static_cast<void>(state.onFailure(capped_retry_ms));
  assert(state.delayMs() == 60'000U);
  assert(state.retryAfterMs() == capped_retry_ms + 60'000U);

  state.onSuccess();
  assert(!state.pending());
  assert(!state.errorActive());
  assert(state.delayMs() == 0U);
  assert(state.retryAfterMs() == 0U);
  assert(state.due(capped_retry_ms));
}

void testPersistenceErrorLogIsRateLimited() {
  RuntimePersistenceRetryState state;
  assert(state.onFailure(5'000U));
  assert(!state.onFailure(6'000U));
  assert(!state.onFailure(64'999U));
  assert(state.onFailure(65'000U));
}

} // namespace

int main() {
  testIntentSequenceSkipsZeroOnWrap();
  testIntentSequenceIncrementsMonotonically();
  testTelemetryCadenceStartsImmediatelyAndRepeatsEveryTenTicks();
  testPersistenceRetryBackoffAndRecovery();
  testPersistenceErrorLogIsRateLimited();
  return 0;
}
