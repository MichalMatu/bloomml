#pragma once

#include "climate/display/DisplayPresenter.h"

#include <cstddef>

namespace growbox::app::climate_io::display {

// Surface contract used by host simulators and concrete display adapters.
// A surface must provide noexcept-compatible methods:
//   bool beginPage(const char* title, bool warning)
//   bool drawLine(std::size_t index, const char* label, const char* value)
//   bool endPage()
// The presenter remains the owner of page content; surfaces only render it.
template <typename Surface>
bool renderDisplayPage(const DisplayPageModel& page, Surface& surface) noexcept {
  if (page.line_count > page.lines.size() || page.title[0] == '\0') {
    return false;
  }

  if (!surface.beginPage(page.title.data(), page.warning)) {
    return false;
  }

  for (std::size_t index = 0U; index < page.line_count; ++index) {
    const auto& line = page.lines[index];
    if (!surface.drawLine(index, line.label.data(), line.value.data())) {
      return false;
    }
  }

  return surface.endPage();
}

} // namespace growbox::app::climate_io::display
