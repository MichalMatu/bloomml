// Input handling: crown, buttons, and board touch panels
#pragma once

#include <Arduino.h>

namespace epd2_9 {

struct Config; // fwd

enum class InputEventType {
  None = 0,
  CrownUp,
  CrownDown,
  CrownPress,
  CrownLongPress,
  Home,
  Back,
};

struct InputEvent {
  InputEventType type = InputEventType::None;
  uint32_t ms = 0;
  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  bool hasPosition = false;
};

// Human readable label for diagnostics/logging
const char* inputEventLabel(InputEventType type);

// Configure input GPIOs
void input_init(const Config& cfg);

// Poll for a debounced input event. Returns true if an event is produced.
bool input_poll(InputEvent& out);

} // namespace epd2_9
