#include "epd2_9/clay_ui/modules/menu/layout/MenuLayout.h"

#include <clay/clay.h>

#include "epd2_9/clay_ui/modules/menu/layout/MenuListLayout.h"
#include "epd2_9/clay_ui/modules/menu/preview/MenuPreviewLayout.h"
#include "epd2_9/clay_ui/support/ClayLayoutPrimitives.h"

namespace epd2_9::clay_menu {

void renderMenu(const MenuRenderContext& ctx) {
  const TwoPaneLayoutMetrics metrics = defaultTwoPaneMenuMetrics();
  clay_layout::renderTwoPaneMenu(
      CLAY_ID("MenuRoot"),
      CLAY_ID("MenuViewport"),
      CLAY_ID("MenuPreviewDock"),
      ctx.palette,
      metrics,
      ctx.displayWidth,
      [&](const clay_layout::TwoPaneGeometry&) {
        CLAY(CLAY_ID("MenuScrollViewport"), {
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
        },
        .clip = {
            .horizontal = true,
            .vertical = true,
            .childOffset = {0.0f, -static_cast<float>(ctx.scrollOffset)},
        },
        }) {
          renderMenuList(ctx);
        }
      },
      [&](const clay_layout::TwoPaneGeometry&) { renderMenuPreview(ctx); });
}

}  // namespace epd2_9::clay_menu
