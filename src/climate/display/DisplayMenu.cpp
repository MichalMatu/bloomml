#include "climate/display/DisplayMenu.h"

#include <cstdio>

namespace growbox::app::climate_io::display {
namespace {

template <std::size_t N> void setText(std::array<char, N>& target, const char* text) noexcept {
  std::snprintf(target.data(), target.size(), "%s", text != nullptr ? text : "");
}

const char* pageName(DisplayPage page) noexcept {
  switch (page) {
  case DisplayPage::Environment:
    return "Environment";
  case DisplayPage::Outputs:
    return "Outputs";
  case DisplayPage::System:
    return "System";
  case DisplayPage::Diagnostics:
    return "Diagnostics";
  }
  return "Unknown";
}

} // namespace

std::uint8_t DisplayMenuState::pageIndex(DisplayPage page) noexcept {
  const auto index = static_cast<std::uint8_t>(page);
  return index < kItemCount ? index : 0U;
}

bool DisplayMenuState::open(DisplayPage current_page) noexcept {
  const std::uint8_t next_index = pageIndex(current_page);
  const bool changed = !active_ || selected_index_ != next_index;
  selected_index_ = next_index;
  active_ = true;
  return changed;
}

bool DisplayMenuState::close() noexcept {
  if (!active_) {
    return false;
  }
  active_ = false;
  return true;
}

bool DisplayMenuState::previous() noexcept {
  if (!active_) {
    return false;
  }
  selected_index_ = selected_index_ == 0U ? static_cast<std::uint8_t>(kItemCount - 1U)
                                          : static_cast<std::uint8_t>(selected_index_ - 1U);
  return true;
}

bool DisplayMenuState::next() noexcept {
  if (!active_) {
    return false;
  }
  selected_index_ = static_cast<std::uint8_t>((selected_index_ + 1U) % kItemCount);
  return true;
}

bool buildDisplayMenuPage(const DisplayPageModel& underlying_page, const DisplayMenuState& menu,
                          DisplayPageModel& output) noexcept {
  if (!menu.active()) {
    return false;
  }

  output = {};
  output.warning = underlying_page.warning;
  output.warning_mask = underlying_page.warning_mask;
  output.safety_warning_reason_code = underlying_page.safety_warning_reason_code;
  setText(output.title, "Pages");

  for (std::size_t index = 0U; index < DisplayMenuState::kItemCount; ++index) {
    if (output.line_count >= output.lines.size()) {
      return false;
    }
    DisplayLine& line = output.lines[output.line_count++];
    const DisplayPage page = static_cast<DisplayPage>(index);
    setText(line.label, page == menu.selectedPage() ? ">" : "");
    setText(line.value, pageName(page));
  }
  return true;
}

} // namespace growbox::app::climate_io::display
