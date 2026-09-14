#pragma once

#include <cstdint>

/**
 * @file ClayTheme.h
 * @brief Shared visual tokens and layout metrics for Clay-based EPD screens.
 *
 * @page clay_ui_screen_layout_guide Clay UI screen layout guide
 *
 * @section clay_ui_add_screen Adding a new screen
 *
 * Keep a new Clay screen split into the same three layers as the existing
 * views:
 *
 * - State and routing: add the mode in ClayViewState.h, route it in
 *   ClayViewModeManager, and call the renderer from ClayViewLayoutOrchestrator.
 * - Presenter: prepare strings, selected indexes, scroll content height, and
 *   other view state outside the layout module.
 * - Layout module: render only the context it receives. It should not fetch
 *   services, mutate presenters, or calculate business state.
 *
 * Build the render context in ClayViewContextFactory so scroll metrics are
 * updated in one place. Prefer the existing ClayPalette and one of the shared
 * layout models: ListLayoutMetrics, TwoPaneLayoutMetrics,
 * DenseStatusLayoutMetrics, or GlanceLayoutMetrics. Add a new metric only when
 * the screen introduces a reusable layout pattern.
 *
 * @section clay_ui_position_math Position math
 *
 * Treat the display as the outer viewport and derive every inner rectangle
 * from the shared metrics. Avoid local kPreviewMinWidth, ratio, padding, or
 * gap constants inside screen modules.
 *
 * @code
 * frameWidth = displayWidth - 2 * outerPadding
 * contentWidth = frameWidth - 2 * framePadding
 * valueWidth = contentWidth - labelColumnWidth - gap
 *
 * frameHeight = displayHeight - 2 * outerPadding
 * scrollViewportHeight = frameHeight - 2 * framePadding
 * contentHeight = rowCount * rowHeight + max(0, rowCount - 1) * rowGap
 * @endcode
 *
 * Use CLAY_SIZING_GROW for the trailing column so Clay absorbs rounding and
 * long values get the remaining space. When a screen scrolls, pass
 * scrollViewportHeight and contentHeight to the controller before rendering.
 * Size rows from the real GFX font selected by textFontId; the EPD backend
 * currently treats Clay fontSize as advisory and does not scale the font. Do
 * not use fontSize for layout math until both EPD and SDL renderers implement
 * scaling. Pick a rowHeight/rowGap pair that leaves a whole number of rows
 * visible in the clipped viewport whenever possible, especially on status
 * screens.
 *
 * @section clay_ui_text_policy Text policy
 *
 * Every text element must choose one explicit policy:
 *
 * - Fit: short labels that are guaranteed by the presenter to fit.
 * - Clip: one-line values inside a clipped wrapper when truncation is intended.
 * - Wrap: explicit newlines or Clay wrapping with a documented lineHeight.
 * - Shorten: presenter returns a shorter label/value for the available slot.
 *
 * Do not rely on accidental clipping by a parent container. For common framed
 * panels, fixed rows, two-pane menus, and preview text, use
 * ClayLayoutPrimitives.h helpers so the policy is visible at the call site.
 *
 * @section clay_ui_visual_consistency Visual consistency
 *
 * Dense EPD screens should follow the Dashboard Quick Overview rhythm: no
 * outer margin, one full-width frame/card, 4 px inner padding, 2 px gaps, and
 * a 1 px border using ClayPalette::border. Reuse ClayPalette colors instead of
 * local hard-coded greys so status, menu preview, and settings screens remain
 * visually aligned.
 *
 * Use left-aligned labels with a fixed label column and right-aligned values
 * that grow into the remaining width. Keep list and status rows fixed-height
 * to avoid layout jumps while scrolling. Avoid nested cards; use one framed
 * surface per screen, and use selected-row palette colors for focus states.
 */

namespace epd2_9 {

struct ClayPalette {
  uint32_t background;
  uint32_t card;
  uint32_t cardSelected;
  uint32_t cardText;
  uint32_t cardTextSelected;
  uint32_t border;
};

struct ListLayoutMetrics {
  int16_t viewportPadding;
  int16_t rowHeight;
  int16_t rowGap;
  int16_t itemPadding;
  uint16_t textFontId = 0;
  uint16_t rowLineHeight = 18;
  uint16_t labelColumnWidth = 132;
  uint16_t valueColumnWidth = 90;
};

using ClayLayoutMetrics = ListLayoutMetrics;

struct TwoPaneLayoutMetrics {
  uint16_t outerPadding;
  uint16_t columnGap;
  uint16_t listWidth;
  uint16_t listRowHeight;
  uint16_t listRowGap;
  uint16_t listItemPadding;
  uint16_t previewPadding;
  uint16_t previewLineHeight;
  uint16_t previewLineGap;
  uint16_t borderWidth;
  uint16_t textFontId;
};

struct GlanceLayoutMetrics {
  uint16_t outerPadding;
  uint16_t framePadding;
  uint16_t rowGap;
  uint16_t columnGap;
};

struct DenseStatusLayoutMetrics {
  uint16_t outerPadding;
  uint16_t cardPadding;
  uint16_t sectionGap;
  uint16_t cardGap;
  uint16_t labelColumnWidth;
  uint16_t cardHeight;
  uint16_t rowHeight = 18;
  uint16_t textFontId = 0;
  uint16_t rowLineHeight = 18;
};

using StatusOverviewLayoutMetrics = DenseStatusLayoutMetrics;

ClayPalette defaultClayPalette();
bool isLargeDisplay();
ListLayoutMetrics defaultListMetrics();
ClayLayoutMetrics defaultLayoutMetrics();
TwoPaneLayoutMetrics defaultTwoPaneMenuMetrics();
GlanceLayoutMetrics defaultGlanceMetrics();
DenseStatusLayoutMetrics defaultDenseStatusMetrics();
StatusOverviewLayoutMetrics defaultStatusOverviewMetrics();

}  // namespace epd2_9
