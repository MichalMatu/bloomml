#pragma once

#include "growbox_display_model/DisplayPageModel.h"

#include <cstddef>
#include <cstdint>

namespace growbox::clay_ui {

inline constexpr std::uint16_t kDisplayWidth = 296U;
inline constexpr std::uint16_t kDisplayHeight = 128U;
inline constexpr std::size_t kFrameStrideBytes = (kDisplayWidth + 7U) / 8U;
inline constexpr std::size_t kFrameBytes = kFrameStrideBytes * kDisplayHeight;

struct RenderSummary final {
  std::uint16_t width{0U};
  std::uint16_t height{0U};
  std::size_t black_pixels{0U};
  std::size_t render_commands{0U};
};

// Required caller-owned scratch arena for one Clay page render. The arena may
// live in PSRAM in firmware and is never retained after renderPageToMonochrome()
// returns.
[[nodiscard]] std::size_t pageRendererArenaBytes() noexcept;

// Render one semantic growbox page into a caller-owned 296x128 row-major 1-bpp
// framebuffer. A set bit is black. Clay state and render commands remain private
// to this C++20 component; only growbox-owned C++17-compatible types cross the
// public boundary.
[[nodiscard]] bool renderPageToMonochrome(const ::growbox::display_model::DisplayPageModel& page,
                                          std::uint8_t* framebuffer,
                                          std::size_t framebuffer_bytes, void* arena_memory,
                                          std::size_t arena_bytes,
                                          RenderSummary* summary = nullptr) noexcept;

} // namespace growbox::clay_ui
