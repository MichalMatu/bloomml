#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "epd2_9/clay_ui/ClayViewState.h"

namespace epd2_9::clay_menu {

struct MenuItem {
  const char* label;
  const char* previewLine1;
  const char* previewLine2;
  const char* previewLine3;
  const char* previewLine4;
  ClayViewMode targetView;
};

constexpr size_t kMenuItemCount = 4;
using MenuItemArray = std::array<MenuItem, kMenuItemCount>;

const MenuItemArray& menuItems();
const MenuItem* findMenuItemByView(ClayViewMode mode);
int findMenuIndexByView(ClayViewMode mode);

}  // namespace epd2_9::clay_menu
