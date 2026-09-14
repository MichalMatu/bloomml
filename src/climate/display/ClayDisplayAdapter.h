#pragma once

#include "climate/display/DisplayRenderList.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::display {

struct ClayDisplayTextStyle final {
  std::uint16_t font_id{0U};
  std::uint16_t font_size_px{10U};
  bool emphasized{false};
};

struct ClayDisplayTheme final {
  ClayDisplayTextStyle title{0U, 15U, true};
  ClayDisplayTextStyle warning{0U, 15U, true};
  ClayDisplayTextStyle label{0U, 10U, false};
  ClayDisplayTextStyle value{0U, 10U, false};
};

struct ClayDisplayTextElement final {
  std::uint16_t x_px{0U};
  std::uint16_t y_px{0U};
  std::uint16_t max_width_px{0U};
  ClayDisplayTextStyle style{};
  const char* text{nullptr};
};

namespace detail {

inline bool clayThemeValid(const ClayDisplayTheme& theme) noexcept {
  return theme.title.font_size_px > 0U && theme.warning.font_size_px > 0U &&
         theme.label.font_size_px > 0U && theme.value.font_size_px > 0U;
}

inline const ClayDisplayTextStyle& clayStyleForRole(DisplayTextRole role,
                                                    const ClayDisplayTheme& theme) noexcept {
  switch (role) {
  case DisplayTextRole::Title:
    return theme.title;
  case DisplayTextRole::WarningMarker:
    return theme.warning;
  case DisplayTextRole::Label:
    return theme.label;
  case DisplayTextRole::Value:
    return theme.value;
  }
  return theme.label;
}

inline bool clayCommandFits(const DisplayTextCommand& command,
                            const DisplayRenderGeometry& geometry) noexcept {
  if (geometry.width_px == 0U || geometry.height_px == 0U || command.text[0] == '\0' ||
      command.max_width_px == 0U || command.x_px >= geometry.width_px ||
      command.y_px >= geometry.height_px) {
    return false;
  }

  return static_cast<std::uint32_t>(command.x_px) + command.max_width_px <= geometry.width_px;
}

} // namespace detail

// Narrow Clay-facing seam. A concrete Clay integration implements a sink with:
//   bool beginFrame(width_px, height_px, warning)
//   bool drawText(const ClayDisplayTextElement&)
//   bool endFrame()
//   void cancelFrame()
// cancelFrame() must abandon any incomplete software frame so a later retry can
// start cleanly. This layer owns validation and role-to-style mapping while
// keeping the authoritative presenter/render-list path independent from Clay and
// hardware.
template <typename ClaySink>
bool renderDisplayListToClay(const DisplayRenderList& render_list,
                             const DisplayRenderGeometry& geometry, const ClayDisplayTheme& theme,
                             ClaySink& sink) noexcept {
  if (render_list.command_count == 0U || render_list.command_count > render_list.commands.size() ||
      !detail::clayThemeValid(theme) || geometry.width_px == 0U || geometry.height_px == 0U) {
    return false;
  }

  for (std::size_t index = 0U; index < render_list.command_count; ++index) {
    if (!detail::clayCommandFits(render_list.commands[index], geometry)) {
      return false;
    }
  }

  if (!sink.beginFrame(geometry.width_px, geometry.height_px, render_list.warning)) {
    return false;
  }

  for (std::size_t index = 0U; index < render_list.command_count; ++index) {
    const auto& command = render_list.commands[index];
    const ClayDisplayTextElement element{
        command.x_px,         command.y_px,
        command.max_width_px, detail::clayStyleForRole(command.role, theme),
        command.text.data(),
    };
    if (!sink.drawText(element)) {
      sink.cancelFrame();
      return false;
    }
  }

  if (!sink.endFrame()) {
    sink.cancelFrame();
    return false;
  }
  return true;
}

} // namespace growbox::app::climate_io::display
