#pragma once

#include "climate/output/OutputExecutionTelemetry.h"
#include "climate/storage/Stage27StorageTypes.h"
#include "climate/telemetry/Stage27Telemetry.h"

#include <array>
#include <cstdint>

namespace growbox::app::climate_io::display {

struct DisplaySensorValue {
  bool valid{false};
  float value{0.0F};
  std::uint64_t age_ms{0U};
};

struct DisplayActuatorState {
  bool requested_known{false};
  ::growbox::app::output::NormalizedOutputLevel requested_level{0.0F};
  ::growbox::app::output::OutputSource requested_source{
      ::growbox::app::output::OutputSource::None};
  ::growbox::app::output::OutputReason requested_reason{
      ::growbox::app::output::OutputReason::None};

  bool effective_known{false};
  ::growbox::app::output::BinaryOutputState effective_state{
      ::growbox::app::output::BinaryOutputState::Off};
  bool held_by_dwell{false};
  bool safety_override{false};
  bool inhibited{false};

  bool safety_active{false};
  ::growbox::app::output::SafetyConstraint safety_constraint{
      ::growbox::app::output::SafetyConstraint::Allow};
  ::growbox::app::output::OutputReason safety_reason{
      ::growbox::app::output::OutputReason::None};

  ::growbox::app::output::PhysicalOutputState physical_state{
      ::growbox::app::output::PhysicalOutputState::Unknown};
  bool physical_independent{false};
};

struct DisplayStorageState {
  ::growbox::app::climate_io::storage::Stage27StorageBackendKind active_backend{
      ::growbox::app::climate_io::storage::Stage27StorageBackendKind::None};
  bool sd_mounted{false};
  bool flash_mounted{false};
  std::uint32_t sd_mount_errors{0U};
  std::uint32_t flash_mount_errors{0U};
  std::uint32_t write_errors{0U};
  std::uint32_t queue_drops{0U};
  std::uint32_t records_written{0U};
  std::uint64_t last_write_ms{0U};
};

struct DisplayEndpointRoles {
  ::growbox::app::output::OutputEndpointId exhaust_fan{
      ::growbox::app::output::kInvalidOutputEndpoint};
  ::growbox::app::output::OutputEndpointId lamp{
      ::growbox::app::output::kInvalidOutputEndpoint};
  ::growbox::app::output::OutputEndpointId humidifier{
      ::growbox::app::output::kInvalidOutputEndpoint};
};

struct DisplaySnapshot {
  static constexpr std::size_t kFirmwareShaChars = 10U;

  std::uint64_t uptime_ms{0U};
  std::array<char, kFirmwareShaChars + 1U> firmware_sha{};

  bool climate_sampled{false};
  bool scd_available{false};
  DisplaySensorValue temperature_c{};
  DisplaySensorValue relative_humidity_pct{};
  DisplaySensorValue co2_ppm{};

  bool rtc_available{false};
  bool rtc_trusted{false};
  std::uint64_t unix_time_s{0U};

  bool ble_scanning{false};

  ::growbox::app::output::SupervisorMode lifecycle_mode{
      ::growbox::app::output::SupervisorMode::BootLocked};
  bool automation_requested{false};
  bool transport_active{false};
  bool lifecycle_active{false};
  bool safety_latched{false};
  std::uint32_t safety_reason_code{0U};

  DisplayActuatorState lamp{};
  DisplayActuatorState exhaust_fan{};
  DisplayActuatorState humidifier{};

  DisplayStorageState storage{};
};

namespace detail {

inline bool endpointRolesValid(const DisplayEndpointRoles& roles) noexcept {
  using ::growbox::app::output::isValidOutputEndpoint;
  return isValidOutputEndpoint(roles.exhaust_fan) && isValidOutputEndpoint(roles.lamp) &&
         isValidOutputEndpoint(roles.humidifier) && roles.exhaust_fan != roles.lamp &&
         roles.exhaust_fan != roles.humidifier && roles.lamp != roles.humidifier;
}

inline const ::growbox::app::output::OutputEndpointExecutionTelemetry*
findEndpoint(const ::growbox::app::output::OutputExecutionTelemetrySnapshot& snapshot,
             ::growbox::app::output::OutputEndpointId endpoint) noexcept {
  for (std::uint8_t index = 0U; index < snapshot.endpoint_count; ++index) {
    const auto& candidate = snapshot.endpoints[index];
    if (candidate.endpoint == endpoint) {
      return &candidate;
    }
  }
  return nullptr;
}

inline DisplayActuatorState projectActuator(
    const ::growbox::app::output::OutputEndpointExecutionTelemetry* source) noexcept {
  DisplayActuatorState result{};
  if (source == nullptr) {
    return result;
  }

  result.requested_known = source->selected;
  result.requested_level = source->selected_level;
  result.requested_source = source->selected_source;
  result.requested_reason = source->selected_reason;
  result.effective_known = source->resolved;
  result.effective_state = source->resolved_state;
  result.held_by_dwell = source->held_by_dwell;
  result.safety_override = source->safety_override;
  result.inhibited = source->inhibited;
  result.safety_active = source->safety_active;
  result.safety_constraint = source->safety_constraint;
  result.safety_reason = source->safety_reason;
  result.physical_state = source->physical_state;
  result.physical_independent = source->physical_independent;
  return result;
}

} // namespace detail

inline bool buildDisplaySnapshot(
    const ::growbox::app::climate_io::telemetry::Stage27TelemetrySnapshot& telemetry,
    const ::growbox::app::climate_io::storage::Stage27StorageStatus& storage_status,
    const DisplayEndpointRoles& roles, DisplaySnapshot& output) noexcept {
  output = {};

  if (!detail::endpointRolesValid(roles) ||
      telemetry.output.version !=
          ::growbox::app::output::OutputExecutionTelemetrySnapshot::kVersion ||
      telemetry.output.endpoint_count > telemetry.output.endpoints.size()) {
    return false;
  }

  output.uptime_ms = telemetry.uptime_ms;
  output.climate_sampled = telemetry.input_sampled;
  output.scd_available = telemetry.scd_available;
  output.temperature_c = {telemetry.scd_sample, telemetry.scd_temperature_c, telemetry.scd_age_ms};
  output.relative_humidity_pct = {telemetry.scd_sample, telemetry.scd_humidity_pct,
                                  telemetry.scd_age_ms};
  output.co2_ppm = {telemetry.scd_sample, telemetry.scd_co2_ppm, telemetry.scd_age_ms};

  output.rtc_available = telemetry.rtc_available;
  output.rtc_trusted = telemetry.rtc_trusted;
  output.unix_time_s = telemetry.unix_time_s;
  output.ble_scanning = telemetry.ble_scanning;

  output.lifecycle_mode = telemetry.output.mode;
  output.automation_requested = telemetry.output.automation_requested;
  output.transport_active = telemetry.output.transport_active;
  output.lifecycle_active = telemetry.output.lifecycle_active;
  output.safety_latched = telemetry.output.safety_latched;
  output.safety_reason_code = telemetry.output.safety_reason_code;

  output.lamp = detail::projectActuator(detail::findEndpoint(telemetry.output, roles.lamp));
  output.exhaust_fan =
      detail::projectActuator(detail::findEndpoint(telemetry.output, roles.exhaust_fan));
  output.humidifier =
      detail::projectActuator(detail::findEndpoint(telemetry.output, roles.humidifier));

  output.storage.active_backend = storage_status.active_backend;
  output.storage.sd_mounted = storage_status.sd_mounted;
  output.storage.flash_mounted = storage_status.flash_mounted;
  output.storage.sd_mount_errors = storage_status.sd_mount_errors;
  output.storage.flash_mount_errors = storage_status.flash_mount_errors;
  output.storage.write_errors = storage_status.write_errors;
  output.storage.queue_drops = storage_status.queue_drops;
  output.storage.records_written = storage_status.records_written;
  output.storage.last_write_ms = storage_status.last_write_ms;
  return true;
}

} // namespace growbox::app::climate_io::display
