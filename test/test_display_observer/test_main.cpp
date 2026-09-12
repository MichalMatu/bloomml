#include "climate/display/DisplayClayCoordinator.h"
#include "climate/display/DisplayTelemetryObserver.h"
#include "climate/output/LampSafety.h"
#include "climate/output/OutputBindings.h"

#include <cassert>
#include <cstring>

namespace display = growbox::app::climate_io::display;
namespace output = growbox::app::output;
namespace stage28d = growbox::app::climate_io::stage28d;
namespace storage = growbox::app::climate_io::storage;
namespace telemetry = growbox::app::climate_io::telemetry;

namespace {

const display::DisplayLine* findLine(const display::DisplayPageModel& page, const char* label) {
  for (std::size_t index = 0U; index < page.line_count; ++index) {
    if (std::strcmp(page.lines[index].label.data(), label) == 0) {
      return &page.lines[index];
    }
  }
  return nullptr;
}

class FakeClayBackend final {
public:
  bool beginFrame(std::uint16_t width_px, std::uint16_t height_px, bool warning,
                  display::DisplayRefreshKind refresh_kind) noexcept {
    ++begin_count;
    width = width_px;
    height = height_px;
    frame_warning = warning;
    last_refresh_kind = refresh_kind;
    frame_open = begin_result;
    return begin_result;
  }

  bool drawText(const display::ClayDisplayTextElement& element) noexcept {
    if (!frame_open || element.text == nullptr) {
      return false;
    }
    ++draw_count;
    return draw_result;
  }

  bool endFrame() noexcept {
    if (!frame_open) {
      return false;
    }
    ++end_count;
    frame_open = false;
    return end_result;
  }

  bool begin_result{true};
  bool draw_result{true};
  bool end_result{true};
  bool frame_open{false};
  bool frame_warning{false};
  std::uint16_t width{0U};
  std::uint16_t height{0U};
  std::size_t begin_count{0U};
  std::size_t draw_count{0U};
  std::size_t end_count{0U};
  display::DisplayRefreshKind last_refresh_kind{display::DisplayRefreshKind::None};
};

telemetry::Stage27TelemetrySnapshot nominalTelemetry(std::uint64_t uptime_ms) {
  telemetry::Stage27TelemetrySnapshot snapshot{};
  snapshot.uptime_ms = uptime_ms;
  snapshot.unix_time_s = 1'767'225'600U;
  snapshot.input_sampled = true;
  snapshot.scd_available = true;
  snapshot.scd_sample = true;
  snapshot.scd_temperature_c = 23.4F;
  snapshot.scd_humidity_pct = 61.2F;
  snapshot.scd_co2_ppm = 712.0F;
  snapshot.scd_age_ms = 2'000U;
  snapshot.rtc_available = true;
  snapshot.rtc_trusted = true;
  snapshot.ble_scanning = true;

  snapshot.output.version = output::OutputExecutionTelemetrySnapshot::kVersion;
  snapshot.output.mode = output::SupervisorMode::Automatic;
  snapshot.output.transport_active = true;
  snapshot.output.lifecycle_active = true;
  snapshot.output.automation_requested = true;
  snapshot.output.safety_reason_code =
      static_cast<std::uint32_t>(stage28d::LampSafetyReason::Safe);
  snapshot.output.endpoint_count = 3U;

  auto& lamp = snapshot.output.endpoints[0];
  lamp.endpoint = stage28d::kScheduledLightEndpoint;
  lamp.selected = true;
  lamp.selected_level = 1.0F;
  lamp.resolved = true;
  lamp.resolved_state = output::BinaryOutputState::On;
  lamp.physical_state = output::PhysicalOutputState::On;
  lamp.physical_independent = true;

  auto& fan = snapshot.output.endpoints[1];
  fan.endpoint = stage28d::kExhaustFanEndpoint;
  fan.selected = true;
  fan.selected_level = 0.2F;
  fan.resolved = true;
  fan.resolved_state = output::BinaryOutputState::On;
  fan.physical_state = output::PhysicalOutputState::On;

  auto& humidifier = snapshot.output.endpoints[2];
  humidifier.endpoint = stage28d::kHumidifierEndpoint;
  humidifier.selected = true;
  humidifier.selected_level = 0.0F;
  humidifier.resolved = true;
  humidifier.resolved_state = output::BinaryOutputState::Off;
  humidifier.physical_state = output::PhysicalOutputState::Off;
  return snapshot;
}

storage::Stage27StorageStatus nominalStorage() {
  storage::Stage27StorageStatus status{};
  status.active_backend = storage::Stage27StorageBackendKind::Sd;
  status.sd_mounted = true;
  status.records_written = 42U;
  status.last_write_ms = 120'000U;
  return status;
}

display::DisplayEndpointRoles endpointRoles() {
  return {stage28d::kExhaustFanEndpoint, stage28d::kScheduledLightEndpoint,
          stage28d::kHumidifierEndpoint};
}

void testObserverProjectsAuthoritativeTelemetryIntoDisplayRuntime() {
  display::DisplayRuntimeConfig config{};
  config.refresh.minimum_refresh_interval_ms = 15'000U;
  display::DisplayTelemetryObserver observer{endpointRoles(), config};

  auto telemetry_snapshot = nominalTelemetry(123'000U);
  const auto storage_status = nominalStorage();
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(observer.hasSnapshot());
  assert(observer.hasFrame());
  assert(observer.hasPendingRefresh());
  assert(observer.projectionErrorCount() == 0U);
  assert(observer.runtimeErrorCount() == 0U);

  const auto& projected = observer.lastSnapshot();
  assert(projected.temperature_c.valid);
  assert(projected.temperature_c.value == 23.4F);
  assert(projected.exhaust_fan.requested_known);
  assert(projected.exhaust_fan.requested_level == 0.2F);
  assert(projected.exhaust_fan.effective_state == output::BinaryOutputState::On);
  assert(projected.storage.active_backend == storage::Stage27StorageBackendKind::Sd);

  const auto& initial_frame = observer.lastFrame();
  assert(initial_frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(initial_frame.refresh_reason == display::DisplayRefreshReason::Initial);
  assert(initial_frame.page == display::DisplayPage::Status);
  assert(std::strcmp(initial_frame.page_model.title.data(), "Growbox status") == 0);
  assert(initial_frame.render_list.command_count == 21U);

  telemetry_snapshot.uptime_ms = 123'500U;
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(observer.lastFrame().refresh_kind == display::DisplayRefreshKind::Full);
  assert(observer.lastFrame().refresh_reason == display::DisplayRefreshReason::Initial);
  assert(observer.confirmRendered(123'500U));
  assert(!observer.hasPendingRefresh());
  assert(!observer.confirmRendered(123'501U));

  telemetry_snapshot.uptime_ms = 124'000U;
  telemetry_snapshot.scd_temperature_c = 24.0F;
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(!observer.lastFrame().refreshRequired());
  assert(observer.lastSnapshot().temperature_c.value == 24.0F);

  assert(observer.handleButton(display::DisplayButton::Next));
  telemetry_snapshot.uptime_ms = 124'001U;
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(observer.lastFrame().refresh_kind == display::DisplayRefreshKind::Partial);
  assert(observer.lastFrame().refresh_reason == display::DisplayRefreshReason::Navigation);
  assert(observer.lastFrame().page == display::DisplayPage::Outputs);
  assert(std::strcmp(observer.lastFrame().page_model.title.data(), "Outputs") == 0);
  assert(observer.confirmRendered(124'001U));
}

void testObserverCarriesShortFirmwareShaIntoDiagnostics() {
  display::DisplayTelemetryObserver observer{endpointRoles(), "0123456789abcdef"};
  auto telemetry_snapshot = nominalTelemetry(123'000U);
  const auto storage_status = nominalStorage();

  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(std::strcmp(observer.lastSnapshot().firmware_sha.data(), "0123456789") == 0);
  assert(observer.confirmRendered(123'000U));

  assert(observer.handleButton(display::DisplayButton::Next));
  telemetry_snapshot.uptime_ms = 123'001U;
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(observer.confirmRendered(123'001U));

  assert(observer.handleButton(display::DisplayButton::Next));
  telemetry_snapshot.uptime_ms = 123'002U;
  assert(observer.observe(telemetry_snapshot, storage_status));

  const auto& diagnostics = observer.lastFrame().page_model;
  assert(observer.lastFrame().page == display::DisplayPage::Diagnostics);
  assert(std::strcmp(diagnostics.title.data(), "Diagnostics") == 0);
  assert(diagnostics.line_count == 10U);
  const auto* firmware_line = findLine(diagnostics, "FW");
  assert(firmware_line != nullptr);
  assert(std::strcmp(firmware_line->value.data(), "0123456789") == 0);
  assert(observer.confirmRendered(123'002U));
}

void testClayCoordinatorAcknowledgesOnlySuccessfulBackendRender() {
  display::DisplayTelemetryObserver observer{endpointRoles()};
  auto telemetry_snapshot = nominalTelemetry(200'000U);
  const auto storage_status = nominalStorage();
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(observer.hasPendingRefresh());

  display::ClayDisplayTheme theme{};
  FakeClayBackend backend{};
  backend.end_result = false;

  assert(!display::renderPendingDisplayToClay(observer, theme, backend, 200'010U));
  assert(observer.hasPendingRefresh());
  assert(backend.begin_count == 1U);
  assert(backend.end_count == 1U);
  assert(backend.draw_count == observer.lastFrame().render_list.command_count);
  assert(backend.width == observer.geometry().width_px);
  assert(backend.height == observer.geometry().height_px);
  assert(backend.last_refresh_kind == display::DisplayRefreshKind::Full);

  backend.end_result = true;
  assert(display::renderPendingDisplayToClay(observer, theme, backend, 200'020U));
  assert(!observer.hasPendingRefresh());
  assert(backend.begin_count == 2U);
  assert(backend.end_count == 2U);
  assert(backend.last_refresh_kind == display::DisplayRefreshKind::Full);
  assert(!display::renderPendingDisplayToClay(observer, theme, backend, 200'021U));
  assert(backend.begin_count == 2U);

  assert(observer.handleButton(display::DisplayButton::Next));
  telemetry_snapshot.uptime_ms = 200'100U;
  assert(observer.observe(telemetry_snapshot, storage_status));
  assert(display::renderPendingDisplayToClay(observer, theme, backend, 200'100U));
  assert(backend.last_refresh_kind == display::DisplayRefreshKind::Partial);
}

void testObserverFailsClosedOnInvalidEndpointRoles() {
  const display::DisplayEndpointRoles invalid_roles{
      stage28d::kScheduledLightEndpoint, stage28d::kScheduledLightEndpoint,
      stage28d::kHumidifierEndpoint};
  display::DisplayTelemetryObserver observer{invalid_roles};

  const auto telemetry_snapshot = nominalTelemetry(123'000U);
  const auto storage_status = nominalStorage();
  assert(!observer.observe(telemetry_snapshot, storage_status));
  assert(!observer.hasSnapshot());
  assert(!observer.hasFrame());
  assert(!observer.hasPendingRefresh());
  assert(observer.projectionErrorCount() == 1U);
  assert(observer.runtimeErrorCount() == 0U);
}

} // namespace

int main() {
  testObserverProjectsAuthoritativeTelemetryIntoDisplayRuntime();
  testObserverCarriesShortFirmwareShaIntoDiagnostics();
  testClayCoordinatorAcknowledgesOnlySuccessfulBackendRender();
  testObserverFailsClosedOnInvalidEndpointRoles();
  return 0;
}
