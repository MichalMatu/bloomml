#pragma once

#include "climate/display/DisplayRuntime.h"

#include <cstdint>

namespace growbox::app::climate_io::display {

struct DisplayRenderWorkItem final {
  std::uint32_t generation{0U};
  DisplayRuntimeFrame frame{};
};

struct DisplayRenderCompletion final {
  std::uint32_t generation{0U};
  std::uint64_t rendered_at_ms{0U};
  bool success{false};
};

// Owner-thread state for one asynchronous hardware render. The worker only
// receives DisplayRenderWorkItem copies and returns a small completion token.
// Mutable presenter/runtime state never crosses the task boundary.
class DisplayAsyncTransaction final {
public:
  bool start(const DisplayRuntimeFrame& frame, DisplayRenderWorkItem& work_item) noexcept {
    if (in_flight_ || !frame.refreshRequired()) {
      return false;
    }

    ++next_generation_;
    if (next_generation_ == 0U) {
      ++next_generation_;
    }

    in_flight_frame_ = frame;
    in_flight_generation_ = next_generation_;
    in_flight_ = true;

    work_item = {};
    work_item.generation = in_flight_generation_;
    work_item.frame = in_flight_frame_;
    return true;
  }

  bool matches(const DisplayRenderCompletion& completion) const noexcept {
    return in_flight_ && completion.generation != 0U &&
           completion.generation == in_flight_generation_;
  }

  const DisplayRuntimeFrame& inFlightFrame() const noexcept {
    return in_flight_frame_;
  }

  bool finish(const DisplayRenderCompletion& completion) noexcept {
    if (!matches(completion)) {
      return false;
    }
    in_flight_ = false;
    in_flight_generation_ = 0U;
    return true;
  }

  void abort() noexcept {
    in_flight_ = false;
    in_flight_generation_ = 0U;
  }

  bool inFlight() const noexcept {
    return in_flight_;
  }

  std::uint32_t generation() const noexcept {
    return in_flight_generation_;
  }

private:
  DisplayRuntimeFrame in_flight_frame_{};
  std::uint32_t next_generation_{0U};
  std::uint32_t in_flight_generation_{0U};
  bool in_flight_{false};
};

} // namespace growbox::app::climate_io::display
