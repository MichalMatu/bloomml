#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::display_model {

struct DisplayLine final {
  std::array<char, 12> label{};
  std::array<char, 28> value{};
};

struct DisplayPageModel final {
  static constexpr std::size_t kMaxLines = 10U;

  std::array<char, 20> title{};
  std::array<DisplayLine, kMaxLines> lines{};
  std::size_t line_count{0U};
  std::uint8_t warning_mask{0U};
  std::uint32_t safety_warning_reason_code{0U};
  bool warning{false};
};

} // namespace growbox::display_model
