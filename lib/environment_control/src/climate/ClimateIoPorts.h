#pragma once

#include "ClimateTypes.h"

#include <cstdint>

namespace growbox::climate {

class ClimateInputSource {
public:
  virtual ~ClimateInputSource() = default;
  virtual bool sample(std::uint64_t monotonic_ms, ClimateControllerInput& input) noexcept = 0;
};

class ClimateActuatorSink {
public:
  virtual ~ClimateActuatorSink() = default;
  virtual bool apply(const ClimatePolicyRequest& request, std::uint64_t monotonic_ms) noexcept = 0;

  // Default sinks are exact: the accepted request is the applied request.
  // Hardware adapters with binary/dwell arbitration override this method.
  virtual bool applyAndReport(const ClimatePolicyRequest& request, std::uint64_t monotonic_ms,
                              ClimatePolicyRequest& confirmed_applied) noexcept {
    const bool ok = apply(request, monotonic_ms);
    confirmed_applied = ok ? request : ClimatePolicyRequest{};
    return ok;
  }

  // Execution-aware sink contract. Compatibility sinks report a complete accepted
  // request; supervisor-backed sinks override this with command-truth projection.
  virtual bool applyAndReportExecution(const ClimatePolicyRequest& request,
                                       std::uint64_t monotonic_ms,
                                       ClimateExecutionProjection& execution) noexcept {
    execution = {};
    ClimatePolicyRequest confirmed_applied{};
    const bool ok = applyAndReport(request, monotonic_ms, confirmed_applied);
    if (!ok) {
      return false;
    }
    execution.executed = confirmed_applied;
    execution.known_mask = ClimateExecutionKnownAll;
    return true;
  }

  // Fail-safe OFF must bypass ordinary dwell/hysteresis in hardware adapters.
  virtual bool applyFailSafeOff(std::uint64_t monotonic_ms) noexcept {
    const ClimatePolicyRequest off{};
    return apply(off, monotonic_ms);
  }
};

} // namespace growbox::climate
