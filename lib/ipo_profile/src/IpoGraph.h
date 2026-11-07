// BloomML IPO Foundation v2: portable, bounded, NO hardware I/O.
// This module is intentionally not linked to production runtime yet. Its proposals
// MUST be passed through validated runtime profile ownership, Safety and
// OutputSupervisor before any future physical execution.
#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace growbox::ipo {

inline constexpr std::size_t kMaxDevices = 24U;
inline constexpr std::size_t kMaxSignals = 16U; // semantic roles
inline constexpr std::size_t kMaxSignalSources = kMaxSignals * 4U; // up to 4 sources per role
inline constexpr std::size_t kMaxActuators = 24U;
inline constexpr std::size_t kMaxProcesses = 16U;
inline constexpr std::size_t kMaxLinks = 48U;

enum class Driver : std::uint8_t {
  Tp357, Xiaomi, Scd41, Ds3231, Bme280, Bh1750, SoilAdc,
  Rf433BinarySocket, GpioRelay, PwmChannel, ModbusRelay
};
enum class Channel : std::uint8_t {
  Temperature, Humidity, Co2, UnixTime, Illuminance, SoilMoisture, Switch, Duty
};
enum class SignalRole : std::uint8_t {
  InsideTemperature, InsideHumidity, InsideCo2, OutsideTemperature,
  OutsideHumidity, ClockTime, LightIlluminance, SubstrateMoisture
};
enum class OutputRole : std::uint8_t {
  Exhaust, Humidifier, Light, Heater, Cooler, Dehumidifier,
  Co2Doser, CirculationFan, IrrigationPump
};
enum class ProcessKind : std::uint8_t {
  Temperature, Humidity, Co2Safety, Co2Target, LightSchedule, Circulation, Irrigation
};
enum class Effect : std::uint8_t {
  Ventilate, Humidify, Illuminate, Heat, Cool, Dehumidify,
  DoseCo2, Circulate, Irrigate
};

struct Device {
  std::uint16_t id = 0;
  Driver driver = Driver::Tp357;
  std::uint32_t physical_locator = 0;  // Parsed/normalized by a future vetted adapter.
  bool enabled = false;
};
struct SignalBinding {
  SignalRole role = SignalRole::InsideTemperature;
  std::uint16_t device_id = 0;
  Channel channel = Channel::Temperature;
  std::uint8_t priority = 0;  // 0 = primary, 1 = fallback, never overwrite valid/fresh.
};
struct ActuatorBinding {
  std::uint16_t id = 0;
  std::uint16_t device_id = 0;
  Channel channel = Channel::Switch;
  OutputRole role = OutputRole::Exhaust;
  bool safe_off = true;
  std::uint32_t min_on_ms = 0;
  std::uint32_t min_off_ms = 0;
};
struct Process {
  std::uint16_t id = 0;
  ProcessKind kind = ProcessKind::Temperature;
  bool enabled = false;
};
struct Link {
  std::uint16_t process_id = 0;
  std::uint16_t actuator_id = 0;
  Effect effect = Effect::Ventilate;
};
struct Graph {
  std::array<Device, kMaxDevices> devices{};
  std::array<SignalBinding, kMaxSignalSources> signals{};
  std::array<ActuatorBinding, kMaxActuators> actuators{};
  std::array<Process, kMaxProcesses> processes{};
  std::array<Link, kMaxLinks> links{};
  std::uint8_t device_count = 0;
  std::uint8_t signal_count = 0;
  std::uint8_t actuator_count = 0;
  std::uint8_t process_count = 0;
  std::uint8_t link_count = 0;
};
enum class GraphStatus : std::uint8_t {
  Ok, Capacity, UnknownDevice, UnknownProcess, UnknownActuator,
  DuplicateId, DuplicatePhysicalEndpoint, DuplicateSignalPriority,
  ChannelMismatch, IncompatibleEffect, UnsafeOutput, MissingCo2Safety
};

constexpr bool driverProvides(Driver driver, Channel channel) noexcept {
  switch (driver) {
    case Driver::Tp357: case Driver::Xiaomi:
    case Driver::Bme280: return channel == Channel::Temperature || channel == Channel::Humidity;
    case Driver::Scd41:
      return channel == Channel::Temperature || channel == Channel::Humidity || channel == Channel::Co2;
    case Driver::Ds3231: return channel == Channel::UnixTime;
    case Driver::Bh1750: return channel == Channel::Illuminance;
    case Driver::SoilAdc: return channel == Channel::SoilMoisture;
    case Driver::Rf433BinarySocket: case Driver::GpioRelay:
    case Driver::ModbusRelay: return channel == Channel::Switch;
    case Driver::PwmChannel: return channel == Channel::Duty;
  }
  return false;
}
constexpr bool accepts(SignalRole role, Channel channel) noexcept {
  switch (role) {
    case SignalRole::InsideTemperature: case SignalRole::OutsideTemperature:
      return channel == Channel::Temperature;
    case SignalRole::InsideHumidity: case SignalRole::OutsideHumidity:
      return channel == Channel::Humidity;
    case SignalRole::InsideCo2: return channel == Channel::Co2;
    case SignalRole::ClockTime: return channel == Channel::UnixTime;
    case SignalRole::LightIlluminance: return channel == Channel::Illuminance;
    case SignalRole::SubstrateMoisture: return channel == Channel::SoilMoisture;
  }
  return false;
}
constexpr bool matches(Effect effect, OutputRole role) noexcept {
  switch (effect) {
    case Effect::Ventilate: return role == OutputRole::Exhaust;
    case Effect::Humidify: return role == OutputRole::Humidifier;
    case Effect::Illuminate: return role == OutputRole::Light;
    case Effect::Heat: return role == OutputRole::Heater;
    case Effect::Cool: return role == OutputRole::Cooler;
    case Effect::Dehumidify: return role == OutputRole::Dehumidifier;
    case Effect::DoseCo2: return role == OutputRole::Co2Doser;
    case Effect::Circulate: return role == OutputRole::CirculationFan;
    case Effect::Irrigate: return role == OutputRole::IrrigationPump;
  }
  return false;
}
constexpr bool permitted(ProcessKind kind, Effect effect) noexcept {
  switch (kind) {
    case ProcessKind::Temperature:
      return effect == Effect::Ventilate || effect == Effect::Heat || effect == Effect::Cool;
    case ProcessKind::Humidity:
      return effect == Effect::Ventilate || effect == Effect::Humidify || effect == Effect::Dehumidify;
    case ProcessKind::Co2Safety: return effect == Effect::Ventilate;
    case ProcessKind::Co2Target: return effect == Effect::DoseCo2;
    case ProcessKind::LightSchedule: return effect == Effect::Illuminate;
    case ProcessKind::Circulation: return effect == Effect::Circulate;
    case ProcessKind::Irrigation: return effect == Effect::Irrigate;
  }
  return false;
}
inline const Device* findDevice(const Graph& graph, std::uint16_t id) noexcept {
  for (std::size_t i = 0; i < graph.device_count && i < kMaxDevices; ++i) {
    if (graph.devices[i].id == id) return &graph.devices[i];
  }
  return nullptr;
}
inline const ActuatorBinding* findActuator(const Graph& graph, std::uint16_t id) noexcept {
  for (std::size_t i = 0; i < graph.actuator_count && i < kMaxActuators; ++i) {
    if (graph.actuators[i].id == id) return &graph.actuators[i];
  }
  return nullptr;
}
inline const Process* findProcess(const Graph& graph, std::uint16_t id) noexcept {
  for (std::size_t i = 0; i < graph.process_count && i < kMaxProcesses; ++i) {
    if (graph.processes[i].id == id) return &graph.processes[i];
  }
  return nullptr;
}
inline GraphStatus validateGraph(const Graph& graph) noexcept {
  if (graph.device_count > kMaxDevices || graph.signal_count > kMaxSignalSources ||
      graph.actuator_count > kMaxActuators || graph.process_count > kMaxProcesses ||
      graph.link_count > kMaxLinks) return GraphStatus::Capacity;
  for (std::size_t i = 0; i < graph.device_count; ++i) {
    const auto& d = graph.devices[i];
    if (d.id == 0U) return GraphStatus::DuplicateId;
    for (std::size_t j = 0; j < i; ++j) {
      if (graph.devices[j].id == d.id) return GraphStatus::DuplicateId;
      if (d.enabled && graph.devices[j].enabled && graph.devices[j].driver == d.driver &&
          graph.devices[j].physical_locator == d.physical_locator)
        return GraphStatus::DuplicatePhysicalEndpoint;
    }
  }
  for (std::size_t i = 0; i < graph.signal_count; ++i) {
    const auto& s = graph.signals[i];
    const auto* device = findDevice(graph, s.device_id);
    if (!device) return GraphStatus::UnknownDevice;
    if (!driverProvides(device->driver, s.channel) || !accepts(s.role, s.channel))
      return GraphStatus::ChannelMismatch;
    for (std::size_t j = 0; j < i; ++j) {
      const auto& prev = graph.signals[j];
      if (prev.role == s.role && prev.priority == s.priority)
        return GraphStatus::DuplicateSignalPriority;
    }
  }
  for (std::size_t i = 0; i < graph.actuator_count; ++i) {
    const auto& a = graph.actuators[i];
    if (a.id == 0U || !a.safe_off) return GraphStatus::UnsafeOutput;
    const auto* device = findDevice(graph, a.device_id);
    if (!device) return GraphStatus::UnknownDevice;
    if (!driverProvides(device->driver, a.channel) ||
        (a.channel != Channel::Switch && a.channel != Channel::Duty))
      return GraphStatus::ChannelMismatch;
    for (std::size_t j = 0; j < i; ++j) {
      const auto& prev = graph.actuators[j];
      if (prev.id == a.id) return GraphStatus::DuplicateId;
      if (prev.device_id == a.device_id && prev.channel == a.channel)
        return GraphStatus::DuplicatePhysicalEndpoint;
    }
  }
  bool co2_safety_present = false;
  for (std::size_t i = 0; i < graph.process_count; ++i) {
    const auto& p = graph.processes[i];
    if (p.id == 0U) return GraphStatus::DuplicateId;
    if (p.kind == ProcessKind::Co2Safety) {
      if (co2_safety_present || !p.enabled) return GraphStatus::MissingCo2Safety;
      co2_safety_present = true;
    }
    for (std::size_t j = 0; j < i; ++j)
      if (graph.processes[j].id == p.id) return GraphStatus::DuplicateId;
  }
  if (!co2_safety_present) return GraphStatus::MissingCo2Safety;
  for (std::size_t i = 0; i < graph.link_count; ++i) {
    const auto& l = graph.links[i];
    const auto* p = findProcess(graph, l.process_id);
    const auto* a = findActuator(graph, l.actuator_id);
    if (!p) return GraphStatus::UnknownProcess;
    if (!a) return GraphStatus::UnknownActuator;
    if (!permitted(p->kind, l.effect) || !matches(l.effect, a->role))
      return GraphStatus::IncompatibleEffect;
    for (std::size_t j = 0; j < i; ++j) {
      const auto& prev = graph.links[j];
      if (prev.process_id == l.process_id && prev.actuator_id == l.actuator_id &&
          prev.effect == l.effect) return GraphStatus::DuplicateId;
    }
  }
  return GraphStatus::Ok;
}

// Symbolic proposals: no device transport, safety bypass or direct switch commands.
struct Proposal {
  std::uint16_t process_id = 0;
  std::uint16_t actuator_id = 0;
  Effect effect = Effect::Ventilate;
  float level = 0.0F;
};
struct OutputIntentPreview {
  std::uint16_t actuator_id = 0;
  float level = 0.0F;
  std::uint8_t contributor_count = 0;
};
struct PreviewResult {
  std::array<OutputIntentPreview, kMaxActuators> intents{};
  std::uint8_t count = 0;
  std::uint16_t rejected = 0;
  GraphStatus status = GraphStatus::Ok;
};
inline PreviewResult previewCompatibleIntents(
    const Graph& graph, const Proposal* proposals, std::size_t proposal_count) noexcept {
  PreviewResult result{};
  result.status = validateGraph(graph);
  if (result.status != GraphStatus::Ok) return result;
  if (proposal_count > kMaxLinks || (proposal_count != 0U && proposals == nullptr)) {
    result.status = GraphStatus::Capacity;
    return result;
  }
  for (std::size_t i = 0; i < proposal_count; ++i) {
    const Proposal& q = proposals[i];
    const auto* process = findProcess(graph, q.process_id);
    const auto* actuator = findActuator(graph, q.actuator_id);
    const auto* device = actuator ? findDevice(graph, actuator->device_id) : nullptr;
    bool linked = false;
    for (std::size_t k = 0; k < graph.link_count; ++k) {
      const auto& link = graph.links[k];
      if (link.process_id == q.process_id && link.actuator_id == q.actuator_id &&
          link.effect == q.effect) { linked = true; break; }
    }
    // Safety proposals cannot enter the generic mixer; runtime safety owns them separately.
    if (!process || !process->enabled || process->kind == ProcessKind::Co2Safety ||
        !actuator || !device || !device->enabled || !linked ||
        !std::isfinite(q.level) || q.level < 0.0F || q.level > 1.0F) {
      ++result.rejected;
      continue;
    }
    std::size_t index = result.count;
    for (std::size_t k = 0; k < result.count; ++k) {
      if (result.intents[k].actuator_id == q.actuator_id) { index = k; break; }
    }
    if (index == result.count) {
      if (result.count == kMaxActuators) { ++result.rejected; continue; }
      result.intents[result.count++].actuator_id = q.actuator_id;
    }
    auto& intent = result.intents[index];
    if (q.level > intent.level) intent.level = q.level;
    if (intent.contributor_count < 255U) ++intent.contributor_count;
  }
  return result;
}

} // namespace growbox::ipo
