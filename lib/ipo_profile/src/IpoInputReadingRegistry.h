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
