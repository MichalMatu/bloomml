#include "climate/display/DisplayMonochromeRaster.h"

#include <algorithm>

namespace growbox::app::climate_io::display {
namespace {

using Glyph = std::array<std::uint8_t, 7U>;

constexpr Glyph glyphFor(char input) noexcept {
  const char c = input >= 'a' && input <= 'z' ? static_cast<char>(input - 'a' + 'A') : input;
  switch (c) {
  case ' ':
    return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
  case '!':
    return {0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x00U, 0x04U};
  case '%':
    return {0x19U, 0x1AU, 0x02U, 0x04U, 0x08U, 0x0BU, 0x13U};
  case '-':
    return {0x00U, 0x00U, 0x00U, 0x1FU, 0x00U, 0x00U, 0x00U};
  case '.':
    return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x04U};
  case '/':
    return {0x01U, 0x02U, 0x02U, 0x04U, 0x08U, 0x08U, 0x10U};
  case ':':
    return {0x00U, 0x04U, 0x04U, 0x00U, 0x04U, 0x04U, 0x00U};
  case '>':
    return {0x10U, 0x08U, 0x04U, 0x02U, 0x04U, 0x08U, 0x10U};
  case '_':
    return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU};
  case '0':
    return {0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU};
  case '1':
    return {0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
  case '2':
    return {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x08U, 0x1FU};
  case '3':
    return {0x1EU, 0x01U, 0x01U, 0x0EU, 0x01U, 0x01U, 0x1EU};
  case '4':
    return {0x02U, 0x06U, 0x0AU, 0x12U, 0x1FU, 0x02U, 0x02U};
  case '5':
    return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x01U, 0x01U, 0x1EU};
  case '6':
    return {0x0EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU};
  case '7':
    return {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x08U, 0x08U};
  case '8':
    return {0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU};
  case '9':
    return {0x0EU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x0EU};
  case 'A':
    return {0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
  case 'B':
    return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x11U, 0x11U, 0x1EU};
  case 'C':
    return {0x0EU, 0x11U, 0x10U, 0x10U, 0x10U, 0x11U, 0x0EU};
  case 'D':
    return {0x1EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x1EU};
  case 'E':
    return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU};
  case 'F':
    return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U};
  case 'G':
    return {0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0FU};
  case 'H':
    return {0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
  case 'I':
    return {0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
  case 'J':
    return {0x07U, 0x02U, 0x02U, 0x02U, 0x12U, 0x12U, 0x0CU};
  case 'K':
    return {0x11U, 0x12U, 0x14U, 0x18U, 0x14U, 0x12U, 0x11U};
  case 'L':
    return {0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x1FU};
  case 'M':
    return {0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U};
  case 'N':
    return {0x11U, 0x19U, 0x19U, 0x15U, 0x13U, 0x13U, 0x11U};
  case 'O':
    return {0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
  case 'P':
    return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x10U, 0x10U};
  case 'Q':
    return {0x0EU, 0x11U, 0x11U, 0x11U, 0x15U, 0x12U, 0x0DU};
  case 'R':
    return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U};
  case 'S':
    return {0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU};
  case 'T':
    return {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U};
  case 'U':
    return {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
  case 'V':
    return {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U};
  case 'W':
    return {0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU};
  case 'X':
    return {0x11U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U, 0x11U};
  case 'Y':
    return {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U};
  case 'Z':
    return {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x1FU};
  default:
    return {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x00U, 0x04U};
  }
}

} // namespace

void DisplayMonochromeRaster::clearWhite() noexcept {
  buffer_.fill(0xFFU);
}

bool DisplayMonochromeRaster::drawText(const DisplayTextElement& element) noexcept {
  if (element.text == nullptr || element.text[0] == '\0' || element.max_width_px == 0U ||
      element.style.font_size_px == 0U || element.x_px >= kWidthPx || element.y_px >= kHeightPx) {
    return false;
  }

  const std::uint16_t scale = element.style.font_size_px >= 14U ? 2U : 1U;
  const std::uint16_t glyph_width = static_cast<std::uint16_t>(5U * scale);
  const std::uint16_t advance = static_cast<std::uint16_t>(6U * scale);
  const std::uint32_t requested_right =
      static_cast<std::uint32_t>(element.x_px) + element.max_width_px;
  const std::uint16_t right = static_cast<std::uint16_t>(
      std::min<std::uint32_t>(requested_right, static_cast<std::uint32_t>(kWidthPx)));

  std::uint16_t cursor_x = element.x_px;
  bool processed = false;
  for (const char* text = element.text; *text != '\0'; ++text) {
    if (static_cast<std::uint32_t>(cursor_x) + glyph_width > right) {
      break;
    }

    const Glyph glyph = glyphFor(*text);
    for (std::uint16_t row = 0U; row < glyph.size(); ++row) {
      for (std::uint16_t column = 0U; column < 5U; ++column) {
        const std::uint8_t mask = static_cast<std::uint8_t>(1U << (4U - column));
        if ((glyph[row] & mask) == 0U) {
          continue;
        }
        for (std::uint16_t dy = 0U; dy < scale; ++dy) {
          const std::uint32_t y = static_cast<std::uint32_t>(element.y_px) + row * scale + dy;
          if (y >= kHeightPx) {
            continue;
          }
          for (std::uint16_t dx = 0U; dx < scale; ++dx) {
            const std::uint16_t x = static_cast<std::uint16_t>(cursor_x + column * scale + dx);
            if (x < right) {
              setBlack(x, static_cast<std::uint16_t>(y));
            }
          }
        }
      }
    }

    processed = true;
    if (static_cast<std::uint32_t>(cursor_x) + advance >= right) {
      break;
    }
    cursor_x = static_cast<std::uint16_t>(cursor_x + advance);
  }

  return processed;
}

bool DisplayMonochromeRaster::isBlack(std::uint16_t x_px, std::uint16_t y_px) const noexcept {
  if (x_px >= kWidthPx || y_px >= kHeightPx) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y_px) * kBytesPerRow + x_px / 8U;
  const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> (x_px % 8U));
  return (buffer_[index] & mask) == 0U;
}

void DisplayMonochromeRaster::setBlack(std::uint16_t x_px, std::uint16_t y_px) noexcept {
  if (x_px >= kWidthPx || y_px >= kHeightPx) {
    return;
  }
  const std::size_t index = static_cast<std::size_t>(y_px) * kBytesPerRow + x_px / 8U;
  const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> (x_px % 8U));
  buffer_[index] = static_cast<std::uint8_t>(buffer_[index] & static_cast<std::uint8_t>(~mask));
}

} // namespace growbox::app::climate_io::display
