#pragma once

#include "climate/display/DisplayMenu.h"
#include "climate/display/DisplayPresenter.h"
#include "climate/display/DisplayRenderList.h"
#include "climate/display/DisplaySurface.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace growbox::app::climate_io::display {

enum class DisplayViewMode : std::uint8_t {
  Page = 0U,
  Menu,
};

enum class DisplayRefreshKind : std::uint8_t {
  None = 0U,
  Partial,
  Full,
};

enum class DisplayRefreshReason : std::uint8_t {
  None = 0U,
  Initial,
  Navigation,
  WarningChanged,
  ContentChanged,
  Requested,
};

struct DisplayRefreshPolicyConfig final {
  // Coalesce routine telemetry changes so an e-ink backend is not refreshed on
  // every runtime tick. Navigation and warning transitions intentionally bypass
  // this interval for operator feedback and safety visibility.
  std::uint64_t minimum_refresh_interval_ms{15'000U};

  // Recommend a full refresh periodically to give concrete e-ink backends a
  // deterministic ghosting-control hook. Zero disables cadence-based full refreshes.
  std::uint16_t full_refresh_every_partial{20U};
};

struct DisplayRuntimeConfig final {
  DisplayPresenterConfig presenter{};
  DisplayRenderGeometry geometry{};
  DisplayRefreshPolicyConfig refresh{};
};

struct DisplayRuntimeFrame final {
  DisplayPage page{DisplayPage::Status};
  DisplayViewMode view_mode{DisplayViewMode::Page};
  DisplayPageModel page_model{};
  DisplayRenderList render_list{};
  DisplayRefreshKind refresh_kind{DisplayRefreshKind::None};
  DisplayRefreshReason refresh_reason{DisplayRefreshReason::None};

  bool refreshRequired() const noexcept {
    return refresh_kind != DisplayRefreshKind::None;
  }
};

namespace detail {

inline bool displayWarningsEqual(const DisplayPageModel& left,
                                 const DisplayPageModel& right) noexcept {
  return left.warning == right.warning && left.warning_mask == right.warning_mask &&
         left.safety_warning_reason_code == right.safety_warning_reason_code;
}

inline bool displayPageModelsEqual(const DisplayPageModel& left,
                                   const DisplayPageModel& right) noexcept {
  if (!displayWarningsEqual(left, right) || left.line_count != right.line_count ||
      std::strcmp(left.title.data(), right.title.data()) != 0) {
    return false;
  }

  for (std::size_t index = 0U; index < left.line_count; ++index) {
    if (std::strcmp(left.lines[index].label.data(), right.lines[index].label.data()) != 0 ||
        std::strcmp(left.lines[index].value.data(), right.lines[index].value.data()) != 0) {
      return false;
    }
  }
  return true;
}

inline bool displayRenderListsEqual(const DisplayRenderList& left,
                                    const DisplayRenderList& right) noexcept {
  if (left.warning != right.warning || left.command_count != right.command_count) {
    return false;
  }

  for (std::size_t index = 0U; index < left.command_count; ++index) {
    const auto& left_command = left.commands[index];
    const auto& right_command = right.commands[index];
    if (left_command.x_px != right_command.x_px || left_command.y_px != right_command.y_px ||
        left_command.max_width_px != right_command.max_width_px ||
        left_command.role != right_command.role ||
        std::strcmp(left_command.text.data(), right_command.text.data()) != 0) {
      return false;
    }
  }
  return true;
}

inline bool displayRuntimeFramesEqual(const DisplayRuntimeFrame& left,
                                      const DisplayRuntimeFrame& right) noexcept {
  return left.page == right.page && left.view_mode == right.view_mode &&
         left.refresh_kind == right.refresh_kind && left.refresh_reason == right.refresh_reason &&
         displayPageModelsEqual(left.page_model, right.page_model) &&
         displayRenderListsEqual(left.render_list, right.render_list);
}

inline bool elapsedAtLeast(std::uint64_t now_ms, std::uint64_t since_ms,
                           std::uint64_t interval_ms) noexcept {
  if (interval_ms == 0U || now_ms < since_ms) {
    return true;
  }
  return now_ms - since_ms >= interval_ms;
}

} // namespace detail

// Host-testable lifecycle for the read-only operator display. update() only plans
// a refresh. Successful hardware rendering must be acknowledged with
// confirmRendered() before cadence/request state advances. No control/output state
// is mutated here.
class DisplayRuntimeController final {
public:
  explicit DisplayRuntimeController(const DisplayRuntimeConfig& config = {}) noexcept
      : config_(config) {}

  DisplayPage page() const noexcept {
    return navigation_.page();
  }

  DisplayViewMode viewMode() const noexcept {
    return menu_.active() ? DisplayViewMode::Menu : DisplayViewMode::Page;
  }

  bool menuActive() const noexcept {
    return menu_.active();
  }

  DisplayPage menuSelection() const noexcept {
    return menu_.selectedPage();
  }

  const DisplayRenderGeometry& geometry() const noexcept {
    return config_.geometry;
  }

  bool hasPendingRefresh() const noexcept {
    return pending_refresh_valid_;
  }

  bool handleButtonEvent(const DisplayButtonEvent& event) noexcept {
    bool changed = false;
    if (event.gesture == DisplayButtonGesture::LongPress) {
      if (event.button == DisplayButton::Ok && !menu_.active()) {
        changed = menu_.open(navigation_.page());
      }
    } else if (menu_.active()) {
      switch (event.button) {
      case DisplayButton::Home:
        changed = menu_.close();
        changed = navigation_.select(DisplayPage::Environment) || changed;
        break;
      case DisplayButton::Back:
        changed = menu_.close();
        break;
      case DisplayButton::Previous:
        changed = menu_.previous();
        break;
      case DisplayButton::Next:
        changed = menu_.next();
        break;
      case DisplayButton::Ok: {
        const DisplayPage selected = menu_.selectedPage();
        changed = menu_.close();
        changed = navigation_.select(selected) || changed;
        break;
      }
      }
    } else {
      changed = navigation_.handle(event.button);
    }

    if (changed) {
      refresh_requested_ = true;
      navigation_refresh_requested_ = true;
      pending_refresh_valid_ = false;
    }
    return changed;
  }

  bool handleButton(DisplayButton button) noexcept {
    return handleButtonEvent({button, DisplayButtonGesture::Press, 0U});
  }

  void requestRefresh(bool full_refresh = false) noexcept {
    refresh_requested_ = true;
    full_refresh_requested_ = full_refresh_requested_ || full_refresh;
    pending_refresh_valid_ = false;
  }

  bool update(const DisplaySnapshot& snapshot, std::uint64_t now_ms,
              DisplayRuntimeFrame& output) noexcept {
    output = {};
    output.page = navigation_.page();
    output.view_mode = viewMode();

    DisplayPageModel underlying_page{};
    if (!buildDisplayPage(snapshot, output.page, config_.presenter, underlying_page)) {
      pending_refresh_valid_ = false;
      return false;
    }
    if (menu_.active()) {
      if (!buildDisplayMenuPage(underlying_page, menu_, output.page_model)) {
        pending_refresh_valid_ = false;
        return false;
      }
    } else {
      output.page_model = underlying_page;
    }

    DisplayRenderListSurface surface{config_.geometry, output.render_list};
    if (!renderDisplayPage(output.page_model, surface)) {
      pending_refresh_valid_ = false;
      return false;
    }

    const bool initial = !has_last_frame_;
    const bool warning_changed =
        has_last_frame_ && !detail::displayWarningsEqual(output.page_model, last_page_);
    const bool content_changed =
        initial || !detail::displayPageModelsEqual(output.page_model, last_page_);
    const bool interval_elapsed =
        initial || detail::elapsedAtLeast(now_ms, last_refresh_ms_,
                                          config_.refresh.minimum_refresh_interval_ms);

    DisplayRefreshReason reason = DisplayRefreshReason::None;
    bool force_full = false;

    if (initial) {
      reason = DisplayRefreshReason::Initial;
      force_full = true;
    } else if (warning_changed) {
      reason = DisplayRefreshReason::WarningChanged;
      force_full = true;
    } else if (navigation_refresh_requested_) {
      reason = DisplayRefreshReason::Navigation;
    } else if (full_refresh_requested_) {
      reason = DisplayRefreshReason::Requested;
      force_full = true;
    } else if (refresh_requested_) {
      reason = DisplayRefreshReason::Requested;
    } else if (content_changed && interval_elapsed) {
      reason = DisplayRefreshReason::ContentChanged;
    }

    if (reason == DisplayRefreshReason::None) {
      pending_refresh_valid_ = false;
      return true;
    }

    output.refresh_reason = reason;
    output.refresh_kind = plannedRefreshKind(force_full);
    pending_frame_ = output;
    pending_refresh_valid_ = true;
    return true;
  }

  bool confirmRendered(const DisplayRuntimeFrame& frame, std::uint64_t rendered_at_ms) noexcept {
    if (!pending_refresh_valid_ || !frame.refreshRequired() ||
        !detail::displayRuntimeFramesEqual(frame, pending_frame_)) {
      return false;
    }

    last_page_ = pending_frame_.page_model;
    last_refresh_ms_ = rendered_at_ms;
    has_last_frame_ = true;

    if (pending_frame_.refresh_kind == DisplayRefreshKind::Full) {
      partial_refreshes_since_full_ = 0U;
    } else if (pending_frame_.refresh_kind == DisplayRefreshKind::Partial &&
               partial_refreshes_since_full_ < UINT16_MAX) {
      ++partial_refreshes_since_full_;
    }

    refresh_requested_ = false;
    full_refresh_requested_ = false;
    navigation_refresh_requested_ = false;
    pending_refresh_valid_ = false;
    return true;
  }

private:
  DisplayRefreshKind plannedRefreshKind(bool force_full) const noexcept {
    if (force_full) {
      return DisplayRefreshKind::Full;
    }

    const std::uint16_t cadence = config_.refresh.full_refresh_every_partial;
    if (cadence > 0U && static_cast<std::uint32_t>(partial_refreshes_since_full_) + 1U >= cadence) {
      return DisplayRefreshKind::Full;
    }
    return DisplayRefreshKind::Partial;
  }

  DisplayRuntimeConfig config_{};
  DisplayNavigation navigation_{};
  DisplayMenuState menu_{};
  DisplayPageModel last_page_{};
  DisplayRuntimeFrame pending_frame_{};
  std::uint64_t last_refresh_ms_{0U};
  std::uint16_t partial_refreshes_since_full_{0U};
  bool has_last_frame_{false};
  bool refresh_requested_{false};
  bool full_refresh_requested_{false};
  bool navigation_refresh_requested_{false};
  bool pending_refresh_valid_{false};
};

} // namespace growbox::app::climate_io::display
