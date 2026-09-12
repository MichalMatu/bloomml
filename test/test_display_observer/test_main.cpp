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
  assert(observer.projectionErrorCount() == 1U);
  assert(observer.runtimeErrorCount() == 0U);
}

} // namespace

int main() {
  testObserverProjectsAuthoritativeTelemetryIntoDisplayRuntime();
  testObserverFailsClosedOnInvalidEndpointRoles();
  return 0;
}
