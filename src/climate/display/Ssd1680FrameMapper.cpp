#include "climate/display/Ssd1680FrameMapper.h"

#include <cstdint>

namespace growbox::app::climate_io::display {
namespace {

bool logicalPixelBlack(const DisplayMonochromeRaster::Buffer& buffer, std::uint16_t x_px,
                       std::uint16_t y_px) noexcept {
  const std::size_t index = static_cast<std::size_t>(y_px) * DisplayMonochromeRaster::kBytesPerRow +
                            static_cast<std::size_t>(x_px / 8U);
  const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> (x_px % 8U));
  return (buffer[index] & mask) == 0U;
}

} // namespace

bool Ssd1680FrameMapper::nativeByteAt(const DisplayMonochromeRaster::Buffer& logical_buffer,
                                      std::size_t native_byte_index, Ssd1680Rotation rotation,
                                      std::uint8_t& value) noexcept {
  if (native_byte_index >= kNativeBufferBytes) {
    return false;
  }

  const std::uint16_t native_y = static_cast<std::uint16_t>(native_byte_index / kNativeBytesPerRow);
  const std::uint16_t native_byte_x =
      static_cast<std::uint16_t>(native_byte_index % kNativeBytesPerRow);

  value = 0xFFU;
  for (std::uint16_t bit = 0U; bit < 8U; ++bit) {
    const std::uint16_t native_x = static_cast<std::uint16_t>(native_byte_x * 8U + bit);
    std::uint16_t logical_x = 0U;
    std::uint16_t logical_y = 0U;
    switch (rotation) {
    case Ssd1680Rotation::Clockwise90:
      logical_x = native_y;
      logical_y = static_cast<std::uint16_t>(DisplayMonochromeRaster::kHeightPx - 1U - native_x);
      break;
    case Ssd1680Rotation::CounterClockwise90:
      logical_x = static_cast<std::uint16_t>(DisplayMonochromeRaster::kWidthPx - 1U - native_y);
      logical_y = native_x;
      break;
    }

    if (logicalPixelBlack(logical_buffer, logical_x, logical_y)) {
      const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> bit);
      value = static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~mask));
    }
  }
  return true;
}

bool Ssd1680FrameMapper::nativeWindowForLogicalRegion(const DisplayRegion& logical_region,
                                                       Ssd1680Rotation rotation,
                                                       Ssd1680NativeWindow& output) noexcept {
  const DisplayRegion region =
      clampDisplayRegion(logical_region, DisplayMonochromeRaster::kWidthPx,
                         DisplayMonochromeRaster::kHeightPx);
  if (displayRegionEmpty(region)) {
    output = {};
    return false;
  }

  const std::uint16_t logical_right =
      static_cast<std::uint16_t>(region.x_px + region.width_px);
  const std::uint16_t logical_bottom =
      static_cast<std::uint16_t>(region.y_px + region.height_px);

  std::uint16_t native_x_start = 0U;
  std::uint16_t native_x_end_exclusive = 0U;
  std::uint16_t native_y_start = 0U;
  std::uint16_t native_y_end_exclusive = 0U;

  switch (rotation) {
  case Ssd1680Rotation::Clockwise90:
    native_x_start = static_cast<std::uint16_t>(kNativeWidthPx - logical_bottom);
    native_x_end_exclusive = static_cast<std::uint16_t>(kNativeWidthPx - region.y_px);
    native_y_start = region.x_px;
    native_y_end_exclusive = logical_right;
    break;
  case Ssd1680Rotation::CounterClockwise90:
    native_x_start = region.y_px;
    native_x_end_exclusive = logical_bottom;
    native_y_start = static_cast<std::uint16_t>(kNativeHeightPx - logical_right);
    native_y_end_exclusive = static_cast<std::uint16_t>(kNativeHeightPx - region.x_px);
    break;
  }

  if (native_x_start >= native_x_end_exclusive || native_y_start >= native_y_end_exclusive ||
      native_x_end_exclusive > kNativeWidthPx || native_y_end_exclusive > kNativeHeightPx) {
    output = {};
    return false;
  }

  output.x_start_byte = static_cast<std::uint8_t>(native_x_start / 8U);
  output.x_end_byte = static_cast<std::uint8_t>((native_x_end_exclusive - 1U) / 8U);
  output.y_start_px = native_y_start;
  output.y_end_px = static_cast<std::uint16_t>(native_y_end_exclusive - 1U);
  return output.transferBytes() > 0U;
}

} // namespace growbox::app::climate_io::display
