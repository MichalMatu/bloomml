#pragma once

#include "epd2_9/clay_ui/support/ClayTheme.h"
#include "epd2_9/clay_ui/modules/menu/model/MenuModel.h"

namespace epd2_9::clay_menu {

struct MenuRenderContext {
  const MenuItemArray& items;
  int selectedIndex;
  int32_t scrollOffset;
  const ClayPalette& palette;
  const ClayLayoutMetrics& metrics;
  int16_t displayWidth;
};

void renderMenu(const MenuRenderContext& ctx);

}  // namespace epd2_9::clay_menu
