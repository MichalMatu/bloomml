#pragma once

#include "climate/display/DisplayDirtyRegion.h"
#include "climate/display/DisplayMonochromeRaster.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {

enum class Ssd1680Rotation : std::uint8_t {
  Clockwise90,
  CounterClockwise90,
};

struct Ssd1680NativeWindow final {
  std::uint8_t x_start_byte{0U};
  std::uint8_t x_end_byte{0U};
  std::uint16_t y_start_px{0U};
  std::uint16_t y_end_px{0U};

  [[nodiscard]] constexpr std::size_t transferBytes() const noexcept {
    return x_end_byte >= x_start_byte && y_end_px >= y_start_px
               ? static_cast<std::size_t>(x_end_byte - x_start_byte + 1U) *
                     static_cast<std::size_t>(y_end_px - y_start_px + 1U)
               : 0U;
  }
};

// Converts the landscape 296x128 software canvas into the SSD1680's native
// 128x296 RAM byte order without allocating a second framebuffer.
class Ssd1680FrameMapper final {
public:
  static constexpr std::uint16_t kNativeWidthPx = 128U;
  static constexpr std::uint16_t kNativeHeightPx = 296U;
  static constexpr std::size_t kNativeBytesPerRow = kNativeWidthPx / 8U;
  static constexpr std::size_t kNativeBufferBytes = kNativeBytesPerRow * kNativeHeightPx;

  static bool nativeByteAt(const DisplayMonochromeRaster::Buffer& logical_buffer,
                           std::size_t native_byte_index, Ssd1680Rotation rotation,
                           std::uint8_t& value) noexcept;

  // Map a logical landscape region to the smallest byte-aligned SSD1680 native
  // RAM window that covers it. X coordinates are controller byte addresses;
  // Y coordinates are inclusive native pixel rows.
  static bool nativeWindowForLogicalRegion(const DisplayRegion& logical_region,
                                           Ssd1680Rotation rotation,
                                           Ssd1680NativeWindow& output) noexcept;
};

static_assert(Ssd1680FrameMapper::kNativeBufferBytes == DisplayMonochromeRaster::kBufferBytes);

} // namespace growbox::app::climate_io::display
