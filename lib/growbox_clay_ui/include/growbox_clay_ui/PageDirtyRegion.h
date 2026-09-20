#pragma once

#include "growbox_display_model/DisplayPageModel.h"

#include <cstdint>

namespace growbox::clay_ui {

struct PageDirtyRegion final {
  std::uint16_t x_px{0U};
  std::uint16_t y_px{0U};
  std::uint16_t width_px{0U};
  std::uint16_t height_px{0U};
};

[[nodiscard]] constexpr bool pageDirtyRegionEmpty(const PageDirtyRegion& region) noexcept {
  return region.width_px == 0U || region.height_px == 0U;
}

// Computes the smallest layout-owned region that can visually differ between
// two semantic page models. Passing nullptr for previous means there is no
// physically confirmed baseline and therefore requests a full-screen region.
// The result is intentionally unpadded; hardware-specific safety padding and
// byte alignment belong to the SSD1680 backend.
[[nodiscard]] bool planPageDirtyRegion(
    const ::growbox::display_model::DisplayPageModel* previous,
    const ::growbox::display_model::DisplayPageModel& current,
    PageDirtyRegion& output) noexcept;

} // namespace growbox::clay_ui
