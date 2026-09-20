#include "growbox_clay_ui/PageDirtyRegion.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace growbox::clay_ui {
namespace {

constexpr std::uint16_t kDisplayWidth = 296U;
constexpr std::uint16_t kDisplayHeight = 128U;
constexpr std::uint16_t kInnerLeftPx = 6U;
constexpr std::uint16_t kInnerWidthPx = 284U;
constexpr std::uint16_t kHeaderTopPx = 5U;
constexpr std::uint16_t kHeaderHeightPx = 14U;
constexpr std::uint16_t kRowsTopPx = 24U;
constexpr std::uint16_t kRowHeightPx = 9U;
constexpr std::uint16_t kRowStepPx = 10U;

void includeRegion(PageDirtyRegion& bounds, bool& has_bounds,
                   const PageDirtyRegion& region) noexcept {
  if (pageDirtyRegionEmpty(region)) {
    return;
  }
  if (!has_bounds) {
    bounds = region;
    has_bounds = true;
    return;
  }

  const std::uint32_t left = std::min<std::uint32_t>(bounds.x_px, region.x_px);
  const std::uint32_t top = std::min<std::uint32_t>(bounds.y_px, region.y_px);
  const std::uint32_t right = std::max<std::uint32_t>(
      static_cast<std::uint32_t>(bounds.x_px) + bounds.width_px,
      static_cast<std::uint32_t>(region.x_px) + region.width_px);
  const std::uint32_t bottom = std::max<std::uint32_t>(
      static_cast<std::uint32_t>(bounds.y_px) + bounds.height_px,
      static_cast<std::uint32_t>(region.y_px) + region.height_px);
  bounds = {static_cast<std::uint16_t>(left), static_cast<std::uint16_t>(top),
            static_cast<std::uint16_t>(right - left),
            static_cast<std::uint16_t>(bottom - top)};
}

template <std::size_t N>
bool stringsDiffer(const std::array<char, N>& left, const std::array<char, N>& right) noexcept {
  return std::strcmp(left.data(), right.data()) != 0;
}

} // namespace

bool planPageDirtyRegion(const ::growbox::display_model::DisplayPageModel* previous,
                         const ::growbox::display_model::DisplayPageModel& current,
                         PageDirtyRegion& output) noexcept {
  output = {};
  if (previous == nullptr) {
    output = {0U, 0U, kDisplayWidth, kDisplayHeight};
    return true;
  }

  bool has_bounds = false;
  if (stringsDiffer(previous->title, current.title) || previous->warning != current.warning) {
    includeRegion(output, has_bounds,
                  {kInnerLeftPx, kHeaderTopPx, kInnerWidthPx, kHeaderHeightPx});
  }

  const std::size_t previous_count = std::min(previous->line_count, previous->lines.size());
  const std::size_t current_count = std::min(current.line_count, current.lines.size());
  const std::size_t line_count = std::max(previous_count, current_count);
  for (std::size_t index = 0U; index < line_count; ++index) {
    const bool previous_present = index < previous_count;
    const bool current_present = index < current_count;
    bool changed = previous_present != current_present;
    if (previous_present && current_present) {
      changed = stringsDiffer(previous->lines[index].label, current.lines[index].label) ||
                stringsDiffer(previous->lines[index].value, current.lines[index].value);
    }
    if (!changed) {
      continue;
    }

    const std::uint16_t row_y =
        static_cast<std::uint16_t>(kRowsTopPx + index * kRowStepPx);
    includeRegion(output, has_bounds, {kInnerLeftPx, row_y, kInnerWidthPx, kRowHeightPx});
  }

  return has_bounds;
}

} // namespace growbox::clay_ui
