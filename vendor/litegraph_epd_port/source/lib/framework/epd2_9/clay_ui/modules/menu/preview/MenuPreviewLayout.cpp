#include "epd2_9/clay_ui/modules/menu/preview/MenuPreviewLayout.h"

#include <clay/clay.h>

#include "epd2_9/clay_ui/render/ClayRenderer.h"
#include "epd2_9/clay_ui/support/ClayLayoutPrimitives.h"
#include "epd2_9/clay_ui/support/ClayTextUtils.h"

namespace epd2_9::clay_menu {
namespace {

const MenuItem* selectedMenuItem(const MenuRenderContext& ctx) {
  if (ctx.selectedIndex < 0 || ctx.selectedIndex >= static_cast<int>(ctx.items.size())) {
    return nullptr;
  }
  return &ctx.items[static_cast<size_t>(ctx.selectedIndex)];
}

}  // namespace

void renderMenuPreview(const MenuRenderContext& ctx) {
  const MenuItem* selected = selectedMenuItem(ctx);
  if (!selected) {
    return;
  }

  const Clay_Color cardColor = toClayColor(ctx.palette.card);
  const Clay_Color borderColor = toClayColor(ctx.palette.border);
  const Clay_Color textColor = toClayColor(ctx.palette.cardText);
  const TwoPaneLayoutMetrics metrics = defaultTwoPaneMenuMetrics();
  const char* const lines[] = {
      selected->previewLine1,
      selected->previewLine2,
      selected->previewLine3,
      selected->previewLine4,
  };

  clay_layout::renderFramedPanel(
      CLAY_ID("MenuPreview"),
      Clay_Sizing{CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0.0f, 0.0f)},
      cardColor,
      borderColor,
      metrics.borderWidth,
      metrics.previewPadding,
      metrics.previewLineGap,
      [&]() { clay_layout::renderPreviewLines(lines, 4, textColor, metrics); });
}

}  // namespace epd2_9::clay_menu
