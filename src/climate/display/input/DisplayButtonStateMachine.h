#pragma once

#include "climate/display/DisplayInputEvent.h"

#include <cstdint>

namespace growbox::app::climate_io::display {

struct DisplayButtonTiming final {
  std::uint32_t debounce_ms{40U};
  std::uint32_t long_press_ms{700U};
};

// Pure, host-testable single-key state machine. Raw state must remain unchanged
// for debounce_ms before it becomes the new stable state. Short presses are
// emitted on the stable release edge; a long press suppresses that later short
// press so one physical gesture produces exactly one semantic event.
class DisplayButtonStateMachine final {
public:
  explicit DisplayButtonStateMachine(const DisplayButtonTiming& timing = {}) noexcept
      : timing_(timing) {}

  void reset(bool raw_pressed, std::uint64_t now_ms) noexcept;

  bool update(DisplayButton button, bool raw_pressed, std::uint64_t now_ms,
              DisplayButtonEvent& output) noexcept;

  bool initialized() const noexcept {
    return initialized_;
  }

  bool stablePressed() const noexcept {
    return stable_pressed_;
  }

private:
  static bool elapsedAtLeast(std::uint64_t now_ms, std::uint64_t since_ms,
                             std::uint32_t duration_ms) noexcept;

  DisplayButtonTiming timing_{};
  std::uint64_t candidate_since_ms_{0U};
  std::uint64_t pressed_since_ms_{0U};
  bool candidate_pressed_{false};
  bool stable_pressed_{false};
  bool long_press_emitted_{false};
  bool initialized_{false};
};

} // namespace growbox::app::climate_io::display
