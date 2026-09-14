#pragma once

#include <cstdint>

namespace epd2_9 {

enum class ClayViewMode : uint8_t {
  Home = 0,
  Menu,
  WifiOverview,
  AutomationSettings,
  SetTime,
  SystemSettings,
  WifiProfileSettings,
  WifiStatus,
};

}  // namespace epd2_9
