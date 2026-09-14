#include "climate/input/sensors/Scd41InsideSource.h"

extern "C" {
#include "growbox_sensirion_i2c_hal.h"
#include "scd4x_i2c.h"
#include "sensirion_i2c_hal.h"
}

#include <driver/i2c_master.h>
#include <esp_log.h>

namespace growbox::app::climate_io::native {
namespace {

constexpr char kScd41Tag[] = "scd41";
constexpr std::uint8_t kScd41Address = 0x62U;
constexpr std::uint32_t kI2cClockHz = 100'000U;
constexpr std::uint64_t kNoMeasurementRecoveryMs = 30'000U;

std::uint64_t ageSince(std::uint64_t now_ms, std::uint64_t then_ms) noexcept {
  return now_ms >= then_ms ? now_ms - then_ms : 0U;
}

bool plausible(float temperature_c, float humidity_pct, float co2_ppm) noexcept {
  return temperature_c >= -45.0F && temperature_c <= 130.0F && humidity_pct >= 0.0F &&
         humidity_pct <= 100.0F && co2_ppm > 0.0F;
}

} // namespace

Scd41InsideSource::~Scd41InsideSource() {
  if (started_) {
    growbox_sensirion_i2c_bind_device(device_);
    static_cast<void>(scd4x_stop_periodic_measurement());
  }
  if (device_ != nullptr) {
    i2c_master_bus_rm_device(device_);
  }
}

bool Scd41InsideSource::begin(NativeI2cBus& bus) noexcept {
  if (started_) {
    return true;
  }
  if (bus.addDevice(kScd41Address, kI2cClockHz, device_) != ESP_OK) {
    available_ = false;
    return false;
  }

  growbox_sensirion_i2c_bind_device(device_);
  sensirion_i2c_hal_init();
  scd4x_init(kScd41Address);

  // Follow Sensirion's clean-start sequence for SCD41. The sensor can retain a
  // measurement or sleep state across MCU-only resets, so wake it first, stop
  // any inherited periodic session, reload EEPROM-backed settings, then start
  // a fresh periodic session. The generated wake_up command intentionally
  // ignores its expected missing ACK and includes the required settling delay.
  const int16_t wake_error = scd4x_wake_up();
  if (wake_error != 0) {
    ESP_LOGI(kScd41Tag, "wake_up returned %d; continuing", static_cast<int>(wake_error));
  }

  const int16_t stop_error = scd4x_stop_periodic_measurement();
  if (stop_error != 0) {
    ESP_LOGI(kScd41Tag, "pre-start stop_periodic_measurement returned %d; continuing",
             static_cast<int>(stop_error));
  }

  const int16_t reinit_error = scd4x_reinit();
  if (reinit_error != 0) {
    ESP_LOGW(kScd41Tag, "pre-start reinit failed: %d; attempting periodic start",
             static_cast<int>(reinit_error));
  }

  const int16_t error = scd4x_start_periodic_measurement();
  if (error != 0) {
    ESP_LOGW(kScd41Tag, "start_periodic_measurement failed: %d", static_cast<int>(error));
  }
  available_ = error == 0;
  started_ = available_;
  recovery_attempted_ = false;
  recovery_window_started_ = false;
  recovery_window_start_ms_ = 0U;
  recovery_attempt_count_ = 0U;
  return available_;
}

bool Scd41InsideSource::recoverPeriodicMeasurement() noexcept {
  growbox_sensirion_i2c_bind_device(device_);
  const int16_t wake_error = scd4x_wake_up();
  if (wake_error != 0) {
    ESP_LOGI(kScd41Tag, "recovery wake_up returned %d; continuing", static_cast<int>(wake_error));
  }
  const int16_t stop_error = scd4x_stop_periodic_measurement();
  if (stop_error != 0) {
    ESP_LOGI(kScd41Tag, "recovery stop_periodic_measurement returned %d; continuing",
             static_cast<int>(stop_error));
  }
  const int16_t reinit_error = scd4x_reinit();
  if (reinit_error != 0) {
    ESP_LOGW(kScd41Tag, "recovery reinit failed: %d; attempting periodic start",
             static_cast<int>(reinit_error));
  }
  const int16_t start_error = scd4x_start_periodic_measurement();
  if (start_error != 0) {
    ESP_LOGW(kScd41Tag, "recovery start_periodic_measurement failed: %d",
             static_cast<int>(start_error));
    return false;
  }
  ESP_LOGW(kScd41Tag, "one-shot periodic measurement recovery started");
  return true;
}

bool Scd41InsideSource::fillCached(std::uint64_t monotonic_ms,
                                   InsideEnvironmentSnapshot& output) const noexcept {
  output = {};
  if (!has_measurement_) {
    return false;
  }
  const std::uint64_t age_ms = ageSince(monotonic_ms, last_measurement_ms_);
  output.air_temperature_c = {temperature_c_, true, age_ms};
  output.relative_humidity_pct = {relative_humidity_pct_, true, age_ms};
  output.co2_ppm = {co2_ppm_, true, age_ms};
  return true;
}

bool Scd41InsideSource::sample(std::uint64_t monotonic_ms,
                               InsideEnvironmentSnapshot& output) noexcept {
  output = {};
  if (!started_ || device_ == nullptr) {
    available_ = false;
    return false;
  }

  if (!recovery_window_started_) {
    recovery_window_started_ = true;
    recovery_window_start_ms_ = monotonic_ms;
  }

  growbox_sensirion_i2c_bind_device(device_);
  bool data_ready = false;
  const int16_t ready_error = scd4x_get_data_ready_status(&data_ready);
  if (ready_error != 0) {
    ++read_error_count_;
    available_ = false;
    return fillCached(monotonic_ms, output);
  }
  available_ = true;
  if (!data_ready) {
    const std::uint64_t progress_ms =
        has_measurement_ ? last_measurement_ms_ : recovery_window_start_ms_;
    if (!recovery_attempted_ && ageSince(monotonic_ms, progress_ms) >= kNoMeasurementRecoveryMs) {
      recovery_attempted_ = true;
      ++recovery_attempt_count_;
      ESP_LOGW(kScd41Tag, "no new measurement for %llums; attempting one-shot recovery",
               static_cast<unsigned long long>(ageSince(monotonic_ms, progress_ms)));
      available_ = recoverPeriodicMeasurement();
      recovery_window_start_ms_ = monotonic_ms;
    }
    return fillCached(monotonic_ms, output);
  }

  uint16_t co2 = 0U;
  int32_t temperature_mdeg_c = 0;
  int32_t humidity_mpercent = 0;
  const int16_t read_error = scd4x_read_measurement(&co2, &temperature_mdeg_c, &humidity_mpercent);
  if (read_error != 0) {
    ++read_error_count_;
    available_ = false;
    return fillCached(monotonic_ms, output);
  }

  const float temperature_c = static_cast<float>(temperature_mdeg_c) / 1000.0F;
  const float humidity_pct = static_cast<float>(humidity_mpercent) / 1000.0F;
  const float co2_ppm = static_cast<float>(co2);
  if (!plausible(temperature_c, humidity_pct, co2_ppm)) {
    ++invalid_measurement_count_;
    return fillCached(monotonic_ms, output);
  }

  temperature_c_ = temperature_c;
  relative_humidity_pct_ = humidity_pct;
  co2_ppm_ = co2_ppm;
  last_measurement_ms_ = monotonic_ms;
  has_measurement_ = true;
  ++successful_measurement_count_;
  return fillCached(monotonic_ms, output);
}

} // namespace growbox::app::climate_io::native
