#include "climate/display/input/DisplayButtonStateMachine.h"

namespace growbox::app::climate_io::display {

void DisplayButtonStateMachine::reset(bool raw_pressed, std::uint64_t now_ms) noexcept {
  candidate_pressed_ = raw_pressed;
  stable_pressed_ = raw_pressed;
  candidate_since_ms_ = now_ms;
  pressed_since_ms_ = now_ms;
  long_press_emitted_ = false;
  initialized_ = true;
}

bool DisplayButtonStateMachine::elapsedAtLeast(std::uint64_t now_ms, std::uint64_t since_ms,
                                               std::uint32_t duration_ms) noexcept {
  return now_ms >= since_ms && now_ms - since_ms >= duration_ms;
}

bool DisplayButtonStateMachine::update(DisplayButton button, bool raw_pressed,
                                       std::uint64_t now_ms,
                                       DisplayButtonEvent& output) noexcept {
  output = {};
  if (!initialized_) {
    reset(raw_pressed, now_ms);
    return false;
  }

  if (raw_pressed != candidate_pressed_) {
    candidate_pressed_ = raw_pressed;
    candidate_since_ms_ = now_ms;
  }

  if (candidate_pressed_ != stable_pressed_ &&
      elapsedAtLeast(now_ms, candidate_since_ms_, timing_.debounce_ms)) {
    stable_pressed_ = candidate_pressed_;
    if (stable_pressed_) {
      pressed_since_ms_ = candidate_since_ms_;
      long_press_emitted_ = false;
    } else {
      if (!long_press_emitted_) {
        output = {button, DisplayButtonGesture::Press, now_ms};
        long_press_emitted_ = false;
        return true;
      }
      long_press_emitted_ = false;
    }
  }

  // A candidate release pauses long-press detection until the input is either
  // confirmed released or returns stably pressed. This avoids a long press being
  // emitted while the physical contact is already opening/bouncing.
  if (stable_pressed_ && candidate_pressed_ && !long_press_emitted_ &&
      elapsedAtLeast(now_ms, pressed_since_ms_, timing_.long_press_ms)) {
    long_press_emitted_ = true;
    output = {button, DisplayButtonGesture::LongPress, now_ms};
    return true;
  }

  return false;
}

} // namespace growbox::app::climate_io::display
