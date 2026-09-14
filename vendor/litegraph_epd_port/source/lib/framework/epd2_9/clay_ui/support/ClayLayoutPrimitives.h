#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <clay/clay.h>

#include "epd2_9/clay_ui/render/ClayRenderer.h"
#include "epd2_9/clay_ui/support/ClayTextUtils.h"
#include "epd2_9/clay_ui/support/ClayTheme.h"

namespace epd2_9::clay_layout {

struct TwoPaneGeometry {
  float listWidth;
  float previewWidth;
};

inline float toFloat(uint16_t value) {
  return static_cast<float>(value);
}

inline TwoPaneGeometry twoPaneGeometry(const TwoPaneLayoutMetrics& metrics,
                                       int16_t displayWidth) {
  const int32_t available = static_cast<int32_t>(displayWidth) -
                            2 * static_cast<int32_t>(metrics.outerPadding) -
                            static_cast<int32_t>(metrics.columnGap);
  if (available <= 0) {
    return TwoPaneGeometry{0.0f, 0.0f};
  }

  const uint16_t listWidth =
      std::min<uint16_t>(metrics.listWidth, static_cast<uint16_t>(available));
  const int32_t previewWidth = available - static_cast<int32_t>(listWidth);
  return TwoPaneGeometry{
      .listWidth = static_cast<float>(listWidth),
      .previewWidth = static_cast<float>(std::max<int32_t>(0, previewWidth)),
  };
}

template <typename Body>
void renderFramedPanel(Clay_ElementId id,
                       Clay_Sizing sizing,
                       Clay_Color background,
                       Clay_Color border,
                       uint16_t borderWidth,
                       uint16_t padding,
                       uint16_t childGap,
                       Body body) {
  CLAY(id, {
      .layout = {
          .sizing = sizing,
          .padding = {.left = padding,
                      .right = padding,
                      .top = padding,
                      .bottom = padding},
          .childGap = childGap,
          .layoutDirection = CLAY_TOP_TO_BOTTOM,
      },
      .backgroundColor = background,
      .border = {.color = border,
                 .width = {borderWidth, borderWidth, borderWidth, borderWidth, 0}},
  }) {
    body();
  }
}

template <typename Body>
void renderFixedRow(Clay_ElementId id,
                    float height,
                    uint16_t padding,
                    uint16_t childGap,
                    Clay_Color background,
                    Clay_Color border,
                    uint16_t borderWidth,
                    Body body) {
  CLAY(id, {
      .layout = {
          .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(height)},
          .padding = {.left = padding,
                      .right = padding,
                      .top = padding,
                      .bottom = padding},
          .childGap = childGap,
          .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
          .layoutDirection = CLAY_LEFT_TO_RIGHT,
      },
      .backgroundColor = background,
      .clip = {.horizontal = true},
      .border = {.color = border,
                 .width = {borderWidth, borderWidth, borderWidth, borderWidth, 0}},
  }) {
    body();
  }
}

template <typename LeftBody, typename RightBody>
void renderTwoPaneMenu(Clay_ElementId rootId,
                       Clay_ElementId listId,
                       Clay_ElementId previewId,
                       const ClayPalette& palette,
                       const TwoPaneLayoutMetrics& metrics,
                       int16_t displayWidth,
                       LeftBody leftBody,
                       RightBody rightBody) {
  const TwoPaneGeometry geometry = twoPaneGeometry(metrics, displayWidth);
  const Clay_Color background = toClayColor(palette.background);

  CLAY(rootId, {
      .layout = {
          .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
          .padding = {.left = metrics.outerPadding,
                      .right = metrics.outerPadding,
                      .top = metrics.outerPadding,
                      .bottom = metrics.outerPadding},
          .childGap = metrics.columnGap,
          .childAlignment = {.y = CLAY_ALIGN_Y_TOP},
          .layoutDirection = CLAY_LEFT_TO_RIGHT,
      },
      .backgroundColor = background,
  }) {
    CLAY(listId, {
        .layout = {
            .sizing = {CLAY_SIZING_FIXED(geometry.listWidth),
                       CLAY_SIZING_GROW(0)},
            .childAlignment = {.y = CLAY_ALIGN_Y_TOP},
        },
    }) {
      leftBody(geometry);
    }

    CLAY(previewId, {
        .layout = {
            .sizing = {CLAY_SIZING_FIXED(geometry.previewWidth),
                       CLAY_SIZING_GROW(0)},
            .childAlignment = {.y = CLAY_ALIGN_Y_TOP},
        },
    }) {
      rightBody(geometry);
    }
  }
}

inline void renderClippedText(Clay_ElementId id,
                              Clay_String text,
                              Clay_Color color,
                              uint16_t fontId,
                              uint16_t lineHeight,
                              Clay_TextAlignment alignment = CLAY_TEXT_ALIGN_LEFT) {
  const float clipHeight = static_cast<float>(lineHeight + 6U);
  constexpr uint16_t kRightAlignedGlyphGuard = 12;
  const uint16_t rightPadding =
      alignment == CLAY_TEXT_ALIGN_RIGHT ? kRightAlignedGlyphGuard : 0;
  CLAY(id, {
      .layout = {
          .sizing = {CLAY_SIZING_GROW(1), CLAY_SIZING_FIXED(clipHeight)},
          .padding = {.left = 0, .right = rightPadding, .top = 0, .bottom = 0},
          .childAlignment = {.x = alignment == CLAY_TEXT_ALIGN_RIGHT
                                      ? CLAY_ALIGN_X_RIGHT
                                      : (alignment == CLAY_TEXT_ALIGN_CENTER
                                             ? CLAY_ALIGN_X_CENTER
                                             : CLAY_ALIGN_X_LEFT),
                              .y = CLAY_ALIGN_Y_CENTER},
      },
      .clip = {.horizontal = true},
  }) {
    CLAY_TEXT(text,
              CLAY_TEXT_CONFIG({.textColor = color,
                                .fontId = fontId,
                                .lineHeight = lineHeight,
                                .wrapMode = CLAY_TEXT_WRAP_NONE,
                                .textAlignment = alignment}));
  }
}

inline void renderFitText(Clay_String text,
                          Clay_Color color,
                          uint16_t fontId,
                          uint16_t lineHeight,
                          Clay_TextAlignment alignment = CLAY_TEXT_ALIGN_LEFT) {
  CLAY_TEXT(text,
            CLAY_TEXT_CONFIG({.textColor = color,
                              .fontId = fontId,
                              .lineHeight = lineHeight,
                              .wrapMode = CLAY_TEXT_WRAP_NONE,
                              .textAlignment = alignment}));
}

inline void renderPreviewLines(const char* const* lines,
                               std::size_t lineCount,
                               Clay_Color textColor,
                               const TwoPaneLayoutMetrics& metrics) {
  uint32_t visibleLine = 0;
  for (std::size_t i = 0; i < lineCount; ++i) {
    if (!clay_text::hasText(lines[i])) {
      continue;
    }
    const uint32_t lineId = visibleLine++;
    renderClippedText(CLAY_IDI_LOCAL("PreviewLine", lineId),
                      clay_text::makeDynamic(lines[i]),
                      textColor,
                      metrics.textFontId,
                      metrics.previewLineHeight);
  }
}

}  // namespace epd2_9::clay_layout
