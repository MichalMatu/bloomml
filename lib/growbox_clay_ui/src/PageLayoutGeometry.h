#pragma once

#include "growbox_clay_ui/PageRenderer.h"

#include <cstdint>

namespace growbox::clay_ui::internal::page_layout {

inline constexpr std::uint16_t kHorizontalPaddingPx = 6U;
inline constexpr std::uint16_t kVerticalPaddingPx = 5U;
inline constexpr std::uint16_t kInnerLeftPx = kHorizontalPaddingPx;
inline constexpr std::uint16_t kInnerWidthPx = kDisplayWidth - (2U * kHorizontalPaddingPx);
inline constexpr std::uint16_t kChildGapPx = 2U;
inline constexpr std::uint16_t kHeaderHeightPx = 14U;
inline constexpr std::uint16_t kHeaderTopPx = kVerticalPaddingPx;
inline constexpr std::uint16_t kSeparatorHeightPx = 1U;
inline constexpr std::uint16_t kRowsTopPx =
    kHeaderTopPx + kHeaderHeightPx + kChildGapPx + kSeparatorHeightPx + kChildGapPx;
inline constexpr std::uint16_t kRowsGapPx = 1U;
inline constexpr std::uint16_t kRowHeightPx = 9U;
inline constexpr std::uint16_t kRowStepPx = kRowHeightPx + kRowsGapPx;
inline constexpr std::uint16_t kTextBoxHeightPx = 8U;
inline constexpr std::uint16_t kLabelWidthPx = 72U;
inline constexpr std::uint16_t kColumnGapPx = 4U;
inline constexpr std::uint16_t kWarningWidthPx = 12U;

} // namespace growbox::clay_ui::internal::page_layout
