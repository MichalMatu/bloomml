#pragma once

#include <cstdint>

namespace growbox::app::climate_io::display {

enum class DisplayButton : std::uint8_t {
  Home = 0U,
  Back,
  Previous,
  Next,
  Ok,
};

enum class DisplayButtonGesture : std::uint8_t {
  Press = 0U,
  LongPress,
};

struct DisplayButtonEvent final {
  DisplayButton button{DisplayButton::Home};
  DisplayButtonGesture gesture{DisplayButtonGesture::Press};
  std::uint64_t timestamp_ms{0U};
};

} // namespace growbox::app::climate_io::display
