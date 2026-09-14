#include "epd2_9/clay_ui/modules/menu/layout/MenuListLayout.h"

#include <clay/clay.h>

#include "epd2_9/clay_ui/render/ClayRenderer.h"
#include "epd2_9/clay_ui/support/ClayLayoutPrimitives.h"
#include "epd2_9/clay_ui/support/ClayTextUtils.h"

namespace epd2_9::clay_menu {

void renderMenuList(const MenuRenderContext& ctx) {
  const Clay_Color textColor = toClayColor(ctx.palette.cardText);
  const Clay_Color textSelected = toClayColor(ctx.palette.cardTextSelected);
  const Clay_Color cardColor = toClayColor(ctx.palette.card);
  const Clay_Color cardSelected = toClayColor(ctx.palette.cardSelected);
  const Clay_Color borderColor = toClayColor(ctx.palette.border);
  const TwoPaneLayoutMetrics metrics = defaultTwoPaneMenuMetrics();

  CLAY(CLAY_ID("MenuList"), {
      .layout = {
          .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
          .childGap = metrics.listRowGap,
          .layoutDirection = CLAY_TOP_TO_BOTTOM,
      },
  }) {
    for (size_t i = 0; i < ctx.items.size(); ++i) {
      const MenuItem& item = ctx.items[i];
      const bool isSelected = static_cast<int>(i) == ctx.selectedIndex;
      const Clay_Color bg = isSelected ? cardSelected : cardColor;
      const Clay_Color fg = isSelected ? textSelected : textColor;

      clay_layout::renderFixedRow(CLAY_IDI("MenuItem", static_cast<uint32_t>(i)),
                                  static_cast<float>(metrics.listRowHeight),
                                  metrics.listItemPadding,
                                  0,
                                  bg,
                                  borderColor,
                                  metrics.borderWidth,
                                  [&]() {
        Clay_String label = clay_text::makeStatic(item.label);
        clay_layout::renderClippedText(CLAY_IDI_LOCAL("MenuItemLabel", static_cast<uint32_t>(i)),
                                       label,
                                       fg,
                                       metrics.textFontId,
                                       metrics.previewLineHeight);
      });
    }
  }
}

}  // namespace epd2_9::clay_menu
