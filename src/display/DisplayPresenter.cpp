#include "display/DisplayPresenter.h"

#include "climate/output/LampSafety.h"
#include "climate/output/OutputBindings.h"
#include "climate/runtime/schedule/EuropeWarsawTime.h"

#include <cstdio>
#include <cstring>

namespace growbox::app::display {
namespace {

template <std::size_t N> void setText(std::array<char, N>& target, const char* text) noexcept {
  std::snprintf(target.data(), target.size(), "%s", text != nullptr ? text : "");
}

bool appendLine(DisplayPageModel& page, const char* label, const char* value) noexcept {
  if (page.line_count >= page.lines.size()) {
    return false;
  }
  DisplayLine& line = page.lines[page.line_count++];
  setText(line.label, label);
  setText(line.value, value);
  return true;
}

const char* modeName(output::SupervisorMode mode) noexcept {
  switch (mode) {
  case output::SupervisorMode::BootLocked:
    return "BOOT LOCK";
  case output::SupervisorMode::Arming:
    return "ARMING";
  case output::SupervisorMode::Automatic:
    return "AUTO";
  case output::SupervisorMode::Recovering:
    return "RECOVERY";
  case output::SupervisorMode::Disabled:
    return "DISABLED";
  case output::SupervisorMode::FaultLocked:
    return "FAULT LOCK";
  case output::SupervisorMode::MaintenanceLocked:
    return "MAINT";
  }
  return "UNKNOWN";
}

const char* safetyName(std::uint32_t code) noexcept {
  using climate_io::stage28d::LampSafetyReason;
  switch (static_cast<LampSafetyReason>(code)) {
  case LampSafetyReason::Safe:
    return "OK";
  case LampSafetyReason::TimerOff:
    return "TIMER OFF";
  case LampSafetyReason::TemperatureUnavailable:
    return "TEMP MISSING";
  case LampSafetyReason::OverTemperature:
    return "OVER TEMP";
  case LampSafetyReason::RecoveryHold:
    return "RECOVERY";
  case LampSafetyReason::InvalidConfig:
    return "CONFIG";
  }
  return "UNKNOWN";
}

const output::OutputEndpointExecutionTelemetry*
findEndpoint(const DisplaySnapshot& snapshot, output::OutputEndpointId endpoint) noexcept {
  if (!snapshot.outputs_available) {
    return nullptr;
  }
  for (std::size_t index = 0U; index < snapshot.outputs.endpoint_count; ++index) {
    const auto& candidate = snapshot.outputs.endpoints[index];
    if (candidate.endpoint == endpoint) {
      return &candidate;
    }
  }
  return nullptr;
}

void formatOutputState(const output::OutputEndpointExecutionTelemetry* endpoint, char* buffer,
                       std::size_t buffer_size) noexcept {
  if (endpoint == nullptr) {
    std::snprintf(buffer, buffer_size, "--");
    return;
  }

  const bool requested = endpoint->selected && endpoint->selected_level >= 0.5F;
  if (!endpoint->resolved) {
    std::snprintf(buffer, buffer_size, "%s -> --", requested ? "ON" : "OFF");
    return;
  }

  const bool effective = endpoint->resolved_state == output::BinaryOutputState::On;
  std::snprintf(buffer, buffer_size, "%s -> %s%s", requested ? "ON" : "OFF",
                effective ? "ON" : "OFF", endpoint->safety_override ? " !" : "");
}

void formatMeasurement(const ::growbox::climate::MeasuredValue& measurement, const char* suffix,
                       char* buffer, std::size_t buffer_size) noexcept {
  if (!measurement.valid) {
    std::snprintf(buffer, buffer_size, "--");
    return;
  }
  std::snprintf(buffer, buffer_size, "%.1f %s", static_cast<double>(measurement.value), suffix);
}

void formatClock(const DisplaySnapshot& snapshot, char* buffer, std::size_t buffer_size,
                 bool with_date) noexcept {
  if (!snapshot.climate_available || !snapshot.clock.valid) {
    std::snprintf(buffer, buffer_size, "INVALID");
    return;
  }
  climate_io::runtime::EuropeWarsawLocalTime local{};
  if (!climate_io::runtime::resolveEuropeWarsawLocalTime(snapshot.clock.unix_time_s, local)) {
    std::snprintf(buffer, buffer_size, "INVALID");
    return;
  }
  if (with_date) {
    std::snprintf(buffer, buffer_size, "%04u-%02u-%02u %02u:%02u",
                  static_cast<unsigned>(local.year), static_cast<unsigned>(local.month),
                  static_cast<unsigned>(local.day), static_cast<unsigned>(local.hour),
                  static_cast<unsigned>(local.minute));
  } else {
    std::snprintf(buffer, buffer_size, "%02u:%02u", static_cast<unsigned>(local.hour),
                  static_cast<unsigned>(local.minute));
  }
}

bool sensorWarning(const DisplaySnapshot& snapshot) noexcept {
  if (!snapshot.climate_available) {
    return true;
  }
  const auto& measurements = snapshot.climate.measurements;
  const std::uint64_t timeout = snapshot.climate.sensor_timeout_ms;
  return !measurements.air_temperature_c.valid || measurements.air_temperature_c.age_ms > timeout ||
         !measurements.relative_humidity_pct.valid ||
         measurements.relative_humidity_pct.age_ms > timeout || !measurements.co2_ppm.valid ||
         measurements.co2_ppm.age_ms > timeout;
}

void buildStatusPage(const DisplaySnapshot& snapshot, DisplayPageModel& page) noexcept {
  setText(page.title, "Growbox status");
  char value[28]{};

  if (snapshot.climate_available) {
    formatMeasurement(snapshot.climate.measurements.air_temperature_c, "C", value, sizeof(value));
  } else {
    std::snprintf(value, sizeof(value), "--");
  }
  (void)appendLine(page, "Temp", value);

  if (snapshot.climate_available) {
    formatMeasurement(snapshot.climate.measurements.relative_humidity_pct, "%", value,
                      sizeof(value));
  } else {
    std::snprintf(value, sizeof(value), "--");
  }
  (void)appendLine(page, "RH", value);

  if (snapshot.climate_available) {
    formatMeasurement(snapshot.climate.measurements.co2_ppm, "ppm", value, sizeof(value));
  } else {
    std::snprintf(value, sizeof(value), "--");
  }
  (void)appendLine(page, "CO2", value);

  if (!snapshot.climate_available || !snapshot.climate.measurements.co2_ppm.valid) {
    std::snprintf(value, sizeof(value), "NO DATA");
  } else {
    const std::uint64_t age_ms = snapshot.climate.measurements.co2_ppm.age_ms;
    std::snprintf(value, sizeof(value), "%s %llus",
                  age_ms <= snapshot.climate.sensor_timeout_ms ? "OK" : "STALE",
                  static_cast<unsigned long long>(age_ms / 1000U));
  }
  (void)appendLine(page, "SCD41", value);

  formatClock(snapshot, value, sizeof(value), false);
  (void)appendLine(page, "Time", value);

  (void)appendLine(page, "Mode",
                   snapshot.outputs_available ? modeName(snapshot.outputs.mode) : "--");

  formatOutputState(findEndpoint(snapshot, climate_io::stage28d::kScheduledLightEndpoint), value,
                    sizeof(value));
  (void)appendLine(page, "Lamp", value);
  formatOutputState(findEndpoint(snapshot, climate_io::stage28d::kExhaustFanEndpoint), value,
                    sizeof(value));
  (void)appendLine(page, "Fan", value);
  formatOutputState(findEndpoint(snapshot, climate_io::stage28d::kHumidifierEndpoint), value,
                    sizeof(value));
  (void)appendLine(page, "Humid", value);

  if (!snapshot.outputs_available) {
    std::snprintf(value, sizeof(value), "--");
  } else if (snapshot.outputs.safety_latched) {
    std::snprintf(value, sizeof(value), "LATCH %s", safetyName(snapshot.outputs.safety_reason_code));
  } else {
    std::snprintf(value, sizeof(value), "%s", safetyName(snapshot.outputs.safety_reason_code));
  }
  (void)appendLine(page, "Safety", value);
}

void buildOutputsPage(const DisplaySnapshot& snapshot, DisplayPageModel& page) noexcept {
  setText(page.title, "Outputs");
  char value[28]{};
  (void)appendLine(page, "Mode",
                   snapshot.outputs_available ? modeName(snapshot.outputs.mode) : "--");
  (void)appendLine(page, "Automation",
                   snapshot.outputs_available
                       ? (snapshot.outputs.automation_requested ? "REQUESTED" : "OFF")
                       : "--");
  (void)appendLine(page, "Transport",
                   snapshot.outputs_available
                       ? (snapshot.outputs.transport_active ? "ACTIVE" : "LOCKED")
                       : "--");

  formatOutputState(findEndpoint(snapshot, climate_io::stage28d::kScheduledLightEndpoint), value,
                    sizeof(value));
  (void)appendLine(page, "Lamp", value);
  formatOutputState(findEndpoint(snapshot, climate_io::stage28d::kExhaustFanEndpoint), value,
                    sizeof(value));
  (void)appendLine(page, "Fan", value);
  formatOutputState(findEndpoint(snapshot, climate_io::stage28d::kHumidifierEndpoint), value,
                    sizeof(value));
  (void)appendLine(page, "Humid", value);
  (void)appendLine(page, "Safety",
                   snapshot.outputs_available ? safetyName(snapshot.outputs.safety_reason_code)
                                              : "--");
}

void buildDiagnosticsPage(const DisplaySnapshot& snapshot, DisplayPageModel& page) noexcept {
  setText(page.title, "Diagnostics");
  char value[28]{};
  formatClock(snapshot, value, sizeof(value), true);
  (void)appendLine(page, "RTC", value);

  if (!snapshot.storage_enabled) {
    (void)appendLine(page, "Storage", "DISABLED");
  } else {
    (void)appendLine(page, "Storage", snapshot.storage_ready ? "OK" : "FAULT");
  }

  std::snprintf(value, sizeof(value), "%s", snapshot.firmware_sha != nullptr ? snapshot.firmware_sha
                                                                            : "unknown");
  if (std::strlen(value) > 12U) {
    value[12] = '\0';
  }
  (void)appendLine(page, "Firmware", value);

  std::snprintf(value, sizeof(value), "%llus",
                static_cast<unsigned long long>(snapshot.captured_monotonic_ms / 1000U));
  (void)appendLine(page, "Uptime", value);
  (void)appendLine(page, "Climate", snapshot.climate_available ? "OK" : "NO SNAPSHOT");
  (void)appendLine(page, "Outputs", snapshot.outputs_available ? "OK" : "NO SNAPSHOT");
}

} // namespace

bool DisplayNavigation::handle(DisplayButton button) noexcept {
  const DisplayPage before = page_;
  switch (button) {
  case DisplayButton::Home:
    page_ = DisplayPage::Status;
    break;
  case DisplayButton::Back:
    page_ = DisplayPage::Status;
    break;
  case DisplayButton::Previous:
    page_ = page_ == DisplayPage::Status
                ? DisplayPage::Diagnostics
                : static_cast<DisplayPage>(static_cast<std::uint8_t>(page_) - 1U);
    break;
  case DisplayButton::Next:
    page_ = page_ == DisplayPage::Diagnostics
                ? DisplayPage::Status
                : static_cast<DisplayPage>(static_cast<std::uint8_t>(page_) + 1U);
    break;
  case DisplayButton::Ok:
    if (page_ == DisplayPage::Status) {
      page_ = DisplayPage::Outputs;
    }
    break;
  }
  return page_ != before;
}

bool buildDisplayPage(const DisplaySnapshot& snapshot, DisplayPage selected_page,
                      DisplayPageModel& page) noexcept {
  page = {};
  page.warning = sensorWarning(snapshot) || !snapshot.clock.valid ||
                 (snapshot.storage_enabled && !snapshot.storage_ready) ||
                 (snapshot.outputs_available &&
                  (snapshot.outputs.safety_latched ||
                   snapshot.outputs.mode == output::SupervisorMode::FaultLocked));

  switch (selected_page) {
  case DisplayPage::Status:
    buildStatusPage(snapshot, page);
    break;
  case DisplayPage::Outputs:
    buildOutputsPage(snapshot, page);
    break;
  case DisplayPage::Diagnostics:
    buildDiagnosticsPage(snapshot, page);
    break;
  }
  return page.line_count > 0U;
}

} // namespace growbox::app::display
