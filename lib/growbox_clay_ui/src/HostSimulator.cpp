#include "growbox_clay_ui/HostSimulator.h"

#include <cstdio>
#include <vector>

namespace growbox::clay_ui {

bool MonochromeFrame::pixel(std::uint16_t x, std::uint16_t y) const noexcept {
  if (x >= kDisplayWidth || y >= kDisplayHeight) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y) * kFrameStrideBytes + x / 8U;
  const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
  return (bytes[index] & mask) != 0U;
}

bool renderHostPage(const ::growbox::display_model::DisplayPageModel& page, MonochromeFrame& frame,
                    RenderSummary* summary) noexcept {
  try {
    std::vector<std::uint8_t> arena(pageRendererArenaBytes());
    return renderPageToMonochrome(page, frame.bytes.data(), frame.bytes.size(), arena.data(),
                                  arena.size(), summary);
  } catch (...) {
    return false;
  }
}

bool renderHostSmoke(MonochromeFrame& frame, RenderSummary* summary) noexcept {
  ::growbox::display_model::DisplayPageModel page{};
  std::snprintf(page.title.data(), page.title.size(), "Clay smoke");
  page.line_count = 1U;
  std::snprintf(page.lines[0].label.data(), page.lines[0].label.size(), "Render");
  std::snprintf(page.lines[0].value.data(), page.lines[0].value.size(), "OK");
  return renderHostPage(page, frame, summary);
}

bool writePbm(const MonochromeFrame& frame, const char* path) noexcept {
  if (path == nullptr) {
    return false;
  }
  std::FILE* file = std::fopen(path, "wb");
  if (file == nullptr) {
    return false;
  }
  const int header = std::fprintf(file, "P4\n%u %u\n", static_cast<unsigned>(kDisplayWidth),
                                  static_cast<unsigned>(kDisplayHeight));
  const std::size_t written = std::fwrite(frame.bytes.data(), 1, frame.bytes.size(), file);
  const int closed = std::fclose(file);
  return header > 0 && written == frame.bytes.size() && closed == 0;
}

} // namespace growbox::clay_ui
