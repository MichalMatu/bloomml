#pragma once

#include "IpoInputReadingRegistry.h"
#include "climate/input/ble/BleClimateScanner.h"
#include "climate/input/sensors/Scd41InsideSource.h"

namespace growbox::app::climate_io::runtime {

// One bounded frame per control-cycle timestamp. The native driver locators
// identify installed hardware; user-configurable graph IDs and semantic roles
// are independent. No Process/Output implementation is visible here.
class RuntimeInputRegistry {
public:
  RuntimeInputRegistry(const ::growbox::ipo::Graph& graph, native::BleClimateScanner& ble,
                       native::Scd41InsideSource& scd41) noexcept
      : frame_(graph), ble_(ble), scd41_(scd41) {}

  const ::growbox::ipo::InputReadingRegistry& sample(std::uint64_t now_ms) noexcept {
    if (sampled_ && now_ms == sampled_at_) return frame_;
    frame_.clear();
    using ::growbox::ipo::Driver;
    using ::growbox::ipo::Channel;
    native::BleClimateReading reading{};
    if (frame_.hasDevice(Driver::Tp357, 1U) && ble_.sampleTp357(now_ms, reading)) {
      frame_.recordFor(Driver::Tp357, 1U, Channel::Temperature, reading.temperature_c, reading.age_ms, true);
      frame_.recordFor(Driver::Tp357, 1U, Channel::Humidity, reading.relative_humidity_pct, reading.age_ms, true);
    }
    reading = {};
    if (frame_.hasDevice(Driver::Xiaomi, 2U) && ble_.sampleXiaomi(now_ms, reading)) {
      frame_.recordFor(Driver::Xiaomi, 2U, Channel::Temperature, reading.temperature_c, reading.age_ms, true);
      frame_.recordFor(Driver::Xiaomi, 2U, Channel::Humidity, reading.relative_humidity_pct, reading.age_ms, true);
    }
    InsideEnvironmentSnapshot scd{};
    if (frame_.hasDevice(Driver::Scd41, 0x62U) && scd41_.sample(now_ms, scd)) {
      const auto record = [&](Channel channel, const ::growbox::climate::MeasuredValue& value) {
        frame_.recordFor(Driver::Scd41, 0x62U, channel, value.value, value.age_ms, value.valid);
      };
      record(Channel::Temperature, scd.air_temperature_c);
      record(Channel::Humidity, scd.relative_humidity_pct);
      record(Channel::Co2, scd.co2_ppm);
    }
    sampled_ = true;
    sampled_at_ = now_ms;
    return frame_;
  }

private:
  ::growbox::ipo::InputReadingRegistry frame_;
  native::BleClimateScanner& ble_;
  native::Scd41InsideSource& scd41_;
  std::uint64_t sampled_at_{0U};
  bool sampled_{false};
};

} // namespace growbox::app::climate_io::runtime
