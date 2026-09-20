#pragma once

#include "growbox_clay_ui/PageRenderer.h"

#include <array>
#include <cstdint>

namespace growbox::clay_ui {

struct MonochromeFrame {
  std::array<std::uint8_t, kFrameBytes> bytes{};

  [[nodiscard]] bool pixel(std::uint16_t x, std::uint16_t y) const noexcept;
};

[[nodiscard]] bool renderHostSmoke(MonochromeFrame& frame,
                                   RenderSummary* summary = nullptr) noexcept;
[[nodiscard]] bool renderHostPage(const ::growbox::display_model::DisplayPageModel& page,
                                  MonochromeFrame& frame,
                                  RenderSummary* summary = nullptr) noexcept;
[[nodiscard]] bool writePbm(const MonochromeFrame& frame, const char* path) noexcept;

} // namespace growbox::clay_ui
