#pragma once

#include "climate/display/DisplaySnapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {

enum class DisplayPage : std::uint8_t { Status = 0U, Outputs, Diagnostics };
enum class DisplayButton : std::uint8_t { Home = 0U, Back, Previous, Next, Ok };

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
  std::uint8_t warning_mask{0U};
  std::uint32_t safety_warning_reason_code{0U};
  bool warning{false};
};

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
