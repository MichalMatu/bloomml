#pragma once

#include "climate/display/ClayDisplayAdapter.h"
#include "climate/display/DisplayTelemetryObserver.h"

#include <cstdint>

namespace growbox::app::climate_io::display {
namespace detail {

template <typename Backend> class RefreshAwareClaySink final {
public:
  RefreshAwareClaySink(Backend& backend, DisplayRefreshKind refresh_kind) noexcept
      : backend_(backend), refresh_kind_(refresh_kind) {}

  bool beginFrame(std::uint16_t width_px, std::uint16_t height_px, bool warning) noexcept {
    return backend_.beginFrame(width_px, height_px, warning, refresh_kind_);
  }

  bool drawText(const ClayDisplayTextElement& element) noexcept {
    return backend_.drawText(element);
  }

  bool endFrame() noexcept {
    return backend_.endFrame();
  }

  void cancelFrame() noexcept {
    backend_.cancelFrame();
  }

private:
  Backend& backend_;
  DisplayRefreshKind refresh_kind_{DisplayRefreshKind::None};
};

} // namespace detail

// Transactional bridge from a pending display frame to a Clay-facing backend.
// The backend receives the planned e-ink refresh kind and the observer is
// acknowledged only after begin/draw/end all succeed. Failed in-progress frames
// are explicitly cancelled and remain pending for a later retry.
template <typename Backend>
bool renderPendingDisplayToClay(DisplayTelemetryObserver& observer, const ClayDisplayTheme& theme,
                                Backend& backend, std::uint64_t rendered_at_ms) noexcept {
  if (!observer.hasFrame() || !observer.hasPendingRefresh()) {
    return false;
  }

  const DisplayRuntimeFrame& frame = observer.lastFrame();
  if (!frame.refreshRequired()) {
    return false;
  }

  detail::RefreshAwareClaySink<Backend> sink{backend, frame.refresh_kind};
  if (!renderDisplayListToClay(frame.render_list, observer.geometry(), theme, sink)) {
    return false;
  }

  return observer.confirmRendered(rendered_at_ms);
}

} // namespace growbox::app::climate_io::display
