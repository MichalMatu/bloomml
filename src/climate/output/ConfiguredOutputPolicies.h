#pragma once
#include "climate/output/ConfiguredOutputEquipment.h"
#include "climate/output/supervisor/OutputSupervisorResolver.h"
#include <optional>

namespace growbox::app::output {

// Stable lifetime of per-endpoint dwell state; no transport access.
class ConfiguredOutputPolicies final {
public:
  explicit ConfiguredOutputPolicies(const ConfiguredOutputEquipment& equipment) noexcept {
    if (!equipment.valid()) return;
    const auto& policy = equipment.policy();
    for (std::size_t i = 0; i < policy.count; ++i) {
      const auto& endpoint = policy.endpoints[i];
      const auto* item = equipment.configured(endpoint.endpoint);
      if (!item) { config_ = {}; return; }
      const auto slot = endpoint.lifecycle[outputLifecycleEventIndex(OutputLifecycleEvent::Boot)].order;
      const bool lamp = endpoint.role == OutputEndpointRole::ScheduledLight;
      BinaryActuatorPolicy* binary = nullptr;
      if (!lamp || item->min_on_ms || item->min_off_ms) {
        const float on = lamp ? 0.5F : 0.10F;
        const float off = lamp ? 0.499F : 0.03F;
        binary = &policies_[slot].emplace(BinaryActuatorPolicyConfig{
            on, off, item->min_on_ms, item->min_off_ms});
      }
      config_.endpoints[slot] = {endpoint.endpoint, binary};
    }
    config_.count = policy.count;
  }
  ConfiguredOutputPolicies(const ConfiguredOutputPolicies&) = delete;
  ConfiguredOutputPolicies& operator=(const ConfiguredOutputPolicies&) = delete;
  const OutputSupervisorResolverConfig& config() const noexcept { return config_; }
private:
  std::array<std::optional<BinaryActuatorPolicy>, kOutputEndpointCapacity> policies_{};
  OutputSupervisorResolverConfig config_{};
};
} // namespace growbox::app::output
