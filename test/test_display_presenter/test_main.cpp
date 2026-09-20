#include "climate/display/DisplayPresenter.h"
#include "climate/display/DisplayRenderAdapter.h"
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

class FakeClaySink final {
public:
  bool beginFrame(std::uint16_t width_px, std::uint16_t height_px, bool warning) noexcept {
    if (frame_open) {
      return false;
    }
    ++begin_count;
    width = width_px;
    height = height_px;
    frame_warning = warning;
    frame_open = true;
    return true;
  }

  bool drawText(const display::DisplayTextElement& element) noexcept {
    if (!frame_open || element.text == nullptr) {
      return false;
    }
    ++text_count;
    if (!draw_result) {
      return false;
    }

    if (std::strcmp(element.text, "!") == 0) {
      saw_warning = true;
      warning_style = element.style;
    } else if (std::strcmp(element.text, "Growbox status") == 0) {
      saw_title = true;
      title_style = element.style;
      title_x = element.x_px;
    } else if (std::strcmp(element.text, "Temp") == 0) {
      saw_label = true;
      label_style = element.style;
    } else if (std::strcmp(element.text, "23.4 C") == 0) {
      saw_value = true;
      value_style = element.style;
    }
    return true;
  }

  bool endFrame() noexcept {
    if (!frame_open) {
      return false;
    }
    ++end_count;
    if (!end_result) {
      return false;
    }
    frame_open = false;
    ended = true;
    return true;
  }

  void cancelFrame() noexcept {
    ++cancel_count;
    frame_open = false;
  }

  bool draw_result{true};
  bool end_result{true};
  std::size_t begin_count{0U};
  std::size_t text_count{0U};
  std::size_t end_count{0U};
  std::size_t cancel_count{0U};
  std::uint16_t width{0U};
  std::uint16_t height{0U};
  std::uint16_t title_x{0U};
  bool frame_warning{false};
  bool frame_open{false};
  bool ended{false};
  bool saw_warning{false};
  bool saw_title{false};
  bool saw_label{false};
  bool saw_value{false};
  display::DisplayTextStyle warning_style{};
  display::DisplayTextStyle title_style{};
  display::DisplayTextStyle label_style{};
  display::DisplayTextStyle value_style{};
};

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

void testEnvironmentPageContainsAuthoritativeClimateState() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, page));
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
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, page));
  assert(page.warning);
  assert(std::strcmp(findLine(page, "SCD41")->value.data(), "STALE 45s") == 0);
  assert(std::strcmp(findLine(page, "Safety")->value.data(), "LATCH OVER TEMP") == 0);

  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Outputs, page));
  assert(std::strcmp(findLine(page, "Lamp")->value.data(), "100% -> OFF !S") == 0);

  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Diagnostics, page));
  assert(std::strcmp(findLine(page, "Storage")->value.data(), "SD FAULT") == 0);
}

void testFreshnessPolicyIsExplicitPresenterConfig() {
  auto snapshot = nominalSnapshot();
  snapshot.co2_ppm.age_ms = 45'000U;

  display::DisplayPresenterConfig config{};
  config.sensor_stale_after_ms = 60'000U;
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, config, page));
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

void testSystemPageContainsRuntimeAndPlatformState() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::System, page));
  assert(std::strcmp(page.title.data(), "System") == 0);
  assert(page.line_count == 10U);
  assert(std::strcmp(findLine(page, "Mode")->value.data(), "AUTO") == 0);
  assert(std::strcmp(findLine(page, "Automation")->value.data(), "REQUESTED") == 0);
  assert(std::strcmp(findLine(page, "Transport")->value.data(), "ACTIVE") == 0);
  assert(std::strcmp(findLine(page, "Lifecycle")->value.data(), "ACTIVE") == 0);
  assert(std::strcmp(findLine(page, "BLE")->value.data(), "SCANNING") == 0);
  assert(std::strcmp(findLine(page, "RTC")->value.data(), "2026-01-01 01:00") == 0);
  assert(std::strcmp(findLine(page, "Storage")->value.data(), "SD OK") == 0);
  assert(std::strcmp(findLine(page, "FW")->value.data(), "--") == 0);
  assert(std::strcmp(findLine(page, "Uptime")->value.data(), "123s") == 0);
  assert(std::strcmp(findLine(page, "Safety")->value.data(), "OK") == 0);
}

void testNavigationMatchesFivePhysicalKeys() {
  display::DisplayNavigation navigation;
  assert(navigation.page() == display::DisplayPage::Environment);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Outputs);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::System);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Diagnostics);
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Environment);
  assert(navigation.handle(display::DisplayButton::Previous));
  assert(navigation.page() == display::DisplayPage::Diagnostics);
  assert(navigation.handle(display::DisplayButton::Home));
  assert(navigation.page() == display::DisplayPage::Environment);
  assert(navigation.handle(display::DisplayButton::Ok));
  assert(navigation.page() == display::DisplayPage::Outputs);
  assert(navigation.handle(display::DisplayButton::Back));
  assert(navigation.page() == display::DisplayPage::Environment);
  assert(!navigation.handle(display::DisplayButton::Back));
}

void testTextSimulatorUsesTheSameSurfaceSeamAsHardwareAdapters() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, page));

  display::DisplayTextSimulator simulator{};
  assert(display::renderDisplayPage(page, simulator));
  assert(simulator.size() > 0U);
  assert(std::strstr(simulator.text(), "Growbox status") != nullptr);
  assert(std::strstr(simulator.text(), "23.4 C") != nullptr);
  assert(std::strstr(simulator.text(), "OK 2s") != nullptr);
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
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, page));

  display::DisplayRenderGeometry geometry{};
  display::DisplayRenderList render_list{};
  display::DisplayRenderListSurface surface{geometry, render_list};
  assert(display::renderDisplayPage(page, surface));
  assert(render_list.command_count == 20U);
  assert(!render_list.warning);

  const auto& first_label = render_list.commands[0];
  const auto& first_value = render_list.commands[1];
  assert(first_label.role == display::DisplayTextRole::Label);
  assert(first_label.x_px == 8U);
  assert(first_label.y_px == 14U);
  assert(std::strcmp(first_label.text.data(), "Temp") == 0);
  assert(first_value.role == display::DisplayTextRole::Value);
  assert(first_value.x_px == 92U);
  assert(first_value.y_px == 14U);
  assert(std::strcmp(first_value.text.data(), "23.4 C") == 0);

  page.warning = true;
  assert(display::renderDisplayPage(page, surface));
  assert(render_list.command_count == 21U);
  assert(render_list.warning);
  assert(render_list.commands[0].role == display::DisplayTextRole::WarningMarker);
  assert(std::strcmp(render_list.commands[0].text.data(), "!") == 0);
  assert(render_list.commands[0].x_px == 276U);
  assert(render_list.commands[0].y_px == 4U);
  assert(render_list.commands[1].role == display::DisplayTextRole::Label);
  assert(std::strcmp(render_list.commands[1].text.data(), "Temp") == 0);
  assert(render_list.commands[1].y_px == 14U);

  display::DisplayRenderGeometry invalid_geometry{};
  invalid_geometry.value_x_px = invalid_geometry.left_margin_px;
  display::DisplayRenderList invalid_list{};
  display::DisplayRenderListSurface invalid_surface{invalid_geometry, invalid_list};
  assert(!display::renderDisplayPage(page, invalid_surface));
  assert(invalid_list.command_count == 0U);
}

void testClayAdapterMapsRenderRolesAndFailsClosedBeforeFrame() {
  const auto snapshot = nominalSnapshot();
  display::DisplayPageModel page{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, page));
  page.warning = true;

  display::DisplayRenderGeometry geometry{};
  geometry.show_title = true;
  display::DisplayRenderList render_list{};
  display::DisplayRenderListSurface surface{geometry, render_list};
  assert(display::renderDisplayPage(page, surface));

  display::DisplayTheme theme{};
  theme.title = {1U, 16U, true};
  theme.warning = {2U, 16U, true};
  theme.label = {3U, 10U, false};
  theme.value = {4U, 10U, false};

  FakeClaySink sink{};
  assert(display::renderDisplayList(render_list, geometry, theme, sink));
  assert(sink.begin_count == 1U);
  assert(sink.text_count == render_list.command_count);
  assert(sink.width == geometry.width_px);
  assert(sink.height == geometry.height_px);
  assert(sink.frame_warning);
  assert(sink.ended);
  assert(sink.saw_warning);
  assert(sink.saw_title);
  assert(sink.saw_label);
  assert(sink.saw_value);
  assert(sink.warning_style.font_id == 2U);
  assert(sink.warning_style.font_size_px == 16U);
  assert(sink.warning_style.emphasized);
  assert(sink.title_style.font_id == 1U);
  assert(sink.title_style.emphasized);
  assert(sink.label_style.font_id == 3U);
  assert(!sink.label_style.emphasized);
  assert(sink.value_style.font_id == 4U);
  assert(sink.title_x == 22U);

  auto invalid_list = render_list;
  invalid_list.commands[0].max_width_px = geometry.width_px;
  FakeClaySink invalid_sink{};
  assert(!display::renderDisplayList(invalid_list, geometry, theme, invalid_sink));
  assert(invalid_sink.begin_count == 0U);
  assert(invalid_sink.text_count == 0U);
  assert(invalid_sink.cancel_count == 0U);

  auto invalid_theme = theme;
  invalid_theme.value.font_size_px = 0U;
  FakeClaySink invalid_theme_sink{};
  assert(!display::renderDisplayList(render_list, geometry, invalid_theme, invalid_theme_sink));
  assert(invalid_theme_sink.begin_count == 0U);
  assert(invalid_theme_sink.cancel_count == 0U);

  FakeClaySink draw_failure_sink{};
  draw_failure_sink.draw_result = false;
  assert(!display::renderDisplayList(render_list, geometry, theme, draw_failure_sink));
  assert(draw_failure_sink.begin_count == 1U);
  assert(draw_failure_sink.cancel_count == 1U);
  assert(!draw_failure_sink.frame_open);
  draw_failure_sink.draw_result = true;
  assert(display::renderDisplayList(render_list, geometry, theme, draw_failure_sink));
  assert(draw_failure_sink.begin_count == 2U);

  FakeClaySink end_failure_sink{};
  end_failure_sink.end_result = false;
  assert(!display::renderDisplayList(render_list, geometry, theme, end_failure_sink));
  assert(end_failure_sink.end_count == 1U);
  assert(end_failure_sink.cancel_count == 1U);
  assert(!end_failure_sink.frame_open);
  end_failure_sink.end_result = true;
  assert(display::renderDisplayList(render_list, geometry, theme, end_failure_sink));
  assert(end_failure_sink.begin_count == 2U);
}

} // namespace

int main() {
  testEnvironmentPageContainsAuthoritativeClimateState();
  testWarningsAreDerivedFromProjectedRuntimeTruth();
  testFreshnessPolicyIsExplicitPresenterConfig();
  testOutputsPageKeepsRequestedEffectiveAndPhysicalTruthSeparate();
  testSystemPageContainsRuntimeAndPlatformState();
  testNavigationMatchesFivePhysicalKeys();
  testTextSimulatorUsesTheSameSurfaceSeamAsHardwareAdapters();
  testRenderListSurfaceMapsPresenterDataToFixedGeometry();
  testClayAdapterMapsRenderRolesAndFailsClosedBeforeFrame();
  return 0;
}
