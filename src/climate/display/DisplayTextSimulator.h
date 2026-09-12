#pragma once

#include <array>
#include <cstddef>

namespace growbox::app::climate_io::display {

class DisplayTextSimulator final {
public:
  static constexpr std::size_t kBufferSize = 640U;

  void reset() noexcept;
  bool beginPage(const char* title, bool warning) noexcept;
  bool drawLine(std::size_t index, const char* label, const char* value) noexcept;
  bool endPage() noexcept;

  const char* text() const noexcept {
    return buffer_.data();
  }

  std::size_t size() const noexcept {
    return size_;
  }

private:
  bool append(const char* format, ...) noexcept;

  std::array<char, kBufferSize> buffer_{};
  std::size_t size_{0U};
  bool page_open_{false};
};

} // namespace growbox::app::climate_io::display
