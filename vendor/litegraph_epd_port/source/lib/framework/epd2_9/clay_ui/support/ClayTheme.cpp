#include "epd2_9/clay_ui/support/ClayTheme.h"

#include "platform/hardware/BoardPinMap.h"

namespace epd2_9 {
namespace {

constexpr std::uint16_t displayLongEdge() {
  return platform::hardware::kDisplayWidth > platform::hardware::kDisplayHeight
             ? platform::hardware::kDisplayWidth
             : platform::hardware::kDisplayHeight;
}

constexpr bool largeDisplay() {
  return displayLongEdge() >= 700;
}

}  // namespace

ClayPalette defaultClayPalette() {
  return ClayPalette{
      .background = 0xFFFFFFFF,
      .card = 0xF5F5F5FF,
      .cardSelected = 0x111111FF,
      .cardText = 0x222222FF,
      .cardTextSelected = 0xFFFFFFFF,
      .border = 0x1A1A1AFF,
  };
}

bool isLargeDisplay() {
  return largeDisplay();
}

ListLayoutMetrics defaultListMetrics() {
  if (largeDisplay()) {
    return ListLayoutMetrics{
        .viewportPadding = 12,
        .rowHeight = 58,
        .rowGap = 6,
        .itemPadding = 10,
        .textFontId = 2,
        .rowLineHeight = 42,
        .labelColumnWidth = 330,
        .valueColumnWidth = 260,
    };
  }

  return ListLayoutMetrics{
      .viewportPadding = 4,
      .rowHeight = 28,
      .rowGap = 2,
      .itemPadding = 4,
      .textFontId = 0,
      .rowLineHeight = 18,
      .labelColumnWidth = 132,
      .valueColumnWidth = 90,
  };
}

ClayLayoutMetrics defaultLayoutMetrics() {
  return defaultListMetrics();
}

TwoPaneLayoutMetrics defaultTwoPaneMenuMetrics() {
  if (largeDisplay()) {
    return TwoPaneLayoutMetrics{
        .outerPadding = 12,
        .columnGap = 10,
        .listWidth = 340,
        .listRowHeight = 58,
        .listRowGap = 6,
        .listItemPadding = 10,
        .previewPadding = 12,
        .previewLineHeight = 42,
        .previewLineGap = 6,
        .borderWidth = 2,
        .textFontId = 2,
    };
  }

  return TwoPaneLayoutMetrics{
      .outerPadding = 4,
      .columnGap = 4,
      .listWidth = 124,
      .listRowHeight = 28,
      .listRowGap = 2,
      .listItemPadding = 4,
      .previewPadding = 4,
      .previewLineHeight = 18,
      .previewLineGap = 2,
      .borderWidth = 1,
      .textFontId = 0,
  };
}

GlanceLayoutMetrics defaultGlanceMetrics() {
  if (largeDisplay()) {
    return GlanceLayoutMetrics{
        .outerPadding = 0,
        .framePadding = 10,
        .rowGap = 6,
        .columnGap = 10,
    };
  }

  return GlanceLayoutMetrics{
      .outerPadding = 0,
      .framePadding = 4,
      .rowGap = 2,
      .columnGap = 4,
  };
}

DenseStatusLayoutMetrics defaultDenseStatusMetrics() {
  if (largeDisplay()) {
    return DenseStatusLayoutMetrics{
        .outerPadding = 0,
        .cardPadding = 10,
        .sectionGap = 6,
        .cardGap = 10,
        .labelColumnWidth = 260,
        .cardHeight = 260,
        .rowHeight = 46,
        .textFontId = 2,
        .rowLineHeight = 42,
    };
  }

  return DenseStatusLayoutMetrics{
      .outerPadding = 0,
      .cardPadding = 4,
      .sectionGap = 2,
      .cardGap = 2,
      .labelColumnWidth = 86,
      .cardHeight = 128,
      .rowHeight = 18,
      .textFontId = 0,
      .rowLineHeight = 18,
  };
}

StatusOverviewLayoutMetrics defaultStatusOverviewMetrics() {
  return defaultDenseStatusMetrics();
}

}  // namespace epd2_9
