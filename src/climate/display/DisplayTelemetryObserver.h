#pragma once

#include "climate/display/DisplayRuntime.h"
#include "climate/display/DisplaySnapshot.h"

#include <array>
#include <cstdint>
#include <cstdio>

namespace growbox::app::climate_io::display {

// Read-only bridge from the authoritative Stage27 telemetry/storage snapshots into
// the display lifecycle. It never owns or mutates climate/output control state.
class DisplayTelemetryObserver final {
public:
  DisplayTelemetryObserver(const DisplayEndpointRoles& roles,
                           const DisplayRuntimeConfig& config = {}) noexcept
      : roles_(roles), runtime_(config) {}

  DisplayTelemetryObserver(const DisplayEndpointRoles& roles, const char* firmware_sha,
                           const DisplayRuntimeConfig& config = {}) noexcept
      : roles_(roles), runtime_(config) {
    std::snprintf(firmware_sha_.data(), firmware_sha_.size(), "%.10s",
                  firmware_sha != nullptr ? firmware_sha : "");
  }

  bool observe(const telemetry::Stage27TelemetrySnapshot& telemetry_snapshot,
               const storage::Stage27StorageStatus& storage_status) noexcept {
    DisplaySnapshot projected{};
    if (!buildDisplaySnapshot(telemetry_snapshot, storage_status, roles_, projected)) {
      ++projection_error_count_;
      return false;
    }
    projected.firmware_sha = firmware_sha_;

    DisplayRuntimeFrame frame{};
    if (!runtime_.update(projected, telemetry_snapshot.uptime_ms, frame)) {
      ++runtime_error_count_;
      return false;
    }

    last_snapshot_ = projected;
    last_frame_ = frame;
    last_observed_ms_ = telemetry_snapshot.uptime_ms;
    has_snapshot_ = true;
    has_frame_ = true;
    return true;
  }

  bool confirmRendered(const DisplayRuntimeFrame& frame, std::uint64_t rendered_at_ms) noexcept {
    return runtime_.confirmRendered(frame, rendered_at_ms);
  }

  bool confirmRendered(std::uint64_t rendered_at_ms) noexcept {
    if (!has_frame_ || !last_frame_.refreshRequired()) {
      return false;
    }
    return confirmRendered(last_frame_, rendered_at_ms);
  }

  bool hasPendingRefresh() const noexcept {
    return runtime_.hasPendingRefresh();
  }

  bool handleButtonEvent(const DisplayButtonEvent& event) noexcept {
    const bool changed = runtime_.handleButtonEvent(event);
    if (!changed || !has_snapshot_) {
      return changed;
    }

    DisplayRuntimeFrame frame{};
    const std::uint64_t now_ms = event.timestamp_ms != 0U ? event.timestamp_ms : last_observed_ms_;
    if (!runtime_.update(last_snapshot_, now_ms, frame)) {
      ++runtime_error_count_;
      return changed;
    }

    last_frame_ = frame;
    has_frame_ = true;
    return changed;
  }

  bool handleButton(DisplayButton button) noexcept {
    return runtime_.handleButton(button);
  }

  DisplayViewMode viewMode() const noexcept {
    return runtime_.viewMode();
  }

  bool menuActive() const noexcept {
    return runtime_.menuActive();
  }

  void requestRefresh(bool full_refresh = false) noexcept {
    runtime_.requestRefresh(full_refresh);
  }

  bool hasSnapshot() const noexcept {
    return has_snapshot_;
  }

  bool hasFrame() const noexcept {
    return has_frame_;
  }

  const DisplaySnapshot& lastSnapshot() const noexcept {
    return last_snapshot_;
  }

  const DisplayRuntimeFrame& lastFrame() const noexcept {
    return last_frame_;
  }

  const DisplayRenderGeometry& geometry() const noexcept {
    return runtime_.geometry();
  }

  DisplayPage page() const noexcept {
    return runtime_.page();
  }

  std::uint32_t projectionErrorCount() const noexcept {
    return projection_error_count_;
  }

  std::uint32_t runtimeErrorCount() const noexcept {
    return runtime_error_count_;
  }

private:
  DisplayEndpointRoles roles_{};
  std::array<char, DisplaySnapshot::kFirmwareShaChars + 1U> firmware_sha_{};
  DisplayRuntimeController runtime_{};
  DisplaySnapshot last_snapshot_{};
  DisplayRuntimeFrame last_frame_{};
  std::uint64_t last_observed_ms_{0U};
  std::uint32_t projection_error_count_{0U};
  std::uint32_t runtime_error_count_{0U};
  bool has_snapshot_{false};
  bool has_frame_{false};
};

} // namespace growbox::app::climate_io::display
