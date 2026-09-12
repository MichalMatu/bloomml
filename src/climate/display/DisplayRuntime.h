#pragma once

#include "climate/display/DisplayPresenter.h"
#include "climate/display/DisplayRenderList.h"
#include "climate/display/DisplaySurface.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace growbox::app::climate_io::display {

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
  DisplayPageModel page_model{};
  DisplayRenderList render_list{};
  DisplayRefreshKind refresh_kind{DisplayRefreshKind::None};
  DisplayRefreshReason refresh_reason{DisplayRefreshReason::None};

  bool refreshRequired() const noexcept {
    return refresh_kind != DisplayRefreshKind::None;
  }
};

namespace detail {

inline bool displayPageModelsEqual(const DisplayPageModel& left,
                                   const DisplayPageModel& right) noexcept {
  if (left.warning != right.warning || left.line_count != right.line_count ||
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

inline bool elapsedAtLeast(std::uint64_t now_ms, std::uint64_t since_ms,
                           std::uint64_t interval_ms) noexcept {
  if (interval_ms == 0U || now_ms < since_ms) {
    return true;
  }
  return now_ms - since_ms >= interval_ms;
}

} // namespace detail

// Host-testable lifecycle for the read-only operator display. It consumes only
// DisplaySnapshot facts and produces presenter/render-list output plus a refresh
// recommendation. No control/output state is mutated here.
class DisplayRuntimeController final {
public:
  explicit DisplayRuntimeController(const DisplayRuntimeConfig& config = {}) noexcept
      : config_(config) {}

  DisplayPage page() const noexcept {
    return navigation_.page();
  }

  bool handleButton(DisplayButton button) noexcept {
    const bool changed = navigation_.handle(button);
    if (changed) {
      refresh_requested_ = true;
      navigation_refresh_requested_ = true;
    }
    return changed;
  }

  void requestRefresh(bool full_refresh = false) noexcept {
    refresh_requested_ = true;
    full_refresh_requested_ = full_refresh_requested_ || full_refresh;
  }

  bool update(const DisplaySnapshot& snapshot, std::uint64_t now_ms,
              DisplayRuntimeFrame& output) noexcept {
    output = {};
    output.page = navigation_.page();

    if (!buildDisplayPage(snapshot, output.page, config_.presenter, output.page_model)) {
      return false;
    }

    DisplayRenderListSurface surface{config_.geometry, output.render_list};
    if (!renderDisplayPage(output.page_model, surface)) {
      return false;
    }

    const bool initial = !has_last_frame_;
    const bool warning_changed = has_last_frame_ && output.page_model.warning != last_page_.warning;
    const bool content_changed = initial || !detail::displayPageModelsEqual(output.page_model, last_page_);
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
      return true;
    }

    output.refresh_reason = reason;
    output.refresh_kind = selectRefreshKind(force_full);

    last_page_ = output.page_model;
    last_refresh_ms_ = now_ms;
    has_last_frame_ = true;
    refresh_requested_ = false;
    full_refresh_requested_ = false;
    navigation_refresh_requested_ = false;
    return true;
  }

private:
  DisplayRefreshKind selectRefreshKind(bool force_full) noexcept {
    if (force_full) {
      partial_refreshes_since_full_ = 0U;
      return DisplayRefreshKind::Full;
    }

    const std::uint16_t cadence = config_.refresh.full_refresh_every_partial;
    if (cadence > 0U &&
        static_cast<std::uint32_t>(partial_refreshes_since_full_) + 1U >= cadence) {
      partial_refreshes_since_full_ = 0U;
      return DisplayRefreshKind::Full;
    }

    if (partial_refreshes_since_full_ < UINT16_MAX) {
      ++partial_refreshes_since_full_;
    }
    return DisplayRefreshKind::Partial;
  }

  DisplayRuntimeConfig config_{};
  DisplayNavigation navigation_{};
  DisplayPageModel last_page_{};
  std::uint64_t last_refresh_ms_{0U};
  std::uint16_t partial_refreshes_since_full_{0U};
  bool has_last_frame_{false};
  bool refresh_requested_{false};
  bool full_refresh_requested_{false};
  bool navigation_refresh_requested_{false};
};

} // namespace growbox::app::climate_io::display
