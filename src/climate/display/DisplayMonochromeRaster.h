#pragma once

#include "climate/display/ClayDisplayAdapter.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {

// Fixed-memory 1-bpp landscape canvas matching the presenter's 296x128 geometry.
// SSD1680 RAM orientation is intentionally left to the hardware transport so the
// UI raster stays host-testable and independent from panel scan direction.
class DisplayMonochromeRaster final {
public:
  static constexpr std::uint16_t kWidthPx = 296U;
  static constexpr std::uint16_t kHeightPx = 128U;
  static constexpr std::size_t kBytesPerRow = kWidthPx / 8U;
  static constexpr std::size_t kBufferBytes = kBytesPerRow * kHeightPx;
  using Buffer = std::array<std::uint8_t, kBufferBytes>;

  explicit DisplayMonochromeRaster(Buffer& buffer) noexcept : buffer_(buffer) {}

  void clearWhite() noexcept;
  bool drawText(const ClayDisplayTextElement& element) noexcept;

  bool isBlack(std::uint16_t x_px, std::uint16_t y_px) const noexcept;
  const Buffer& buffer() const noexcept { return buffer_; }

private:
  void setBlack(std::uint16_t x_px, std::uint16_t y_px) noexcept;

  Buffer& buffer_;
};

static_assert(DisplayMonochromeRaster::kWidthPx % 8U == 0U);
static_assert(DisplayMonochromeRaster::kBufferBytes == 4'736U);

} // namespace growbox::app::climate_io::display
