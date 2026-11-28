#pragma once

#include "IpoSignalSelector.h"

namespace growbox::ipo {

// One bounded observation frame over the existing DeviceRegistry (Graph).
// No device discovery, ownership of drivers, Process or Output dependency.
// A bad/duplicate registration poisons the frame until the next clear().
class InputReadingRegistry {
public:
  explicit InputReadingRegistry(const Graph& graph) noexcept : graph_(graph) {}

  void clear() noexcept {
    count_ = 0U;
    error_ = SignalSelectionStatus::Ok;
  }

  // Resolve the configured instance by physical driver identity, not by Mint ID.
  const Device* installedDevice(Driver driver, std::uint32_t locator) const noexcept {
    if (graph_.device_count > kMaxDevices) return nullptr;
    for (std::size_t i = 0; i < graph_.device_count; ++i) {
      const auto& device = graph_.devices[i];
      if (device.enabled && device.driver == driver && device.physical_locator == locator)
        return &device;
    }
    return nullptr;
  }

  bool hasDevice(Driver driver, std::uint32_t locator) const noexcept {
    return installedDevice(driver, locator) != nullptr;
  }

  bool recordFor(Driver driver, std::uint32_t locator, Channel channel, double value,
                 std::uint64_t age_ms, bool valid) noexcept {
    const auto* device = installedDevice(driver, locator);
    if (device == nullptr) return false;
    return record({device->id, channel, value, age_ms, valid});
  }

  bool record(const SignalReading& reading) noexcept {
    if (error_ != SignalSelectionStatus::Ok) return false;
    const auto* device = findDevice(graph_, reading.device_id);
    if (device == nullptr || !device->enabled ||
        !driverProvides(device->driver, reading.channel)) {
      error_ = SignalSelectionStatus::InvalidGraph;
      return false;
    }
    for (std::size_t i = 0; i < count_; ++i) {
      if (readings_[i].device_id == reading.device_id &&
          readings_[i].channel == reading.channel) {
        error_ = SignalSelectionStatus::AmbiguousReading;
        return false;
      }
    }
    if (count_ == readings_.size()) {
      error_ = SignalSelectionStatus::Capacity;
      return false;
    }
    readings_[count_++] = reading;
    return true;
  }

  SignalSelection select(SignalRole role, std::uint64_t timeout_ms) const noexcept {
    if (error_ != SignalSelectionStatus::Ok) {
      SignalSelection result{};
      result.status = error_;
      return result;
    }
    return selectFreshSignal(graph_, role, readings_.data(), count_, timeout_ms);
  }

private:
  const Graph& graph_;
  std::array<SignalReading, kMaxSignalSources> readings_{};
  std::size_t count_ = 0U;
  SignalSelectionStatus error_ = SignalSelectionStatus::Ok;
};

} // namespace growbox::ipo
