#pragma once

#include "IpoGraph.h"

#include <cstddef>
#include <cstdint>

namespace growbox::ipo {

// Read-only admission for the CURRENT CrowPanel Mint transport/driver wiring.
// The v2 schema stays extensible. An admitted graph is NOT permission to apply
// it: runtime profile persistence, endpoint ownership and hardware qualification
// are separate gates.
enum class CrowPanelAdmissionStatus : std::uint8_t {
  CompatibleReferenceOnly,
  InvalidGraph,
  UnsupportedDriver,
  UnsupportedInputBinding,
  UnsupportedProcess,
  UnsupportedOutput,
  UnsafeDwell,
  MissingRequiredOutput,
  MissingRequiredInput,
  MissingRequiredLink
};
struct CrowPanelAdmission {
  CrowPanelAdmissionStatus status = CrowPanelAdmissionStatus::InvalidGraph;
  bool matches_compiled_board = false;
  bool apply_authorized = false;  // permanently false in this phase
};

inline Graph makeCrowPanelMintReferenceGraph() noexcept {
  Graph g{};
  // Physical locators are normalized board-local IDs: BLE aliases 1/2,
  // I2C 7-bit addresses, and RF433 socket numbers 1/2/3.
  g.devices[0] = {1U, Driver::Tp357, 1U, true};
  g.devices[1] = {2U, Driver::Scd41, 0x62U, true};
  g.devices[2] = {3U, Driver::Xiaomi, 2U, true};
  g.devices[3] = {4U, Driver::Ds3231, 0x68U, true};
  g.devices[4] = {5U, Driver::Rf433BinarySocket, 1U, true};
  g.devices[5] = {6U, Driver::Rf433BinarySocket, 2U, true};
  g.devices[6] = {7U, Driver::Rf433BinarySocket, 3U, true};
  g.device_count = 7U;

  g.signals[0] = {SignalRole::InsideTemperature, 1U, Channel::Temperature, 0U};
  g.signals[1] = {SignalRole::InsideHumidity, 1U, Channel::Humidity, 0U};
  g.signals[2] = {SignalRole::InsideCo2, 2U, Channel::Co2, 0U};
  g.signals[3] = {SignalRole::OutsideTemperature, 3U, Channel::Temperature, 0U};
  g.signals[4] = {SignalRole::OutsideHumidity, 3U, Channel::Humidity, 0U};
  g.signals[5] = {SignalRole::ClockTime, 4U, Channel::UnixTime, 0U};
  g.signal_count = 6U;

  g.actuators[0] = {101U, 5U, Channel::Switch, OutputRole::Exhaust, true, 120'000U, 120'000U};
  g.actuators[1] = {102U, 6U, Channel::Switch, OutputRole::Light, true, 0U, 0U};
  g.actuators[2] = {103U, 7U, Channel::Switch, OutputRole::Humidifier, true, 180'000U, 180'000U};
  g.actuator_count = 3U;

  g.processes[0] = {201U, ProcessKind::Temperature, true};
  g.processes[1] = {202U, ProcessKind::Humidity, true};
  g.processes[2] = {203U, ProcessKind::Co2Safety, true};
  g.processes[3] = {204U, ProcessKind::LightSchedule, true};
  g.process_count = 4U;

  g.links[0] = {201U, 101U, Effect::Ventilate};
  g.links[1] = {202U, 101U, Effect::Ventilate};
  g.links[2] = {202U, 103U, Effect::Humidify};
  g.links[3] = {203U, 101U, Effect::Ventilate};
  g.links[4] = {204U, 102U, Effect::Illuminate};
  g.link_count = 5U;
  return g;
}

inline CrowPanelAdmission assessCrowPanelReference(const Graph& graph) noexcept {
  CrowPanelAdmission result{};
  if (validateGraph(graph) != GraphStatus::Ok) {
    result.status = CrowPanelAdmissionStatus::InvalidGraph;
    return result;
  }

  for (std::size_t i = 0; i < graph.device_count; ++i) {
    const auto& d = graph.devices[i];
    if (!d.enabled) continue;
    switch (d.driver) {
      case Driver::Tp357:
        if (d.physical_locator != 1U) { result.status = CrowPanelAdmissionStatus::UnsupportedDriver; return result; }
        break;
      case Driver::Xiaomi:
        if (d.physical_locator != 2U) { result.status = CrowPanelAdmissionStatus::UnsupportedDriver; return result; }
        break;
      case Driver::Scd41:
        if (d.physical_locator != 0x62U) { result.status = CrowPanelAdmissionStatus::UnsupportedDriver; return result; }
        break;
      case Driver::Ds3231:
        if (d.physical_locator != 0x68U) { result.status = CrowPanelAdmissionStatus::UnsupportedDriver; return result; }
        break;
      case Driver::Rf433BinarySocket:
        if (d.physical_locator < 1U || d.physical_locator > 3U) {
          result.status = CrowPanelAdmissionStatus::UnsupportedDriver; return result;
        }
        break;
      default:
        result.status = CrowPanelAdmissionStatus::UnsupportedDriver;
        return result;
    }
  }

  const auto signalPermitted = [](SignalRole role, Driver driver, Channel channel) noexcept {
    switch (role) {
      case SignalRole::InsideTemperature:
        return driver == Driver::Tp357 && channel == Channel::Temperature;
      case SignalRole::InsideHumidity:
        return driver == Driver::Tp357 && channel == Channel::Humidity;
      case SignalRole::InsideCo2:
        return driver == Driver::Scd41 && channel == Channel::Co2;
      case SignalRole::OutsideTemperature:
        return driver == Driver::Xiaomi && channel == Channel::Temperature;
      case SignalRole::OutsideHumidity:
        return driver == Driver::Xiaomi && channel == Channel::Humidity;
      case SignalRole::ClockTime:
        return driver == Driver::Ds3231 && channel == Channel::UnixTime;
      default: return false;
    }
  };
  // The installed safety path must have a real CO2 input, not just an
  // enabled co2_safety process and exhaust link. Missing/stale readings are
  // still invalid at runtime; this checks hardware wiring only.
  constexpr SignalRole required[]{
      SignalRole::InsideTemperature, SignalRole::InsideHumidity,
      SignalRole::InsideCo2, SignalRole::ClockTime
  };
  for (const auto role : required) {
    bool found = false;
    for (std::size_t i = 0; i < graph.signal_count; ++i) {
      const auto& binding = graph.signals[i];
      const auto* device = findDevice(graph, binding.device_id);
      if (binding.role == role && device != nullptr && device->enabled &&
          binding.priority == 0U && signalPermitted(binding.role, device->driver, binding.channel)) {
        found = true;
        break;
      }
    }
    if (!found) {
      result.status = CrowPanelAdmissionStatus::MissingRequiredInput;
      return result;
    }
  }
  for (std::size_t i = 0; i < graph.signal_count; ++i) {
    const auto& binding = graph.signals[i];
    const auto* device = findDevice(graph, binding.device_id);
    if (device == nullptr || !device->enabled || binding.priority != 0U ||
        !signalPermitted(binding.role, device->driver, binding.channel)) {
      result.status = CrowPanelAdmissionStatus::UnsupportedInputBinding;
      return result;
    }
  }

  for (std::size_t i = 0; i < graph.process_count; ++i) {
    const auto kind = graph.processes[i].kind;
    if (kind != ProcessKind::Temperature && kind != ProcessKind::Humidity &&
        kind != ProcessKind::Co2Safety && kind != ProcessKind::LightSchedule) {
      result.status = CrowPanelAdmissionStatus::UnsupportedProcess;
      return result;
    }
  }
  if (graph.actuator_count != 3U) {
    result.status = CrowPanelAdmissionStatus::MissingRequiredOutput;
    return result;
  }
  bool roles_seen[3] = {false, false, false};
  for (std::size_t i = 0; i < graph.actuator_count; ++i) {
    const auto& a = graph.actuators[i];
    const auto* d = findDevice(graph, a.device_id);
    if (d == nullptr || !d->enabled || d->driver != Driver::Rf433BinarySocket ||
        a.channel != Channel::Switch || !a.safe_off) {
      result.status = CrowPanelAdmissionStatus::UnsupportedOutput;
      return result;
    }
    std::size_t index = 0U;
    std::uint32_t minimum_ms = 0U;
    if (a.role == OutputRole::Exhaust && d->physical_locator == 1U) {
      index = 0U; minimum_ms = 120'000U;
    } else if (a.role == OutputRole::Light && d->physical_locator == 2U) {
      index = 1U;
    } else if (a.role == OutputRole::Humidifier && d->physical_locator == 3U) {
      index = 2U; minimum_ms = 180'000U;
    } else {
      result.status = CrowPanelAdmissionStatus::UnsupportedOutput; return result;
    }
    if (roles_seen[index]) {
      result.status = CrowPanelAdmissionStatus::UnsupportedOutput;
      return result;
    }
    roles_seen[index] = true;
    if (a.min_on_ms < minimum_ms || a.min_off_ms < minimum_ms) {
      result.status = CrowPanelAdmissionStatus::UnsafeDwell; return result;
    }
  }
  if (!roles_seen[0] || !roles_seen[1] || !roles_seen[2]) {
    result.status = CrowPanelAdmissionStatus::MissingRequiredOutput;
    return result;
  }
  // A profile can have valid endpoints yet omit a safety-critical route.
  // This admission requires the known Mint process graph, not merely three
  // correctly addressed sockets. Physical Safety remains independent.
  struct RequiredRoute {
    ProcessKind process;
    OutputRole output;
    Effect effect;
  };
  constexpr RequiredRoute required_routes[]{
      {ProcessKind::Temperature, OutputRole::Exhaust, Effect::Ventilate},
      {ProcessKind::Humidity, OutputRole::Exhaust, Effect::Ventilate},
      {ProcessKind::Humidity, OutputRole::Humidifier, Effect::Humidify},
      {ProcessKind::Co2Safety, OutputRole::Exhaust, Effect::Ventilate},
      {ProcessKind::LightSchedule, OutputRole::Light, Effect::Illuminate},
  };
  for (const auto& required : required_routes) {
    bool found = false;
    for (std::size_t i = 0; i < graph.link_count && !found; ++i) {
      const auto& link = graph.links[i];
      const auto* process = findProcess(graph, link.process_id);
      const auto* output = findActuator(graph, link.actuator_id);
      if (process != nullptr && process->enabled && output != nullptr &&
          process->kind == required.process && output->role == required.output &&
          link.effect == required.effect) {
        found = true;
      }
    }
    if (!found) {
      result.status = CrowPanelAdmissionStatus::MissingRequiredLink;
      return result;
    }
  }
  result.status = CrowPanelAdmissionStatus::CompatibleReferenceOnly;
  result.matches_compiled_board = true;
  return result;
}

} // namespace growbox::ipo
