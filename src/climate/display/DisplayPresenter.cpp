#include "climate/display/DisplayPresenter.h"

#include "climate/output/LampSafety.h"
#include "climate/runtime/schedule/EuropeWarsawTime.h"

#include <cstdio>

namespace growbox::app::climate_io::display {
namespace {

namespace output_ns = ::growbox::app::output;

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

const char* modeName(output_ns::SupervisorMode mode) noexcept {
  switch (mode) {
  case output_ns::SupervisorMode::BootLocked:
    return "BOOT LOCK";
  case output_ns::SupervisorMode::Arming:
    return "ARMING";
  case output_ns::SupervisorMode::Automatic:
    return "AUTO";
  case output_ns::SupervisorMode::Recovering:
    return "RECOVERY";
  case output_ns::SupervisorMode::Disabled:
    return "DISABLED";
  case output_ns::SupervisorMode::FaultLocked:
    return "FAULT LOCK";
  case output_ns::SupervisorMode::MaintenanceLocked:
    return "MAINT";
  }
  return "UNKNOWN";
}

const char* safetyReasonName(std::uint32_t code) noexcept {
  using stage28d::LampSafetyReason;
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
  return nullptr;
}

void formatSafety(const DisplaySnapshot& snapshot, char* buffer, std::size_t buffer_size) noexcept {
  const char* reason = safetyReasonName(snapshot.safety_reason_code);
  if (reason != nullptr) {
    std::snprintf(buffer, buffer_size, snapshot.safety_latched ? "LATCH %s" : "%s", reason);
    return;
  }

  std::snprintf(buffer, buffer_size, snapshot.safety_latched ? "LATCH CODE %lu" : "CODE %lu",
                static_cast<unsigned long>(snapshot.safety_reason_code));
}

void formatMeasurement(const DisplaySensorValue& measurement, const char* suffix, char* buffer,
                       std::size_t buffer_size) noexcept {
  if (!measurement.valid) {
    std::snprintf(buffer, buffer_size, "--");
    return;
  }

  std::snprintf(buffer, buffer_size, "%.1f %s", static_cast<double>(measurement.value), suffix);
}

std::uint64_t maxSensorAge(const DisplaySnapshot& snapshot) noexcept {
  std::uint64_t age = snapshot.temperature_c.age_ms;
  if (snapshot.relative_humidity_pct.age_ms > age) {
    age = snapshot.relative_humidity_pct.age_ms;
  }
  if (snapshot.co2_ppm.age_ms > age) {
    age = snapshot.co2_ppm.age_ms;
  }
  return age;
}

bool sensorWarning(const DisplaySnapshot& snapshot, std::uint64_t stale_after_ms) noexcept {
  if (!snapshot.climate_sampled || !snapshot.scd_available || !snapshot.temperature_c.valid ||
      !snapshot.relative_humidity_pct.valid || !snapshot.co2_ppm.valid) {
    return true;
  }

  return snapshot.temperature_c.age_ms > stale_after_ms ||
         snapshot.relative_humidity_pct.age_ms > stale_after_ms ||
         snapshot.co2_ppm.age_ms > stale_after_ms;
}

void formatSensorFreshness(const DisplaySnapshot& snapshot, std::uint64_t stale_after_ms,
                           char* buffer, std::size_t buffer_size) noexcept {
  if (!snapshot.scd_available) {
    std::snprintf(buffer, buffer_size, "UNAVAILABLE");
    return;
  }
  if (!snapshot.temperature_c.valid || !snapshot.relative_humidity_pct.valid ||
      !snapshot.co2_ppm.valid) {
    std::snprintf(buffer, buffer_size, "NO DATA");
    return;
  }

  const std::uint64_t age_ms = maxSensorAge(snapshot);
  std::snprintf(buffer, buffer_size, "%s %llus", age_ms <= stale_after_ms ? "OK" : "STALE",
                static_cast<unsigned long long>(age_ms / 1000U));
}

void formatClock(const DisplaySnapshot& snapshot, char* buffer, std::size_t buffer_size,
                 bool with_date) noexcept {
  if (!snapshot.rtc_available) {
    std::snprintf(buffer, buffer_size, "MISSING");
    return;
  }
  if (!snapshot.rtc_trusted) {
    std::snprintf(buffer, buffer_size, "UNTRUSTED");
    return;
  }

  runtime::EuropeWarsawLocalTime local{};
  if (!runtime::resolveEuropeWarsawLocalTime(snapshot.unix_time_s, local)) {
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

const char* actuatorMarker(const DisplayActuatorState& actuator) noexcept {
  if (actuator.safety_override) {
    return " !S";
  }
  if (actuator.inhibited) {
    return " X";
  }
  if (actuator.held_by_dwell) {
    return " D";
  }
  return "";
}

void formatActuator(const DisplayActuatorState& actuator, char* buffer,
                    std::size_t buffer_size) noexcept {
  const char* effective = "--";
  if (actuator.effective_known) {
    effective = actuator.effective_state == output_ns::BinaryOutputState::On ? "ON" : "OFF";
  }

  if (!actuator.requested_known) {
    std::snprintf(buffer, buffer_size, "-- -> %s%s", effective, actuatorMarker(actuator));
    return;
  }

  std::snprintf(buffer, buffer_size, "%.0f%% -> %s%s",
                static_cast<double>(actuator.requested_level * 100.0F), effective,
                actuatorMarker(actuator));
}

void formatPhysical(const DisplayActuatorState& actuator, char* buffer,
                    std::size_t buffer_size) noexcept {
  const char* state = "UNKNOWN";
  switch (actuator.physical_state) {
  case output_ns::PhysicalOutputState::Unknown:
    state = "UNKNOWN";
    break;
  case output_ns::PhysicalOutputState::Off:
    state = "OFF";
    break;
  case output_ns::PhysicalOutputState::On:
    state = "ON";
    break;
  }

  std::snprintf(buffer, buffer_size, "%s%s", state,
                actuator.physical_independent ? " FB" : "");
}

bool storageMounted(const DisplayStorageState& storage_state) noexcept {
  using storage::Stage27StorageBackendKind;
  switch (storage_state.active_backend) {
  case Stage27StorageBackendKind::None:
    return true;
  case Stage27StorageBackendKind::Sd:
    return storage_state.sd_mounted;
  case Stage27StorageBackendKind::Flash:
    return storage_state.flash_mounted;
  }
  return false;
}

void formatStorage(const DisplayStorageState& storage_state, char* buffer,
                   std::size_t buffer_size) noexcept {
  using storage::Stage27StorageBackendKind;
  switch (storage_state.active_backend) {
  case Stage27StorageBackendKind::None:
    std::snprintf(buffer, buffer_size, "NONE");
    return;
  case Stage27StorageBackendKind::Sd:
    std::snprintf(buffer, buffer_size, "SD %s", storage_state.sd_mounted ? "OK" : "FAULT");
    return;
  case Stage27StorageBackendKind::Flash:
    std::snprintf(buffer, buffer_size, "FLASH %s",
                  storage_state.flash_mounted ? "OK" : "FAULT");
    return;
  }
  std::snprintf(buffer, buffer_size, "UNKNOWN");
}

void buildStatusPage(const DisplaySnapshot& snapshot, const DisplayPresenterConfig& config,
                     DisplayPageModel& page) noexcept {
  setText(page.title, "Growbox status");
  char value[28]{};

  formatMeasurement(snapshot.temperature_c, "C", value, sizeof(value));
  (void)appendLine(page, "Temp", value);
  formatMeasurement(snapshot.relative_humidity_pct, "%", value, sizeof(value));
  (void)appendLine(page, "RH", value);
  formatMeasurement(snapshot.co2_ppm, "ppm", value, sizeof(value));
  (void)appendLine(page, "CO2", value);

  formatSensorFreshness(snapshot, config.sensor_stale_after_ms, value, sizeof(value));
  (void)appendLine(page, "SCD41", value);
  formatClock(snapshot, value, sizeof(value), false);
  (void)appendLine(page, "Time", value);
  (void)appendLine(page, "Mode", modeName(snapshot.lifecycle_mode));

  formatActuator(snapshot.lamp, value, sizeof(value));
  (void)appendLine(page, "Lamp", value);
  formatActuator(snapshot.exhaust_fan, value, sizeof(value));
  (void)appendLine(page, "Fan", value);
  formatActuator(snapshot.humidifier, value, sizeof(value));
  (void)appendLine(page, "Humid", value);

  formatSafety(snapshot, value, sizeof(value));
  (void)appendLine(page, "Safety", value);
}

void buildOutputsPage(const DisplaySnapshot& snapshot, DisplayPageModel& page) noexcept {
  setText(page.title, "Outputs");
  char value[28]{};

  (void)appendLine(page, "Mode", modeName(snapshot.lifecycle_mode));
  (void)appendLine(page, "Automation", snapshot.automation_requested ? "REQUESTED" : "OFF");
  (void)appendLine(page, "Transport", snapshot.transport_active ? "ACTIVE" : "LOCKED");
  (void)appendLine(page, "Lifecycle", snapshot.lifecycle_active ? "ACTIVE" : "IDLE");

  formatActuator(snapshot.lamp, value, sizeof(value));
  (void)appendLine(page, "Lamp", value);
  formatPhysical(snapshot.lamp, value, sizeof(value));
  (void)appendLine(page, "Lamp phys", value);
  formatActuator(snapshot.exhaust_fan, value, sizeof(value));
  (void)appendLine(page, "Fan", value);
  formatPhysical(snapshot.exhaust_fan, value, sizeof(value));
  (void)appendLine(page, "Fan phys", value);
  formatActuator(snapshot.humidifier, value, sizeof(value));
  (void)appendLine(page, "Humid", value);
  formatPhysical(snapshot.humidifier, value, sizeof(value));
  (void)appendLine(page, "Humid phys", value);
}

void buildDiagnosticsPage(const DisplaySnapshot& snapshot, DisplayPageModel& page) noexcept {
  setText(page.title, "Diagnostics");
  char value[28]{};

  formatClock(snapshot, value, sizeof(value), true);
  (void)appendLine(page, "RTC", value);
  formatStorage(snapshot.storage, value, sizeof(value));
  (void)appendLine(page, "Storage", value);

  std::snprintf(value, sizeof(value), "%lu", static_cast<unsigned long>(snapshot.storage.records_written));
  (void)appendLine(page, "Writes", value);
  std::snprintf(value, sizeof(value), "%lu", static_cast<unsigned long>(snapshot.storage.write_errors));
  (void)appendLine(page, "Write err", value);
  std::snprintf(value, sizeof(value), "%lu", static_cast<unsigned long>(snapshot.storage.queue_drops));
  (void)appendLine(page, "Drops", value);

  if (snapshot.storage.last_write_ms == 0U || snapshot.storage.last_write_ms > snapshot.uptime_ms) {
    std::snprintf(value, sizeof(value), "--");
  } else {
    std::snprintf(value, sizeof(value), "%llus ago",
                  static_cast<unsigned long long>((snapshot.uptime_ms - snapshot.storage.last_write_ms) /
                                                  1000U));
  }
  (void)appendLine(page, "Last write", value);
  (void)appendLine(page, "BLE", snapshot.ble_scanning ? "SCANNING" : "IDLE");
  (void)appendLine(page, "SCD41", snapshot.scd_available ? "AVAILABLE" : "MISSING");

  std::snprintf(value, sizeof(value), "%llus",
                static_cast<unsigned long long>(snapshot.uptime_ms / 1000U));
  (void)appendLine(page, "Uptime", value);

  formatSafety(snapshot, value, sizeof(value));
  (void)appendLine(page, "Safety", value);
}

} // namespace

bool DisplayNavigation::handle(DisplayButton button) noexcept {
  const DisplayPage before = page_;
  switch (button) {
  case DisplayButton::Home:
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
                      const DisplayPresenterConfig& config, DisplayPageModel& page) noexcept {
  page = {};
  page.warning = sensorWarning(snapshot, config.sensor_stale_after_ms) || !snapshot.rtc_available ||
                 !snapshot.rtc_trusted || !storageMounted(snapshot.storage) ||
                 snapshot.safety_latched ||
                 snapshot.lifecycle_mode == output_ns::SupervisorMode::FaultLocked;

  switch (selected_page) {
  case DisplayPage::Status:
    buildStatusPage(snapshot, config, page);
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

} // namespace growbox::app::climate_io::display
