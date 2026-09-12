#pragma once

#include "display/DisplaySnapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::app::display {

enum class DisplayPage : std::uint8_t { Status = 0U, Outputs, Diagnostics };
enum class DisplayButton : std::uint8_t { Home = 0U, Back, Previous, Next, Ok };

class DisplayNavigation final {
public:
  DisplayPage page() const noexcept {
    return page_;
  }
  bool handle(DisplayButton button) noexcept;

private:
  DisplayPage page_{DisplayPage::Status};
};

struct DisplayLine final {
  std::array<char, 12> label{};
  std::array<char, 28> value{};
};

struct DisplayPageModel final {
  static constexpr std::size_t kMaxLines = 10U;

  std::array<char, 20> title{};
  std::array<DisplayLine, kMaxLines> lines{};
  std::size_t line_count{0U};
  bool warning{false};
};

bool buildDisplayPage(const DisplaySnapshot& snapshot, DisplayPage page,
                      DisplayPageModel& output) noexcept;

} // namespace growbox::app::display
