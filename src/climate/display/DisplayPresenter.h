#pragma once

#include "climate/display/DisplayInputEvent.h"
#include "climate/display/DisplaySnapshot.h"
#include "growbox_display_model/DisplayPageModel.h"

#include <cstdint>

namespace growbox::app::climate_io::display {

enum class DisplayPage : std::uint8_t {
  Environment = 0U,
  // Compatibility alias for code/tests written before the operator UI was
  // split into explicit Environment / Outputs / System / Diagnostics pages.
  Status = Environment,
  Outputs = 1U,
  System = 2U,
  Diagnostics = 3U,
};

enum class DisplayWarning : std::uint8_t {
  Sensor = 1U << 0U,
  Clock = 1U << 1U,
  Storage = 1U << 2U,
  Safety = 1U << 3U,
  SupervisorFault = 1U << 4U,
};

class DisplayNavigation final {
public:
  DisplayPage page() const noexcept {
    return page_;
  }

  bool select(DisplayPage page) noexcept;
  bool handle(DisplayButton button) noexcept;

private:
  DisplayPage page_{DisplayPage::Environment};
};

using DisplayLine = ::growbox::display_model::DisplayLine;
using DisplayPageModel = ::growbox::display_model::DisplayPageModel;

struct DisplayPresenterConfig final {
  std::uint64_t sensor_stale_after_ms{30'000U};
};

bool buildDisplayPage(const DisplaySnapshot& snapshot, DisplayPage page,
                      const DisplayPresenterConfig& config, DisplayPageModel& output) noexcept;

inline bool buildDisplayPage(const DisplaySnapshot& snapshot, DisplayPage page,
                             DisplayPageModel& output) noexcept {
  return buildDisplayPage(snapshot, page, DisplayPresenterConfig{}, output);
}

} // namespace growbox::app::climate_io::display
