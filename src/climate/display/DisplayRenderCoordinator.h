#pragma once

#include "climate/display/DisplayRenderAdapter.h"
#include "climate/display/DisplayTelemetryObserver.h"

#include <cstdint>

namespace growbox::app::climate_io::display {
namespace detail {

template <typename Backend> class RefreshAwareDisplaySink final {
public:
  RefreshAwareDisplaySink(Backend& backend, DisplayRefreshKind refresh_kind) noexcept
      : backend_(backend), refresh_kind_(refresh_kind) {}

  bool beginFrame(std::uint16_t width_px, std::uint16_t height_px, bool warning) noexcept {
    return backend_.beginFrame(width_px, height_px, warning, refresh_kind_);
  }

  bool drawText(const DisplayTextElement& element) noexcept {
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

// Render one immutable planned frame without touching observer/runtime state. This
// is the worker-safe half of the transaction: the owner thread remains
// responsible for acknowledging successful hardware completion.
template <typename Backend>
bool renderDisplayFrame(const DisplayRuntimeFrame& frame, const DisplayRenderGeometry& geometry,
                        const DisplayTheme& theme, Backend& backend) noexcept {
  if (!frame.refreshRequired()) {
    return false;
  }

  detail::RefreshAwareDisplaySink<Backend> sink{backend, frame.refresh_kind};
  return renderDisplayList(frame.render_list, geometry, theme, sink);
}

// Transactional bridge from a pending display frame to a display backend.
// The backend receives the planned e-ink refresh kind and the observer is
// acknowledged only after begin/draw/end all succeed. Failed in-progress frames
// are explicitly cancelled and remain pending for a later retry.
template <typename Backend>
bool renderPendingDisplay(DisplayTelemetryObserver& observer, const DisplayTheme& theme,
                          Backend& backend, std::uint64_t rendered_at_ms) noexcept {
  if (!observer.hasFrame() || !observer.hasPendingRefresh()) {
    return false;
  }

  const DisplayRuntimeFrame& frame = observer.lastFrame();
  if (!renderDisplayFrame(frame, observer.geometry(), theme, backend)) {
    return false;
  }

  return observer.confirmRendered(rendered_at_ms);
}

} // namespace growbox::app::climate_io::display
