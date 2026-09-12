#include "climate/display/DisplayPresenter.h"
#include "climate/display/DisplayRenderList.h"
#include "climate/display/DisplaySurface.h"
#include "climate/display/DisplayTextSimulator.h"
#include "climate/output/LampSafety.h"

#include <cassert>
#include <cstring>

namespace display = growbox::app::climate_io::display;
namespace output = growbox::app::output;
namespace stage28d = growbox::app::climate_io::stage28d;
namespace storage = growbox::app::climate_io::storage;

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
  snapshot.uptime_ms = 123'000U;
  snapshot.climate_sampled = true;
  snapshot.scd_available = true;
  snapshot.temperature_c = {true, 23.4F, 1'000U};
  snapshot.relative_humidity_pct = {true, 61.2F, 1'000U};
  snapshot.co2_ppm = {true, 712.0F, 2'000U};
  snapshot.rtc_available = true;
  snapshot.rtc_trusted = true;
  snapshot.unix_time_s = 1'767'225'600U;
  snapshot.ble_scanning = true;

  snapshot.lifecycle_mode = output::SupervisorMode::Automatic;
  snapshot.transport_active = true;
  snapshot.lifecycle_active = true;
  snapshot.automation_requested = true;
  snapshot.safety_reason_code = static_cast<std::uint32_t>(stage28d::LampSafetyReason::Safe);

  snapshot.lamp.requested_known = true;
  snapshot.lamp.requested_level = 1.0F;
  snapshot.lamp.effective_known = true;
  snapshot.lamp.effective_state = output::BinaryOutputState::On;
  snapshot.lamp.physical_state = output::PhysicalOutputState::On;
  snapshot.lamp.physical_independent = true;

  snapshot.exhaust_fan.requested_known = true;
  snapshot.exhaust_fan.requested_level = 0.2F;
  snapshot.exhaust_fan.effective_known = true;
  snapshot.exhaust_fan.effective_state = output::BinaryOutputState::On;
  snapshot.exhaust_fan.physical_state = output::PhysicalOutputState::On;

  snapshot.humidifier.requested_known = true;
  snapshot.humidifier.requested_level = 0.0F;
  snapshot.humidifier.effective_known = true;
  snapshot.humidifier.effective_state = output::BinaryOutputState::Off;
  snapshot.humidifier.physical_state = output::PhysicalOutputState::Off;

  snapshot.storage.active_backend = storage::Stage27StorageBackendKind::Sd;
  snapshot.storage.sd_mounted = true;
  snapshot.storage.records_written = 42U;
  snapshot.storage.last_write_ms = 120'000U;
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
  assert(std::strcmp(findLine(page, "Lamp")->value.data(), "100% -> ON") == 0);
  assert(std::strcmp(findLine(page, "Fan")->value.data(), "20% -> ON") == 0);
  assert(std::strcmp(findLine(page, "Humid")->value.data(), "0% -> OFF") == 0);
  assert(std::strcmp(findLine(page, "Safety")->value.data(), "OK") == 0);
}

void testWarningsAreDerivedFromProjectedRuntimeTruth() {
  auto snapshot = nominalSnapshot();
  snapshot.co2_ppm.age_ms = 45'000U;
  snapshot.storage.sd_mounted = false;
  snapshot.safety_latched = true;
  snapshot.safety_reason_code =
      static_cast<std::uint32_t>(stage28d::LampSafetyReason::OverTemperature);
  snapshot.lamp.effective_state = output::BinaryOutputState::Off;
  snapshot.lamp.safety_override = true;

  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Status, page));
  assert(page.warning);
  assert(std::strcmp(findLine(page, "SCD41")->value.data(), "STALE 45s") == 0);
  assert(std::strcmp(findLine(page, "Lamp")->value.data(), "100% -> OFF !S") == 0);
  assert(std::strcmp(findLine(page, "Safety")->value.data(), "LATCH OVER TEMP") == 0);

  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Diagnostics, page));
  assert(std::strcmp(findLine(page, "Storage")->value.data(), "SD FAULT") == 0);
}

void testFreshnessPolicyIsExplicitPresenterConfig() {
  auto snapshot = nominalSnapshot();
  snapshot.co2_ppm.age_ms = 45'000U;

  display::DisplayPresenterConfig config{};
  config.sensor_stale_after_ms = 60'000U;
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Status, config, page));
  assert(!page.warning);
  assert(std::strcmp(findLine(page, "SCD41")->value.data(), "OK 45s") == 0);
}

void testOutputsPageKeepsRequestedEffectiveAndPhysicalTruthSeparate() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Outputs, page));
  assert(page.line_count == 10U);
  assert(std::strcmp(findLine(page, "Fan")->value.data(), "20% -> ON") == 0);
  assert(std::strcmp(findLine(page, "Lamp phys")->value.data(), "ON FB") == 0);
  assert(std::strcmp(findLine(page, "Fan phys")->value.data(), "ON") == 0);
  assert(std::strcmp(findLine(page, "Humid phys")->value.data(), "OFF") == 0);
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

void testTextSimulatorUsesTheSameSurfaceSeamAsHardwareAdapters() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Status, page));

  display::DisplayTextSimulator simulator{};
  assert(display::renderDisplayPage(page, simulator));
  assert(simulator.size() > 0U);
  assert(std::strstr(simulator.text(), "Growbox status") != nullptr);
  assert(std::strstr(simulator.text(), "23.4 C") != nullptr);
  assert(std::strstr(simulator.text(), "20% -> ON") != nullptr);
  assert(simulator.text()[0] == ' ');
  assert(simulator.text()[1] == ' ');

  page.warning = true;
  assert(display::renderDisplayPage(page, simulator));
  assert(simulator.text()[0] == '!');
  assert(simulator.text()[1] == ' ');

  page.line_count = page.lines.size() + 1U;
  assert(!display::renderDisplayPage(page, simulator));
}

void testRenderListSurfaceMapsPresenterDataToFixedGeometry() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Status, page));

  display::DisplayRenderGeometry geometry{};
  display::DisplayRenderList render_list{};
  display::DisplayRenderListSurface surface{geometry, render_list};
  assert(display::renderDisplayPage(page, surface));
  assert(render_list.command_count == 21U);
  assert(!render_list.warning);

  const auto& title = render_list.commands[0];
  assert(title.role == display::DisplayTextRole::Title);
  assert(title.x_px == 8U);
  assert(title.y_px == 14U);
  assert(std::strcmp(title.text.data(), "Growbox status") == 0);

  const auto& first_label = render_list.commands[1];
  const auto& first_value = render_list.commands[2];
  assert(first_label.role == display::DisplayTextRole::Label);
  assert(first_label.x_px == 8U);
  assert(first_label.y_px == 28U);
  assert(std::strcmp(first_label.text.data(), "Temp") == 0);
  assert(first_value.role == display::DisplayTextRole::Value);
  assert(first_value.x_px == 92U);
  assert(first_value.y_px == 28U);
  assert(std::strcmp(first_value.text.data(), "23.4 C") == 0);

  page.warning = true;
  assert(display::renderDisplayPage(page, surface));
  assert(render_list.command_count == 22U);
  assert(render_list.warning);
  assert(render_list.commands[0].role == display::DisplayTextRole::WarningMarker);
  assert(std::strcmp(render_list.commands[0].text.data(), "!") == 0);
  assert(render_list.commands[1].role == display::DisplayTextRole::Title);
  assert(render_list.commands[1].x_px == 22U);

  display::DisplayRenderGeometry invalid_geometry{};
  invalid_geometry.value_x_px = invalid_geometry.left_margin_px;
  display::DisplayRenderList invalid_list{};
  display::DisplayRenderListSurface invalid_surface{invalid_geometry, invalid_list};
  assert(!display::renderDisplayPage(page, invalid_surface));
  assert(invalid_list.command_count == 0U);
}

} // namespace

int main() {
  testStatusPageContainsOperationalState();
  testWarningsAreDerivedFromProjectedRuntimeTruth();
  testFreshnessPolicyIsExplicitPresenterConfig();
  testOutputsPageKeepsRequestedEffectiveAndPhysicalTruthSeparate();
  testNavigationMatchesFivePhysicalKeys();
  testTextSimulatorUsesTheSameSurfaceSeamAsHardwareAdapters();
  testRenderListSurfaceMapsPresenterDataToFixedGeometry();
  return 0;
}
