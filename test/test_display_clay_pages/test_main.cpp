#include "climate/display/DisplayPresenter.h"
#include "growbox_clay_ui/HostSimulator.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace display = growbox::app::climate_io::display;
namespace output = growbox::app::output;
namespace storage = growbox::app::climate_io::storage;

namespace {

constexpr std::array<std::uint64_t, 4U> kGoldenPageHashes{
    0x2d40c7bccbf2e12bULL,
    0xb7e3999ea15d8d2bULL,
    0x430713d76487fc25ULL,
    0x3be1346eb2b8c1e5ULL,
};

std::uint64_t frameHash(const growbox::clay_ui::MonochromeFrame& frame) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const std::uint8_t byte : frame.bytes) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

display::DisplaySnapshot nominalSnapshot() {
  display::DisplaySnapshot snapshot{};
  snapshot.uptime_ms = 123'000U;
  std::snprintf(snapshot.firmware_sha.data(), snapshot.firmware_sha.size(), "abc1234");
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

std::uint64_t renderPage(const display::DisplaySnapshot& snapshot, display::DisplayPage page,
                         const char* expected_title) {
  display::DisplayPageModel model{};
  assert(display::buildDisplayPage(snapshot, page, model));
  assert(std::strcmp(model.title.data(), expected_title) == 0);
  assert(model.line_count == 10U);

  growbox::clay_ui::MonochromeFrame first{};
  growbox::clay_ui::MonochromeFrame second{};
  growbox::clay_ui::RenderSummary summary{};
  assert(growbox::clay_ui::renderHostPage(model, first, &summary));
  assert(growbox::clay_ui::renderHostPage(model, second));
  assert(first.bytes == second.bytes);
  assert(summary.width == growbox::clay_ui::kDisplayWidth);
  assert(summary.height == growbox::clay_ui::kDisplayHeight);
  assert(summary.black_pixels > 0U);
  assert(summary.render_commands == 25U);

  const std::uint64_t hash = frameHash(first);
  std::printf("CLAY_PAGE_HASH %s %016llx commands=%zu black=%zu\n", expected_title,
              static_cast<unsigned long long>(hash), summary.render_commands, summary.black_pixels);
  return hash;
}

void testPresenterNavigationAndFourClayPages() {
  const auto snapshot = nominalSnapshot();
  display::DisplayNavigation navigation{};
  std::array<std::uint64_t, 4U> hashes{};

  assert(navigation.page() == display::DisplayPage::Environment);
  hashes[0] = renderPage(snapshot, navigation.page(), "Growbox status");
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Outputs);
  hashes[1] = renderPage(snapshot, navigation.page(), "Outputs");
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::System);
  hashes[2] = renderPage(snapshot, navigation.page(), "System");
  assert(navigation.handle(display::DisplayButton::Next));
  assert(navigation.page() == display::DisplayPage::Diagnostics);
  hashes[3] = renderPage(snapshot, navigation.page(), "Diagnostics");

  assert(hashes == kGoldenPageHashes);

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
}

void testWarningStateIsVisibleInClayFrame() {
  auto snapshot = nominalSnapshot();
  const std::uint64_t nominal =
      renderPage(snapshot, display::DisplayPage::Environment, "Growbox status");

  snapshot.co2_ppm.age_ms = 45'000U;
  snapshot.storage.sd_mounted = false;
  display::DisplayPageModel warning_model{};
  assert(display::buildDisplayPage(snapshot, display::DisplayPage::Environment, warning_model));
  assert(warning_model.warning);
  growbox::clay_ui::MonochromeFrame warning{};
  assert(growbox::clay_ui::renderHostPage(warning_model, warning));
  assert(frameHash(warning) != nominal);
}

} // namespace

int main() {
  testPresenterNavigationAndFourClayPages();
  testWarningStateIsVisibleInClayFrame();
  return 0;
}
