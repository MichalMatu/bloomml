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

// Plan one physical partial-refresh area from the currently visible content and
// the content area from the last successful physical refresh. Including both is
// required so moved or removed pixels are explicitly rewritten white.
[[nodiscard]] DisplayRegion planDisplayDirtyRegion(const DisplayRegion& current_content,
                                                   const DisplayRegion& previous_content,
                                                   std::uint16_t padding_px,
                                                   std::uint16_t display_width_px,
                                                   std::uint16_t display_height_px) noexcept;

} // namespace growbox::app::climate_io::display
