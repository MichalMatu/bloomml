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

// Tracks only physically confirmed content bounds. Planning is side-effect free:
// a failed refresh therefore cannot advance the previous-content state used by
// the next retry.
class DisplayDirtyRegionTracker final {
public:
  [[nodiscard]] DisplayRegion plan(const DisplayRegion& current_content,
                                   std::uint16_t padding_px,
                                   std::uint16_t display_width_px,
                                   std::uint16_t display_height_px) const noexcept {
    return planDisplayDirtyRegion(current_content, previous_content_, padding_px,
                                  display_width_px, display_height_px);
  }

  void confirm(const DisplayRegion& current_content, std::uint16_t display_width_px,
               std::uint16_t display_height_px) noexcept {
    previous_content_ =
        clampDisplayRegion(current_content, display_width_px, display_height_px);
  }

  void reset() noexcept {
    previous_content_ = {};
  }

  [[nodiscard]] const DisplayRegion& previousContent() const noexcept {
    return previous_content_;
  }

private:
  DisplayRegion previous_content_{};
};

} // namespace growbox::app::climate_io::display
