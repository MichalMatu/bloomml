#pragma once

#include "ClimateIoPorts.h"
#include "ClimateProcess.h"

#include <cstdint>

namespace growbox::climate {

enum class ClimateLoopIoStatus : std::uint8_t {
  Ok = 0U,
  InputUnavailable,
  ActuatorApplyFailed,
  ActuatorFaultLatched,
};

struct ClimateLoopResult {
  ClimateLoopIoStatus io_status = ClimateLoopIoStatus::Ok;
  ClimateRuntimeStatus runtime_status = ClimateRuntimeStatus::Ok;
  bool input_sampled = false;
  bool command_applied = false;
  bool fail_safe_attempted = false;
  bool fail_safe_applied = false;
};

class ClimateControlLoop {
public:
  ClimateControlLoop(ClimateProcess& runtime, ClimateInputSource& input_source,
                     ClimateActuatorSink& actuator_sink) noexcept;

  ClimateLoopResult tick(std::uint64_t monotonic_ms, ClimateRuntimeDecision& decision) noexcept;

  // Optional migration seam: known roles from external execution truth override
  // the compatibility previous_applied_ snapshot used for the next controller input.
  void setPreviousExecutionFeedback(const ClimateExecutionProjection& feedback) noexcept;
  void clearPreviousExecutionFeedback() noexcept;
  bool hasExternalPreviousExecutionFeedback() const noexcept {
    return has_external_previous_execution_;
  }

  void reset() noexcept;

  bool actuatorFaultLatched() const noexcept {
    return actuator_fault_latched_;
  }
  const PreviousClimateActions& previousApplied() const noexcept {
    return previous_applied_;
  }

private:
  static PreviousClimateActions previousFromRequest(const ClimatePolicyRequest& request) noexcept;
  PreviousClimateActions previousForInput() const noexcept;
  static bool isOff(const ClimatePolicyRequest& request) noexcept;

  ClimateProcess& runtime_;
  ClimateInputSource& input_source_;
  ClimateActuatorSink& actuator_sink_;
  PreviousClimateActions previous_applied_{};
  ClimateExecutionProjection external_previous_execution_{};
  bool has_external_previous_execution_ = false;
  bool actuator_fault_latched_ = false;
};

} // namespace growbox::climate
