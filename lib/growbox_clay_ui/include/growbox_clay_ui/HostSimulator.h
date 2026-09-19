#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::clay_ui {

inline constexpr std::uint16_t kDisplayWidth = 296;
inline constexpr std::uint16_t kDisplayHeight = 128;
inline constexpr std::size_t kFrameStrideBytes = (kDisplayWidth + 7U) / 8U;
inline constexpr std::size_t kFrameBytes = kFrameStrideBytes * kDisplayHeight;

struct MonochromeFrame {
  std::array<std::uint8_t, kFrameBytes> bytes{};

  [[nodiscard]] bool pixel(std::uint16_t x, std::uint16_t y) const noexcept;
};

struct RenderSummary {
  std::uint16_t width = 0;
  std::uint16_t height = 0;
  std::size_t black_pixels = 0;
  std::size_t render_commands = 0;
};

[[nodiscard]] bool renderHostSmoke(MonochromeFrame& frame,
                                   RenderSummary* summary = nullptr) noexcept;
[[nodiscard]] bool writePbm(const MonochromeFrame& frame, const char* path) noexcept;

} // namespace growbox::clay_ui
