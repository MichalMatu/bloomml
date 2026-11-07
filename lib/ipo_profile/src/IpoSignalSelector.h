#pragma once

#include "IpoGraph.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace growbox::ipo {

// A typed, bounded selection primitive, NOT a sensor driver and NOT permission
// for a sensor role to replace hardware-critical safety measurements.
// Firmware must separately qualify the specific source/role before enabling
// the fallback. Clock Unix seconds use double instead of lossy float.
struct SignalReading {
  std::uint16_t device_id = 0;
  Channel channel = Channel::Temperature;
  double value = 0.0;
  std::uint64_t age_ms = 0;
  bool valid = false;
};

enum class SignalSelectionStatus : std::uint8_t {
  Ok, MissingRole, NoFreshSource, InvalidGraph, AmbiguousReading, Capacity
};

struct SignalSelection {
  SignalSelectionStatus status = SignalSelectionStatus::MissingRole;
  std::uint16_t device_id = 0;
  Channel channel = Channel::Temperature;
  double value = 0.0;
  std::uint64_t age_ms = std::numeric_limits<std::uint64_t>::max();
  std::uint8_t source_priority = 255U;
  bool fallback = false;

  bool usable() const noexcept { return status == SignalSelectionStatus::Ok; }
};

// Selection does not synthesize a zero for unavailable measurements.
// Order is the graph's per-role priority, not the order in the reading buffer.
inline SignalSelection selectFreshSignal(const Graph& graph, SignalRole role,
                                         const SignalReading* readings, std::size_t reading_count,
                                         std::uint64_t timeout_ms) noexcept {
  SignalSelection result{};
  if (graph.signal_count > kMaxSignalSources || graph.device_count > kMaxDevices ||
      (reading_count > 0U && readings == nullptr)) {
    result.status = SignalSelectionStatus::Capacity;
    return result;
  }
  // Only use graph validation once per admitted installation in the eventual
  // hot path. Here we fail closed on raw untrusted/malformed graphs.
  if (validateGraph(graph) != GraphStatus::Ok) {
    result.status = SignalSelectionStatus::InvalidGraph;
    return result;
  }

  bool role_exists = false;
  for (std::uint16_t priority = 0U; priority < kMaxSourcesPerRole; ++priority) {
    for (std::size_t i = 0; i < graph.signal_count; ++i) {
      const auto& binding = graph.signals[i];
      if (binding.role != role) continue;
      role_exists = true;
      if (binding.priority != priority) continue;
      const auto* device = findDevice(graph, binding.device_id);
      if (device == nullptr || !device->enabled) continue;

      bool matching_reading_seen = false;
      SignalReading candidate{};
      for (std::size_t j = 0; j < reading_count; ++j) {
        const auto& current = readings[j];
        if (current.device_id != binding.device_id || current.channel != binding.channel) continue;
        if (matching_reading_seen) {
          result.status = SignalSelectionStatus::AmbiguousReading;
          return result;
        }
        matching_reading_seen = true;
        candidate = current;
      }
      if (!matching_reading_seen || !candidate.valid ||
          candidate.age_ms > timeout_ms || !std::isfinite(candidate.value)) continue;
      result.status = SignalSelectionStatus::Ok;
      result.device_id = candidate.device_id;
      result.channel = candidate.channel;
      result.value = candidate.value;
      result.age_ms = candidate.age_ms;
      result.source_priority = binding.priority;
      result.fallback = binding.priority > 0U;
      return result;
    }
  }
  result.status = role_exists ? SignalSelectionStatus::NoFreshSource
                              : SignalSelectionStatus::MissingRole;
  return result;
}

} // namespace growbox::ipo
