#include "epd2_9/clay_ui/modules/menu/model/MenuModel.h"

namespace epd2_9::clay_menu {
namespace {

const MenuItemArray kMenuItems = {{
    {"WiFi", "Status", "Power / mode", "", "", ClayViewMode::WifiOverview},
    {"Automation", "Engine mode", "AUTO / MANUAL", "", "", ClayViewMode::AutomationSettings},
    {"System", "Time", "Device controls", "", "", ClayViewMode::SystemSettings},
    {"Back", "Return", "Dashboard", "", "", ClayViewMode::Home},
}};

}  // namespace

const MenuItemArray& menuItems() { return kMenuItems; }

const MenuItem* findMenuItemByView(ClayViewMode mode) {
  for (const auto& item : kMenuItems) {
    if (item.targetView == mode) {
      return &item;
    }
  }
  return nullptr;
}

int findMenuIndexByView(ClayViewMode mode) {
  for (size_t i = 0; i < kMenuItems.size(); ++i) {
    if (kMenuItems[i].targetView == mode) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace epd2_9::clay_menu
