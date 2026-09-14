#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace growbox::app::climate_io::display {

enum class DisplayTextRole : std::uint8_t {
  Title = 0U,
  WarningMarker,
  Label,
  Value,
};

struct DisplayRenderGeometry final {
  std::uint16_t width_px{296U};
  std::uint16_t height_px{128U};
  std::uint16_t left_margin_px{8U};
  std::uint16_t right_margin_px{8U};
  std::uint16_t title_y_px{14U};
  std::uint16_t first_row_y_px{28U};
  std::uint16_t row_height_px{9U};
  std::uint16_t value_x_px{92U};
  bool show_title{false};
};

struct DisplayTextCommand final {
  std::uint16_t x_px{0U};
  std::uint16_t y_px{0U};
  std::uint16_t max_width_px{0U};
  DisplayTextRole role{DisplayTextRole::Label};
  std::array<char, 32> text{};
};

struct DisplayRenderList final {
  static constexpr std::size_t kMaxCommands = 22U;

  std::array<DisplayTextCommand, kMaxCommands> commands{};
  std::size_t command_count{0U};
  bool warning{false};
};

class DisplayRenderListSurface final {
public:
  DisplayRenderListSurface(const DisplayRenderGeometry& geometry,
                           DisplayRenderList& output) noexcept
      : geometry_(geometry), output_(output) {}

  bool beginPage(const char* title, bool warning) noexcept {
    output_ = {};
    page_open_ = false;

    if (title == nullptr || title[0] == '\0' || !geometryValid()) {
      return false;
    }

    output_.warning = warning;
    if (geometry_.show_title) {
      if (warning && !appendText(geometry_.left_margin_px, geometry_.title_y_px, 12U,
                                 DisplayTextRole::WarningMarker, "!")) {
        return false;
      }

      const std::uint16_t title_x =
          static_cast<std::uint16_t>(geometry_.left_margin_px + (warning ? 14U : 0U));
      if (title_x >= geometry_.width_px - geometry_.right_margin_px) {
        output_ = {};
        return false;
      }

      if (!appendText(
              title_x, geometry_.title_y_px,
              static_cast<std::uint16_t>(geometry_.width_px - geometry_.right_margin_px - title_x),
              DisplayTextRole::Title, title)) {
        output_ = {};
        return false;
      }
    } else if (warning) {
      constexpr std::uint16_t kWarningWidthPx = 12U;
      const std::uint16_t warning_x = static_cast<std::uint16_t>(
          geometry_.width_px - geometry_.right_margin_px - kWarningWidthPx);
      const std::uint16_t warning_y =
          geometry_.title_y_px > 10U ? static_cast<std::uint16_t>(geometry_.title_y_px - 10U) : 0U;
      if (!appendText(warning_x, warning_y, kWarningWidthPx, DisplayTextRole::WarningMarker, "!")) {
        output_ = {};
        return false;
      }
    }

    page_open_ = true;
    return true;
  }

  bool drawLine(std::size_t index, const char* label, const char* value) noexcept {
    if (!page_open_ || label == nullptr || value == nullptr || index >= 10U) {
      return false;
    }

    const std::uint16_t row_origin_y =
        geometry_.show_title ? geometry_.first_row_y_px : geometry_.title_y_px;
    const std::uint32_t y = static_cast<std::uint32_t>(row_origin_y) +
                            static_cast<std::uint32_t>(index) * geometry_.row_height_px;
    if (y >= geometry_.height_px) {
      return false;
    }

    if (!appendText(geometry_.left_margin_px, static_cast<std::uint16_t>(y),
                    static_cast<std::uint16_t>(geometry_.value_x_px - geometry_.left_margin_px),
                    DisplayTextRole::Label, label)) {
      return false;
    }

    if (!appendText(geometry_.value_x_px, static_cast<std::uint16_t>(y),
                    static_cast<std::uint16_t>(geometry_.width_px - geometry_.right_margin_px -
                                               geometry_.value_x_px),
                    DisplayTextRole::Value, value)) {
      return false;
    }
    return true;
  }

  bool endPage() noexcept {
    if (!page_open_) {
      return false;
    }
    page_open_ = false;
    return output_.command_count > 0U;
  }

private:
  bool geometryValid() const noexcept {
    const std::uint16_t row_origin_y =
        geometry_.show_title ? geometry_.first_row_y_px : geometry_.title_y_px;
    return geometry_.width_px > geometry_.left_margin_px + geometry_.right_margin_px &&
           geometry_.height_px > geometry_.title_y_px && geometry_.height_px > row_origin_y &&
           geometry_.row_height_px > 0U && geometry_.value_x_px > geometry_.left_margin_px &&
           geometry_.value_x_px < geometry_.width_px - geometry_.right_margin_px;
  }

  bool appendText(std::uint16_t x, std::uint16_t y, std::uint16_t max_width, DisplayTextRole role,
                  const char* text) noexcept {
    if (output_.command_count >= output_.commands.size() || max_width == 0U) {
      return false;
    }

    DisplayTextCommand& command = output_.commands[output_.command_count];
    command.x_px = x;
    command.y_px = y;
    command.max_width_px = max_width;
    command.role = role;
    const int written = std::snprintf(command.text.data(), command.text.size(), "%s", text);
    if (written < 0 || static_cast<std::size_t>(written) >= command.text.size()) {
      return false;
    }
    ++output_.command_count;
    return true;
  }

  const DisplayRenderGeometry& geometry_;
  DisplayRenderList& output_;
  bool page_open_{false};
};

} // namespace growbox::app::climate_io::display
