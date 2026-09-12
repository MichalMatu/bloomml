#include "climate/display/DisplayTextSimulator.h"

#include <cstdarg>
#include <cstdio>

namespace growbox::app::climate_io::display {

void DisplayTextSimulator::reset() noexcept {
  buffer_ = {};
  size_ = 0U;
  page_open_ = false;
}

bool DisplayTextSimulator::append(const char* format, ...) noexcept {
  if (format == nullptr || size_ >= buffer_.size()) {
    return false;
  }

  const std::size_t remaining = buffer_.size() - size_;
  va_list args;
  va_start(args, format);
  const int written = std::vsnprintf(buffer_.data() + size_, remaining, format, args);
  va_end(args);

  if (written < 0 || static_cast<std::size_t>(written) >= remaining) {
    buffer_.back() = '\0';
    return false;
  }

  size_ += static_cast<std::size_t>(written);
  return true;
}

bool DisplayTextSimulator::beginPage(const char* title, bool warning) noexcept {
  reset();
  if (title == nullptr || title[0] == '\0') {
    return false;
  }

  page_open_ = append("%s%s\n", warning ? "! " : "  ", title);
  return page_open_;
}

bool DisplayTextSimulator::drawLine(std::size_t index, const char* label,
                                    const char* value) noexcept {
  if (!page_open_ || label == nullptr || value == nullptr) {
    return false;
  }

  return append("%02zu %-10s %s\n", index, label, value);
}

bool DisplayTextSimulator::endPage() noexcept {
  if (!page_open_) {
    return false;
  }
  page_open_ = false;
  return true;
}

} // namespace growbox::app::climate_io::display
