#pragma once

#include "IpoGraph.h"

namespace growbox::ipo {

// Bounded binary equipment projection. No adapter, physical IO, activation,
// mutable execution state or Process dependency.
inline constexpr std::size_t kMvpOutputRoleCount = 7U;
enum class OutputPlanStatus : std::uint8_t {
  Ok, InvalidGraph, Capacity, UnsupportedRole, UnsupportedChannel,
  DisabledDevice, DuplicateRole, InvalidEndpoint
};
struct ConfiguredOutput {
  std::uint16_t endpoint = 0U;
  std::uint16_t device_id = 0U;
  OutputRole role = OutputRole::Exhaust;
  Driver driver = Driver::Rf433BinarySocket;
  std::uint32_t physical_locator = 0U;
  std::uint32_t min_on_ms = 0U;
  std::uint32_t min_off_ms = 0U;
};
struct OutputPlan {
  OutputPlanStatus status = OutputPlanStatus::InvalidGraph;
  std::array<ConfiguredOutput, kMvpOutputRoleCount> entries{};
  std::size_t count = 0U;
  bool valid() const noexcept { return status == OutputPlanStatus::Ok; }
  const ConfiguredOutput* find(OutputRole role) const noexcept {
    if (!valid()) return nullptr;
    for (std::size_t i = 0; i < count; ++i)
      if (entries[i].role == role) return &entries[i];
    return nullptr;
  }
};
inline OutputPlan buildOutputPlan(const Graph& graph) noexcept {
  const auto reject = [](OutputPlanStatus status) {
    OutputPlan result{};
    result.status = status;
    return result;
  };
  if (validateGraph(graph) != GraphStatus::Ok)
    return reject(OutputPlanStatus::InvalidGraph);
  if (graph.actuator_count > kMvpOutputRoleCount)
    return reject(OutputPlanStatus::Capacity);
  OutputPlan result{};
  result.status = OutputPlanStatus::Ok;
  for (std::size_t i = 0; i < graph.actuator_count; ++i) {
    const auto& binding = graph.actuators[i];
    if (static_cast<std::size_t>(binding.role) >= kMvpOutputRoleCount)
      return reject(OutputPlanStatus::UnsupportedRole);
    if (binding.channel != Channel::Switch)
      return reject(OutputPlanStatus::UnsupportedChannel);
    if (binding.id == UINT16_MAX)
      return reject(OutputPlanStatus::InvalidEndpoint);
    const auto* device = findDevice(graph, binding.device_id);
    if (!device || !device->enabled)
      return reject(OutputPlanStatus::DisabledDevice);
    if (result.find(binding.role))
      return reject(OutputPlanStatus::DuplicateRole);
    result.entries[result.count++] = {binding.id, device->id, binding.role,
        device->driver, device->physical_locator, binding.min_on_ms, binding.min_off_ms};
  }
  return result;
}
} // namespace growbox::ipo
