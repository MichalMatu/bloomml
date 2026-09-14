#include "climate/display/DisplaySnapshot.h"
#include "climate/output/OutputExecutionTelemetry.h"

#include <array>
#include <cassert>

namespace display = growbox::app::climate_io::display;
namespace output = growbox::app::output;
namespace storage = growbox::app::climate_io::storage;
namespace telemetry = growbox::app::climate_io::telemetry;

namespace {
constexpr output::OutputEndpointId kFan = 1U;
constexpr output::OutputEndpointId kLamp = 2U;
constexpr output::OutputEndpointId kHumidifier = 3U;

output::OutputStateStore makeStore() {
  output::OutputStateStore store;
  const std::array<output::OutputEndpointId, output::kOutputEndpointCapacity> endpoints{
      kFan, kLamp, kHumidifier};
  assert(store.configure(endpoints, endpoints.size()));
  return store;
}

void testSnapshotKeepsIntentResolutionTransportAndPhysicalTruthSeparate() {
  auto store = makeStore();

  output::OutputCommand fan_attempt{};
  fan_attempt.endpoint = kFan;
  fan_attempt.state = output::BinaryOutputState::On;
  fan_attempt.source = output::OutputSource::Safety;
  fan_attempt.reason = output::OutputReason::ThermalSafety;
  assert(store.recordAttempt(fan_attempt, 500U,
                             {output::TransportStatus::Completed, output::TransportError::None}));
  assert(store.recordPhysicalObservation(kFan, output::PhysicalOutputState::Off, 490U, 7U));

  output::OutputSupervisorCycleInput cycle{};
  cycle.mode = output::SupervisorMode::Automatic;
  cycle.monotonic_ms = 500U;
  assert(output::setEndpointIntent(cycle.control.endpoints[0], kFan, 0.2F));
  assert(output::setEndpointIntent(cycle.schedule.endpoints[0], kLamp, 1.0F));
  assert(output::setEndpointIntent(cycle.manual.endpoints[0], kHumidifier, 1.0F));
  assert(output::setSafetyConstraint(cycle.safety.endpoints[0], kFan,
                                     output::SafetyConstraint::ForceOn,
                                     output::OutputReason::ThermalSafety));

  output::OutputSupervisorResolution resolution{};
  resolution.endpoint_count = 3U;
  resolution.endpoints[0].endpoint = kFan;
  resolution.endpoints[0].has_selected_input = true;
  resolution.endpoints[0].requested_level = 1.0F;
  resolution.endpoints[0].source = output::OutputSource::Safety;
  resolution.endpoints[0].reason = output::OutputReason::ThermalSafety;
  resolution.endpoints[0].has_resolved_state = true;
  resolution.endpoints[0].resolved_state = output::BinaryOutputState::On;
  resolution.endpoints[0].safety_override = true;
  resolution.endpoints[1].endpoint = kLamp;
  resolution.endpoints[1].has_selected_input = true;
  resolution.endpoints[1].requested_level = 1.0F;
  resolution.endpoints[1].source = output::OutputSource::Schedule;
  resolution.endpoints[1].reason = output::OutputReason::ScheduleRequest;
  resolution.endpoints[1].has_resolved_state = true;
  resolution.endpoints[1].resolved_state = output::BinaryOutputState::On;
  resolution.endpoints[2].endpoint = kHumidifier;
  resolution.endpoints[2].has_selected_input = true;
  resolution.endpoints[2].requested_level = 1.0F;
  resolution.endpoints[2].source = output::OutputSource::Manual;
  resolution.endpoints[2].reason = output::OutputReason::ManualRequest;
  resolution.endpoints[2].has_resolved_state = true;
  resolution.endpoints[2].resolved_state = output::BinaryOutputState::On;
  resolution.endpoints[2].held_by_dwell = true;

  output::OutputExecutionTelemetrySnapshot snapshot{};
  assert(output::buildOutputExecutionTelemetry(cycle, resolution, store, true, true,
                                               output::OutputLifecycleEvent::Recovery, true,
                                               snapshot));
  assert(snapshot.version == 2U);
  assert(snapshot.mode == output::SupervisorMode::Automatic);
  assert(snapshot.transport_active);
  assert(snapshot.lifecycle_active);
  assert(snapshot.lifecycle_event == output::OutputLifecycleEvent::Recovery);
  assert(snapshot.automation_requested);
  assert(snapshot.endpoint_count == 3U);

  const auto& fan = snapshot.endpoints[0];
  assert(fan.control.active && fan.control.level == 0.2F);
  assert(fan.safety_active);
  assert(fan.safety_constraint == output::SafetyConstraint::ForceOn);
  assert(fan.selected_source == output::OutputSource::Safety);
  assert(fan.resolved && fan.resolved_state == output::BinaryOutputState::On);
  assert(fan.attempt_known && fan.attempted_this_cycle);
  assert(fan.transport_status == output::TransportStatus::Completed);
  assert(fan.last_command_known && fan.last_command_state == output::BinaryOutputState::On);
  assert(fan.physical_state == output::PhysicalOutputState::Off);
  assert(fan.physical_independent);

  const auto& lamp = snapshot.endpoints[1];
  assert(lamp.schedule.active && lamp.schedule.level == 1.0F);
  assert(!lamp.attempt_known);
  assert(!lamp.last_command_known);
  assert(lamp.physical_state == output::PhysicalOutputState::Unknown);
  assert(!lamp.physical_independent);

  const auto& humidifier = snapshot.endpoints[2];
  assert(humidifier.manual.active && humidifier.manual.level == 1.0F);
  assert(humidifier.held_by_dwell);
}

void testFailedHistoricalAttemptIsNotCurrentAndDoesNotInventCommandOrPhysicalTruth() {
  auto store = makeStore();
  output::OutputCommand command{};
  command.endpoint = kLamp;
  command.state = output::BinaryOutputState::Off;
  command.source = output::OutputSource::Lifecycle;
  command.reason = output::OutputReason::LifecyclePolicy;
  assert(store.recordAttempt(command, 100U,
                             {output::TransportStatus::Failed, output::TransportError::IoFailure}));

  output::OutputSupervisorCycleInput cycle{};
  cycle.mode = output::SupervisorMode::FaultLocked;
  cycle.monotonic_ms = 200U;
  output::OutputSupervisorResolution resolution{};
  resolution.endpoint_count = 1U;
  resolution.endpoints[0].endpoint = kLamp;

  output::OutputExecutionTelemetrySnapshot snapshot{};
  assert(output::buildOutputExecutionTelemetry(cycle, resolution, store, false, false,
                                               output::OutputLifecycleEvent::Fault, false,
                                               snapshot));
  const auto& lamp = snapshot.endpoints[0];
  assert(lamp.attempt_known);
  assert(!lamp.attempted_this_cycle);
  assert(lamp.transport_status == output::TransportStatus::Failed);
  assert(!lamp.last_command_known);
  assert(lamp.physical_state == output::PhysicalOutputState::Unknown);
  assert(!lamp.physical_independent);
}

void testDisplaySnapshotProjectsReadOnlyRuntimeTruthByRole() {
  telemetry::Stage27TelemetrySnapshot source{};
  source.uptime_ms = 44'000U;
  source.input_sampled = true;
  source.scd_available = true;
  source.scd_sample = true;
  source.scd_temperature_c = 23.5F;
  source.scd_humidity_pct = 61.0F;
  source.scd_co2_ppm = 812.0F;
  source.scd_age_ms = 1'250U;
  source.rtc_available = true;
  source.rtc_trusted = true;
  source.unix_time_s = 1'800'000'000U;
  source.ble_scanning = true;

  source.output.mode = output::SupervisorMode::Automatic;
  source.output.transport_active = true;
  source.output.lifecycle_active = false;
  source.output.automation_requested = true;
  source.output.safety_latched = true;
  source.output.safety_reason_code = 3U;
  source.output.endpoint_count = 3U;

  auto& humidifier = source.output.endpoints[0];
  humidifier.endpoint = kHumidifier;
  humidifier.selected = true;
  humidifier.selected_level = 0.8F;
  humidifier.selected_source = output::OutputSource::Climate;
  humidifier.selected_reason = output::OutputReason::ClimateDecision;
  humidifier.resolved = true;
  humidifier.resolved_state = output::BinaryOutputState::On;
  humidifier.held_by_dwell = true;

  auto& fan = source.output.endpoints[1];
  fan.endpoint = kFan;
  fan.selected = true;
  fan.selected_level = 1.0F;
  fan.selected_source = output::OutputSource::Safety;
  fan.selected_reason = output::OutputReason::ThermalSafety;
  fan.resolved = true;
  fan.resolved_state = output::BinaryOutputState::On;
  fan.safety_override = true;
  fan.safety_active = true;
  fan.safety_constraint = output::SafetyConstraint::ForceOn;
  fan.safety_reason = output::OutputReason::ThermalSafety;
  fan.physical_state = output::PhysicalOutputState::Off;
  fan.physical_independent = true;

  auto& lamp = source.output.endpoints[2];
  lamp.endpoint = kLamp;
  lamp.selected = true;
  lamp.selected_level = 1.0F;
  lamp.selected_source = output::OutputSource::Schedule;
  lamp.selected_reason = output::OutputReason::ScheduleRequest;
  lamp.resolved = true;
  lamp.resolved_state = output::BinaryOutputState::Off;
  lamp.safety_override = true;
  lamp.safety_active = true;
  lamp.safety_constraint = output::SafetyConstraint::ForceOff;
  lamp.safety_reason = output::OutputReason::ThermalSafety;

  storage::Stage27StorageStatus storage_status{};
  storage_status.active_backend = storage::Stage27StorageBackendKind::Sd;
  storage_status.sd_mounted = true;
  storage_status.write_errors = 2U;
  storage_status.queue_drops = 1U;
  storage_status.records_written = 42U;
  storage_status.last_write_ms = 43'000U;

  display::DisplaySnapshot projected{};
  const display::DisplayEndpointRoles roles{kFan, kLamp, kHumidifier};
  assert(display::buildDisplaySnapshot(source, storage_status, roles, projected));

  assert(projected.uptime_ms == source.uptime_ms);
  assert(projected.climate_sampled);
  assert(projected.scd_available);
  assert(projected.temperature_c.valid && projected.temperature_c.value == 23.5F);
  assert(projected.relative_humidity_pct.valid && projected.relative_humidity_pct.value == 61.0F);
  assert(projected.co2_ppm.valid && projected.co2_ppm.value == 812.0F);
  assert(projected.co2_ppm.age_ms == 1'250U);
  assert(projected.rtc_available && projected.rtc_trusted);
  assert(projected.unix_time_s == source.unix_time_s);
  assert(projected.ble_scanning);
  assert(projected.lifecycle_mode == output::SupervisorMode::Automatic);
  assert(projected.automation_requested);
  assert(projected.transport_active);
  assert(projected.safety_latched && projected.safety_reason_code == 3U);

  assert(projected.lamp.requested_known && projected.lamp.requested_level == 1.0F);
  assert(projected.lamp.effective_known);
  assert(projected.lamp.effective_state == output::BinaryOutputState::Off);
  assert(projected.lamp.safety_override);
  assert(projected.lamp.safety_constraint == output::SafetyConstraint::ForceOff);

  assert(projected.exhaust_fan.requested_source == output::OutputSource::Safety);
  assert(projected.exhaust_fan.effective_state == output::BinaryOutputState::On);
  assert(projected.exhaust_fan.physical_state == output::PhysicalOutputState::Off);
  assert(projected.exhaust_fan.physical_independent);

  assert(projected.humidifier.requested_level == 0.8F);
  assert(projected.humidifier.held_by_dwell);

  assert(projected.storage.active_backend == storage::Stage27StorageBackendKind::Sd);
  assert(projected.storage.sd_mounted);
  assert(projected.storage.write_errors == 2U);
  assert(projected.storage.queue_drops == 1U);
  assert(projected.storage.records_written == 42U);
  assert(projected.storage.last_write_ms == 43'000U);

  display::DisplaySnapshot invalid{};
  assert(
      !display::buildDisplaySnapshot(source, storage_status, {kFan, kFan, kHumidifier}, invalid));
  assert(invalid.uptime_ms == 0U);
}

} // namespace

int main() {
  testSnapshotKeepsIntentResolutionTransportAndPhysicalTruthSeparate();
  testFailedHistoricalAttemptIsNotCurrentAndDoesNotInventCommandOrPhysicalTruth();
  testDisplaySnapshotProjectsReadOnlyRuntimeTruthByRole();
  return 0;
}
