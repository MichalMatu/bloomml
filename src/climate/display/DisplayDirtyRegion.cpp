#include "climate/display/DisplayDirtyRegion.h"

#include <algorithm>
#include <cstdint>

namespace growbox::app::climate_io::display {
namespace {

struct RegionEdges final {
  std::uint32_t left{0U};
  std::uint32_t top{0U};
  std::uint32_t right{0U};
  std::uint32_t bottom{0U};
};

RegionEdges clampedEdges(const DisplayRegion& region, std::uint16_t display_width_px,
                         std::uint16_t display_height_px) noexcept {
  const std::uint32_t display_width = display_width_px;
  const std::uint32_t display_height = display_height_px;
  const std::uint32_t left = std::min<std::uint32_t>(region.x_px, display_width);
  const std::uint32_t top = std::min<std::uint32_t>(region.y_px, display_height);
  const std::uint32_t right = std::min<std::uint32_t>(
      static_cast<std::uint32_t>(region.x_px) + region.width_px, display_width);
  const std::uint32_t bottom = std::min<std::uint32_t>(
      static_cast<std::uint32_t>(region.y_px) + region.height_px, display_height);
  return {left, top, std::max(left, right), std::max(top, bottom)};
}

DisplayRegion fromEdges(const RegionEdges& edges) noexcept {
  if (edges.right <= edges.left || edges.bottom <= edges.top) {
    return {};
  }
  return {
      static_cast<std::uint16_t>(edges.left),
      static_cast<std::uint16_t>(edges.top),
      static_cast<std::uint16_t>(edges.right - edges.left),
      static_cast<std::uint16_t>(edges.bottom - edges.top),
  };
}

} // namespace

DisplayRegion clampDisplayRegion(const DisplayRegion& region, std::uint16_t display_width_px,
                                 std::uint16_t display_height_px) noexcept {
  if (display_width_px == 0U || display_height_px == 0U || displayRegionEmpty(region)) {
    return {};
  }
  return fromEdges(clampedEdges(region, display_width_px, display_height_px));
}

DisplayRegion expandDisplayRegion(const DisplayRegion& region, std::uint16_t padding_px,
                                  std::uint16_t display_width_px,
                                  std::uint16_t display_height_px) noexcept {
  const DisplayRegion clamped = clampDisplayRegion(region, display_width_px, display_height_px);
  if (displayRegionEmpty(clamped)) {
    return {};
  }

  const std::uint32_t left =
      clamped.x_px > padding_px ? static_cast<std::uint32_t>(clamped.x_px - padding_px) : 0U;
  const std::uint32_t top =
      clamped.y_px > padding_px ? static_cast<std::uint32_t>(clamped.y_px - padding_px) : 0U;
  const std::uint32_t right = std::min<std::uint32_t>(
      static_cast<std::uint32_t>(display_width_px),
      static_cast<std::uint32_t>(clamped.x_px) + clamped.width_px + padding_px);
  const std::uint32_t bottom = std::min<std::uint32_t>(
      static_cast<std::uint32_t>(display_height_px),
      static_cast<std::uint32_t>(clamped.y_px) + clamped.height_px + padding_px);
  return fromEdges({left, top, right, bottom});
}

DisplayRegion unionDisplayRegions(const DisplayRegion& left, const DisplayRegion& right,
                                  std::uint16_t display_width_px,
                                  std::uint16_t display_height_px) noexcept {
  const DisplayRegion clamped_left = clampDisplayRegion(left, display_width_px, display_height_px);
  const DisplayRegion clamped_right = clampDisplayRegion(right, display_width_px, display_height_px);
  if (displayRegionEmpty(clamped_left)) {
    return clamped_right;
  }
  if (displayRegionEmpty(clamped_right)) {
    return clamped_left;
  }

  const RegionEdges left_edges = clampedEdges(clamped_left, display_width_px, display_height_px);
  const RegionEdges right_edges = clampedEdges(clamped_right, display_width_px, display_height_px);
  return fromEdges({
      std::min(left_edges.left, right_edges.left),
      std::min(left_edges.top, right_edges.top),
      std::max(left_edges.right, right_edges.right),
      std::max(left_edges.bottom, right_edges.bottom),
  });
}

DisplayRegion planDisplayDirtyRegion(const DisplayRegion& current_content,
                                     const DisplayRegion& previous_content,
                                     std::uint16_t padding_px, std::uint16_t display_width_px,
                                     std::uint16_t display_height_px) noexcept {
  const DisplayRegion current =
      expandDisplayRegion(current_content, padding_px, display_width_px, display_height_px);
  const DisplayRegion previous =
      expandDisplayRegion(previous_content, padding_px, display_width_px, display_height_px);
  return unionDisplayRegions(current, previous, display_width_px, display_height_px);
}

} // namespace growbox::app::climate_io::display
