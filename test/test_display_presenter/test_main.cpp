#include "climate/output/LampSafety.h"
#include "climate/output/OutputBindings.h"
#include "display/DisplayPresenter.h"

#include <cassert>
#include <cstring>

using namespace growbox::app;

namespace {

const display::DisplayLine* findLine(const display::DisplayPageModel& page, const char* label) {
  for (std::size_t index = 0U; index < page.line_count; ++index) {
    if (std::strcmp(page.lines[index].label.data(), label) == 0) {
      return &page.lines[index];
    }
  }
  return nullptr;
}

display::DisplaySnapshot nominalSnapshot() {
  display::DisplaySnapshot snapshot{};
  snapshot.captured_monotonic_ms = 123'000U;
  snapshot.climate_available = true;
  snapshot.climate.measurements.air_temperature_c = {23.4F, true, 1'000U};
  snapshot.climate.measurements.relative_humidity_pct = {61.2F, true, 1'000U};
  snapshot.climate.measurements.co2_ppm = {712.0F, true, 2'000U};
  snapshot.climate.sensor_timeout_ms = 30'000U;
  snapshot.clock = {true, 1'767'225'600U};
  snapshot.outputs_available = true;
  snapshot.outputs.mode = output::SupervisorMode::Automatic;
  snapshot.outputs.transport_active = true;
  snapshot.outputs.automation_requested = true;
  snapshot.outputs.safety_reason_code =
      static_cast<std::uint32_t>(climate_io::stage28d::LampSafetyReason::Safe);
  snapshot.outputs.endpoint_count = 3U;

  auto& fan = snapshot.outputs.endpoints[0];
  fan.endpoint = climate_io::stage28d::kExhaustFanEndpoint;
  fan.selected = true;
  fan.selected_level = 1.0F;
  fan.resolved = true;
  fan.resolved_state = output::BinaryOutputState::On;

  auto& lamp = snapshot.outputs.endpoints[1];
  lamp.endpoint = climate_io::stage28d::kScheduledLightEndpoint;
  lamp.selected = true;
  lamp.selected_level = 1.0F;
  lamp.resolved = true;
  lamp.resolved_state = output::BinaryOutputState::On;

  auto& humidifier = snapshot.outputs.endpoints[2];
  humidifier.endpoint = climate_io::stage28d::kHumidifierEndpoint;
  humidifier.selected = true;
  humidifier.selected_level = 0.0F;
  humidifier.resolved = true;
  humidifier.resolved_state = output::BinaryOutputState::Off;

  snapshot.storage_enabled = true;
  snapshot.storage_ready = true;
  snapshot.firmware_sha = "0123456789abcdef";
  return snapshot;
}

void testStatusPageContainsOperationalState() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Status, page));
  assert(std::strcmp(page.title.data(), "Growbox status") == 0);
  assert(page.line_count == 10U);
  assert(!page.warning);

  assert(std::strcmp(findLine(page, "Temp")->value.data(), "23.4 C") == 0);
  assert(std::strcmp(findLine(page, "RH")->value.data(), "61.2 %") == 0);
  assert(std::strcmp(findLine(page, "CO2")->value.data(), "712.0 ppm") == 0);
  assert(std::strcmp(findLine(page, "SCD41")->value.data(), "OK 2s") == 0);
  assert(std::strcmp(findLine(page, "Time")->value.data(), "01:00") == 0);
  assert(std::strcmp(findLine(page, "Mode")->value.data(), "AUTO") == 0);
  assert(std::strcmp(findLine(page, "Lamp")->value.data(), "ON -> ON") == 0);
  assert(std::strcmp(findLine(page, "Fan")->value.data(), "ON -> ON") == 0);
  assert(std::strcmp(findLine(page, "Humid")->value.data(), "OFF -> OFF") == 0);
  assert(std::strcmp(findLine(page, "Safety")->value.data(), "OK") == 0);
}

void testWarningsAreDerivedFromAuthoritativeSnapshot() {
  auto snapshot = nominalSnapshot();
  snapshot.climate.measurements.co2_ppm.age_ms = 45'000U;
  snapshot.storage_ready = false;
  snapshot.outputs.safety_latched = true;
  snapshot.outputs.safety_reason_code =
      static_cast<std::uint32_t>(climate_io::stage28d::LampSafetyReason::OverTemperature);
  snapshot.outputs.endpoints[1].resolved_state = output::BinaryOutputState::Off;
  snapshot.outputs.endpoints[1].safety_override = true;

  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Status, page));
  assert(page.warning);
  assert(std::strcmp(findLine(page, "SCD41")->value.data(), "STALE 45s") == 0);
  assert(std::strcmp(findLine(page, "Lamp")->value.data(), "ON -> OFF !") == 0);
  assert(std::strcmp(findLine(page, "Safety")->value.data(), "LATCH OVER TEMP") == 0);

  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Diagnostics, page));
  assert(std::strcmp(findLine(page, "Storage")->value.data(), "FAULT") == 0);
  assert(std::strcmp(findLine(page, "Firmware")->value.data(), "0123456789ab") == 0);
}

void testNavigationMatchesFivePhysicalKeys() {
  display::DisplayNavigation navigation;
  assert(navigation.page() == display::DisplayPage::Status);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Outputs);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Diagnostics);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Status);
  assert(navigation.handle(display::DisplayButton::Previous));
  assert(navigation.page() == display::DisplayPage::Diagnostics);
  assert(navigation.handle(display::DisplayButton::Home));
  assert(navigation.page() == display::DisplayPage::Status);
  assert(navigation.handle(display::DisplayButton::Ok));
  assert(navigation.page() == display::DisplayPage::Outputs);
  assert(navigation.handle(display::DisplayButton::Back));
  assert(navigation.page() == display::DisplayPage::Status);
  assert(!navigation.handle(display::DisplayButton::Back));
}

} // namespace

int main() {
  testStatusPageContainsOperationalState();
  testWarningsAreDerivedFromAuthoritativeSnapshot();
  testNavigationMatchesFivePhysicalKeys();
  return 0;
}
