#pragma once

#include "IpoOutputPlan.h"
#include "climate/ClimateTypes.h"
#include "climate/application/ClimateSemanticOutput.h"
#include "climate/output/OutputTransport.h"
#include "climate/output/policy/OutputPolicyConfig.h"

namespace growbox::app::output {

// Adapter admission is a composition responsibility. IDs are stable for the
// installed equipment, independent of the graph's editor-local actuator IDs.
// Adapter-local endpoint numbers need not be globally unique.
struct OutputAdapterBinding {
  ::growbox::ipo::Driver driver = ::growbox::ipo::Driver::Rf433BinarySocket;
  std::uint32_t locator = 0U;
  OutputEndpointId endpoint = kInvalidOutputEndpoint;
  OutputEndpointId adapter_endpoint = kInvalidOutputEndpoint;
  OutputTransport* adapter = nullptr;
};
enum class EquipmentStatus : std::uint8_t {
  Ok, InvalidGraph, Empty, AdapterCapacity, InvalidAdapter,
  DuplicateAdapter, MissingAdapter
};

class ConfiguredOutputEquipment final {
public:
  using Bindings = std::array<OutputAdapterBinding, kOutputEndpointCapacity>;

  ConfiguredOutputEquipment(const ::growbox::ipo::Graph& graph,
                            const Bindings& bindings, std::size_t count) noexcept {
    status_ = configure(graph, bindings, count);
    if (!valid()) {
      routes_ = {};
      policy_ = {};
      semantic_ = {};
      capabilities_ = {};
    }
  }

  EquipmentStatus status() const noexcept { return status_; }
  bool valid() const noexcept { return status_ == EquipmentStatus::Ok; }
  const OutputPolicyConfig& policy() const noexcept { return policy_; }
  const climate_io::ClimateSemanticOutputConfig& semantic() const noexcept { return semantic_; }
  const ::growbox::climate::ClimateCapabilities& capabilities() const noexcept {
    return capabilities_;
  }
  const ::growbox::ipo::ConfiguredOutput* configured(OutputEndpointId endpoint) const noexcept {
    if (!valid()) return nullptr;
    for (std::size_t i = 0; i < policy_.count; ++i)
      if (routes_[i].binding.endpoint == endpoint) return &routes_[i].equipment;
    return nullptr;
  }
  const OutputAdapterBinding* route(OutputEndpointId endpoint) const noexcept {
    if (!valid()) return nullptr;
    for (std::size_t i = 0; i < policy_.count; ++i)
      if (routes_[i].binding.endpoint == endpoint) return &routes_[i].binding;
    return nullptr;
  }

private:
  using Role = ::growbox::ipo::OutputRole;
  struct Route {
    OutputAdapterBinding binding{};
    ::growbox::ipo::ConfiguredOutput equipment{};
  };
  static OutputEndpointRole policyRole(Role role) noexcept {
    switch (role) {
    case Role::Exhaust: return OutputEndpointRole::ExhaustFan;
    case Role::Light: return OutputEndpointRole::ScheduledLight;
    case Role::Humidifier: return OutputEndpointRole::Humidifier;
    case Role::Heater: return OutputEndpointRole::Heater;
    case Role::Cooler: return OutputEndpointRole::Cooler;
    case Role::Dehumidifier: return OutputEndpointRole::Dehumidifier;
    case Role::Co2Doser: return OutputEndpointRole::Co2Doser;
    default: return static_cast<OutputEndpointRole>(255U);
    }
  }
  bool bindSemantic(Role role, OutputEndpointId endpoint) noexcept {
    using C = climate_io::ClimateActuatorRole;
    switch (role) {
    case Role::Exhaust:
      capabilities_.exhaust_fan = true; return climate_io::bindClimateRole(semantic_, C::ExhaustFan, endpoint);
    case Role::Humidifier:
      capabilities_.humidifier = true; return climate_io::bindClimateRole(semantic_, C::Humidifier, endpoint);
    case Role::Heater:
      capabilities_.heater = true; return climate_io::bindClimateRole(semantic_, C::Heater, endpoint);
    case Role::Cooler:
      capabilities_.cooler = true; return climate_io::bindClimateRole(semantic_, C::Cooler, endpoint);
    case Role::Dehumidifier:
      capabilities_.dehumidifier = true; return climate_io::bindClimateRole(semantic_, C::Dehumidifier, endpoint);
    case Role::Co2Doser:
      capabilities_.co2_doser = true; return climate_io::bindClimateRole(semantic_, C::Co2Doser, endpoint);
    case Role::Light: return true;
    default: return false;
    }
  }
  EquipmentStatus configure(const ::growbox::ipo::Graph& graph, const Bindings& bindings,
                            std::size_t count) noexcept {
    const auto plan = ::growbox::ipo::buildOutputPlan(graph);
    if (!plan.valid()) return EquipmentStatus::InvalidGraph;
    // The normal supervisor requires a nonempty configuration. Zero roles
    // remain a valid draft but cannot construct an executable output owner.
    if (plan.count == 0U) return EquipmentStatus::Empty;
    if (count > bindings.size()) return EquipmentStatus::AdapterCapacity;
    for (std::size_t i = 0; i < count; ++i) {
      const auto& b = bindings[i];
      if (!isValidOutputEndpoint(b.endpoint) || !isValidOutputEndpoint(b.adapter_endpoint))
        return EquipmentStatus::InvalidAdapter;
      for (std::size_t j = 0; j < i; ++j) {
        const auto& a = bindings[j];
        if (a.endpoint == b.endpoint || (a.driver == b.driver && a.locator == b.locator) ||
            (a.adapter && a.adapter == b.adapter && a.adapter_endpoint == b.adapter_endpoint))
          return EquipmentStatus::DuplicateAdapter;
      }
    }
    policy_.count = static_cast<std::uint8_t>(plan.count);
    // Keep the existing lamp-first, fan-second fail-safe ordering; compact
    // the sequence for optional roles and append the remaining roles.
    constexpr Role order[]{Role::Light, Role::Exhaust, Role::Humidifier,
                           Role::Heater, Role::Cooler, Role::Dehumidifier, Role::Co2Doser};
    std::uint8_t lifecycle_order = 0U;
    for (Role role : order) {
      const auto* entry = plan.find(role);
      if (!entry) continue;
      const OutputAdapterBinding* selected = nullptr;
      for (std::size_t i = 0; i < count; ++i)
        if (bindings[i].driver == entry->driver && bindings[i].locator == entry->physical_locator)
          selected = &bindings[i];
      if (!selected || !selected->adapter) return EquipmentStatus::MissingAdapter;
      // Preserve graph order for persistence and the reference endpoint list.
      const auto index = static_cast<std::size_t>(entry - plan.entries.data());
      routes_[index] = {*selected, *entry};
      auto& endpoint = policy_.endpoints[index];
      endpoint.endpoint = selected->endpoint;
      endpoint.role = policyRole(role);
      for (auto& action : endpoint.lifecycle) action.order = lifecycle_order;
      ++lifecycle_order;
      if (role == Role::Light) {
        endpoint.lifecycle[outputLifecycleEventIndex(OutputLifecycleEvent::AutomationOff)]
            .action = OutputPolicyAction::ApplySchedule;
      }
      if (!bindSemantic(role, selected->endpoint)) return EquipmentStatus::InvalidGraph;
    }
    return validateOutputPolicyConfig(policy_) == OutputPolicyConfigStatus::Ok &&
           climate_io::validateClimateSemanticOutputConfig(semantic_) ==
               climate_io::ClimateSemanticOutputConfigStatus::Ok
               ? EquipmentStatus::Ok : EquipmentStatus::InvalidGraph;
  }

  EquipmentStatus status_ = EquipmentStatus::InvalidGraph;
  std::array<Route, kOutputEndpointCapacity> routes_{};
  OutputPolicyConfig policy_{};
  climate_io::ClimateSemanticOutputConfig semantic_{};
  ::growbox::climate::ClimateCapabilities capabilities_{};
};

// Routing is transport-only. No arbitration, no synthetic ACK, no state store.
class ConfiguredOutputTransport final : public OutputTransport {
public:
  explicit ConfiguredOutputTransport(const ConfiguredOutputEquipment& equipment) noexcept
      : equipment_(equipment) {}
  TxResult send(const OutputCommand& command) noexcept override {
    const auto* binding = equipment_.route(command.endpoint);
    if (!binding || !binding->adapter)
      return {TransportStatus::NotAttempted, TransportError::Unavailable};
    if (command.state != BinaryOutputState::Off && command.state != BinaryOutputState::On)
      return {TransportStatus::NotAttempted, TransportError::InvalidCommand};
    auto translated = command;
    translated.endpoint = binding->adapter_endpoint;
    return binding->adapter->send(translated);
  }
private:
  const ConfiguredOutputEquipment& equipment_;
};
} // namespace growbox::app::output
