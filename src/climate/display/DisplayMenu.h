#pragma once

#include "climate/display/DisplayPresenter.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {

// Read-only operator page chooser state. It owns only whether the chooser is
// visible and which existing status page is highlighted. DisplayNavigation
// remains the owner of the committed page.
class DisplayMenuState final {
public:
  static constexpr std::size_t kItemCount = 4U;

  bool active() const noexcept {
    return active_;
  }

  DisplayPage selectedPage() const noexcept {
    return static_cast<DisplayPage>(selected_index_);
  }

  bool open(DisplayPage current_page) noexcept;
  bool close() noexcept;
  bool previous() noexcept;
  bool next() noexcept;

private:
  static std::uint8_t pageIndex(DisplayPage page) noexcept;

  std::uint8_t selected_index_{0U};
  bool active_{false};
};

// Builds the chooser into the existing neutral semantic page model. Warning
// identity is inherited from the underlying status page so opening the menu
// cannot hide safety/clock/storage/supervisor warnings.
bool buildDisplayMenuPage(const DisplayPageModel& underlying_page, const DisplayMenuState& menu,
                          DisplayPageModel& output) noexcept;

} // namespace growbox::app::climate_io::display
