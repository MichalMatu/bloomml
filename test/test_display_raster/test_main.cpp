#include "climate/display/DisplayDirtyRegion.h"
#include "climate/display/DisplayMonochromeRaster.h"
#include "climate/display/Ssd1680FrameMapper.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace display = growbox::app::climate_io::display;

namespace {

display::DisplayTextElement textElement(std::uint16_t x, std::uint16_t y, std::uint16_t max_width,
                                        std::uint16_t font_size, const char* text) {
  return {x, y, max_width, {0U, font_size, false}, text};
}

void setLogicalBlack(display::DisplayMonochromeRaster::Buffer& buffer, std::uint16_t x,
                     std::uint16_t y) {
  const std::size_t index =
      static_cast<std::size_t>(y) * display::DisplayMonochromeRaster::kBytesPerRow +
      static_cast<std::size_t>(x / 8U);
  const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
  buffer[index] = static_cast<std::uint8_t>(buffer[index] & static_cast<std::uint8_t>(~mask));
}

void assertRegion(const display::DisplayRegion& region, std::uint16_t x, std::uint16_t y,
                  std::uint16_t width, std::uint16_t height) {
  assert(region.x_px == x);
  assert(region.y_px == y);
  assert(region.width_px == width);
  assert(region.height_px == height);
}

void testDirtyRegionGeometry() {
  constexpr std::uint16_t width = display::DisplayMonochromeRaster::kWidthPx;
  constexpr std::uint16_t height = display::DisplayMonochromeRaster::kHeightPx;

  assertRegion(display::clampDisplayRegion({290U, 120U, 20U, 20U}, width, height), 290U, 120U,
               6U, 8U);
  assertRegion(display::expandDisplayRegion({4U, 3U, 10U, 5U}, 16U, width, height), 0U, 0U,
               30U, 24U);
  assertRegion(display::expandDisplayRegion({290U, 120U, 6U, 8U}, 16U, width, height), 274U,
               104U, 22U, 24U);

  const display::DisplayRegion previous{20U, 40U, 20U, 10U};
  const display::DisplayRegion current{100U, 40U, 20U, 10U};
  assertRegion(display::planDisplayDirtyRegion(current, previous, 16U, width, height), 4U, 24U,
               132U, 42U);
  assertRegion(display::planDisplayDirtyRegion({}, previous, 16U, width, height), 4U, 24U, 52U,
               42U);
  assert(display::displayRegionEmpty(display::planDisplayDirtyRegion({}, {}, 16U, width, height)));
}

void testNativeWindowMapping() {
  display::Ssd1680NativeWindow window{};
  const display::DisplayRegion full{0U, 0U, display::DisplayMonochromeRaster::kWidthPx,
                                    display::DisplayMonochromeRaster::kHeightPx};
  assert(display::Ssd1680FrameMapper::nativeWindowForLogicalRegion(
      full, display::Ssd1680Rotation::Clockwise90, window));
  assert(window.x_start_byte == 0U && window.x_end_byte == 15U);
  assert(window.y_start_px == 0U && window.y_end_px == 295U);
  assert(window.transferBytes() == display::Ssd1680FrameMapper::kNativeBufferBytes);

  assert(display::Ssd1680FrameMapper::nativeWindowForLogicalRegion(
      full, display::Ssd1680Rotation::CounterClockwise90, window));
  assert(window.x_start_byte == 0U && window.x_end_byte == 15U);
  assert(window.y_start_px == 0U && window.y_end_px == 295U);

  const display::DisplayRegion logical{10U, 20U, 30U, 40U};
  assert(display::Ssd1680FrameMapper::nativeWindowForLogicalRegion(
      logical, display::Ssd1680Rotation::Clockwise90, window));
  assert(window.x_start_byte == 8U && window.x_end_byte == 13U);
  assert(window.y_start_px == 10U && window.y_end_px == 39U);
  assert(window.transferBytes() == 180U);

  assert(display::Ssd1680FrameMapper::nativeWindowForLogicalRegion(
      logical, display::Ssd1680Rotation::CounterClockwise90, window));
  assert(window.x_start_byte == 2U && window.x_end_byte == 7U);
  assert(window.y_start_px == 256U && window.y_end_px == 285U);
  assert(window.transferBytes() == 180U);

  assert(display::Ssd1680FrameMapper::nativeWindowForLogicalRegion(
      {0U, 0U, 1U, 1U}, display::Ssd1680Rotation::Clockwise90, window));
  assert(window.x_start_byte == 15U && window.x_end_byte == 15U);
  assert(window.y_start_px == 0U && window.y_end_px == 0U);
  assert(window.transferBytes() == 1U);

  assert(!display::Ssd1680FrameMapper::nativeWindowForLogicalRegion(
      {}, display::Ssd1680Rotation::Clockwise90, window));
}

} // namespace

int main() {
  static_assert(display::DisplayMonochromeRaster::kBufferBytes == 4'736U);
  static_assert(display::Ssd1680FrameMapper::kNativeBufferBytes == 4'736U);

  testDirtyRegionGeometry();
  testNativeWindowMapping();

  display::DisplayMonochromeRaster::Buffer buffer{};
  display::DisplayMonochromeRaster raster(buffer);
  raster.clearWhite();
  assert(
      std::all_of(buffer.begin(), buffer.end(), [](std::uint8_t value) { return value == 0xFFU; }));

  const auto normal_a = textElement(0U, 0U, 6U, 10U, "A");
  assert(raster.drawText(normal_a));
  assert(!raster.isBlack(0U, 0U));
  assert(raster.isBlack(1U, 0U));
  assert(raster.isBlack(2U, 0U));
  assert(raster.isBlack(3U, 0U));
  assert(!raster.isBlack(4U, 0U));
  for (std::uint16_t x = 0U; x < 5U; ++x) {
    assert(raster.isBlack(x, 3U));
  }

  raster.clearWhite();
  const auto lowercase = textElement(8U, 8U, 6U, 10U, "a");
  assert(raster.drawText(lowercase));
  assert(raster.isBlack(9U, 8U));
  assert(raster.isBlack(12U, 11U));

  raster.clearWhite();
  const auto scaled = textElement(10U, 10U, 20U, 15U, "A");
  assert(raster.drawText(scaled));
  assert(!raster.isBlack(10U, 10U));
  assert(raster.isBlack(12U, 10U));
  assert(raster.isBlack(13U, 11U));
  assert(raster.isBlack(17U, 11U));
  assert(!raster.isBlack(18U, 10U));

  raster.clearWhite();
  const auto clipped_width = textElement(0U, 20U, 5U, 10U, "AB");
  assert(raster.drawText(clipped_width));
  assert(raster.isBlack(1U, 20U));
  assert(!raster.isBlack(6U, 20U));

  raster.clearWhite();
  const auto clipped_height = textElement(20U, 125U, 6U, 10U, "A");
  assert(raster.drawText(clipped_height));
  assert(raster.isBlack(21U, 125U));
  assert(!raster.isBlack(21U, 127U));
  assert(!raster.isBlack(display::DisplayMonochromeRaster::kWidthPx, 0U));
  assert(!raster.isBlack(0U, display::DisplayMonochromeRaster::kHeightPx));

  const auto empty = textElement(0U, 0U, 20U, 10U, "");
  assert(!raster.drawText(empty));
  const auto zero_width = textElement(0U, 0U, 0U, 10U, "A");
  assert(!raster.drawText(zero_width));

  buffer.fill(0xFFU);
  setLogicalBlack(buffer, 0U, 0U);
  setLogicalBlack(buffer, 295U, 127U);
  std::uint8_t native = 0U;
  assert(display::Ssd1680FrameMapper::nativeByteAt(buffer, 15U,
                                                   display::Ssd1680Rotation::Clockwise90, native));
  assert(native == 0xFEU);
  assert(display::Ssd1680FrameMapper::nativeByteAt(
      buffer, 295U * display::Ssd1680FrameMapper::kNativeBytesPerRow,
      display::Ssd1680Rotation::Clockwise90, native));
  assert(native == 0x7FU);

  assert(display::Ssd1680FrameMapper::nativeByteAt(
      buffer, 295U * display::Ssd1680FrameMapper::kNativeBytesPerRow,
      display::Ssd1680Rotation::CounterClockwise90, native));
  assert(native == 0x7FU);
  assert(display::Ssd1680FrameMapper::nativeByteAt(
      buffer, 15U, display::Ssd1680Rotation::CounterClockwise90, native));
  assert(native == 0xFEU);
  assert(display::Ssd1680FrameMapper::nativeByteAt(buffer, 16U,
                                                   display::Ssd1680Rotation::Clockwise90, native));
  assert(native == 0xFFU);
  assert(!display::Ssd1680FrameMapper::nativeByteAt(buffer,
                                                    display::Ssd1680FrameMapper::kNativeBufferBytes,
                                                    display::Ssd1680Rotation::Clockwise90, native));

  return 0;
}
