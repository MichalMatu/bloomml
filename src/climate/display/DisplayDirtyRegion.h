#pragma once

#include <cstdint>

namespace growbox::app::climate_io::display {

struct DisplayRegion final {
  std::uint16_t x_px{0U};
  std::uint16_t y_px{0U};
  std::uint16_t width_px{0U};
  std::uint16_t height_px{0U};
};

[[nodiscard]] constexpr bool displayRegionEmpty(const DisplayRegion& region) noexcept {
  return region.width_px == 0U || region.height_px == 0U;
}

[[nodiscard]] DisplayRegion clampDisplayRegion(const DisplayRegion& region,
                                               std::uint16_t display_width_px,
                                               std::uint16_t display_height_px) noexcept;

[[nodiscard]] DisplayRegion expandDisplayRegion(const DisplayRegion& region,
                                                std::uint16_t padding_px,
                                                std::uint16_t display_width_px,
                                                std::uint16_t display_height_px) noexcept;

[[nodiscard]] DisplayRegion unionDisplayRegions(const DisplayRegion& left,
                                                const DisplayRegion& right,
                                                std::uint16_t display_width_px,
                                                std::uint16_t display_height_px) noexcept;

// Pure geometry helper used by host tests and callers that already know both
// old and new visual bounds. Layout-aware page diffing remains in growbox_clay_ui.
[[nodiscard]] DisplayRegion planDisplayDirtyRegion(const DisplayRegion& current_content,
                                                   const DisplayRegion& previous_content,
                                                   std::uint16_t padding_px,
                                                   std::uint16_t display_width_px,
                                                   std::uint16_t display_height_px) noexcept;

} // namespace growbox::app::climate_io::display
