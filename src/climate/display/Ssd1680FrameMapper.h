#pragma once

#include "climate/display/DisplayMonochromeRaster.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {

enum class Ssd1680Rotation : std::uint8_t {
  Clockwise90,
  CounterClockwise90,
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
};

static_assert(Ssd1680FrameMapper::kNativeBufferBytes == DisplayMonochromeRaster::kBufferBytes);

} // namespace growbox::app::climate_io::display
