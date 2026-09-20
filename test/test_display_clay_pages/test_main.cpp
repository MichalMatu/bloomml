#include "climate/display/DisplayClayRenderCoordinator.h"
#include "climate/display/DisplayPresenter.h"
#include "growbox_clay_ui/HostSimulator.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace display = growbox::app::climate_io::display;
namespace output = growbox::app::output;
namespace storage = growbox::app::climate_io::storage;

namespace {

constexpr std::array<std::uint64_t, 4U> kGoldenPageHashes{
    0x3bb469ff366c69bfULL,
    0x2e43841abef23957ULL,
    0x262017750c71fa89ULL,
    0xc22a38670d9064f1ULL,
};

std::uint64_t byteHash(const std::uint8_t* data, std::size_t size) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::size_t index = 0U; index < size; ++index) {
    hash ^= data[index];
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::uint64_t frameHash(const growbox::clay_ui::MonochromeFrame& frame) {
  return byteHash(frame.bytes.data(), frame.bytes.size());
}

class FakeFramebufferBackend final {
public:
  bool beginFrame(std::uint16_t width_px, std::uint16_t height_px, bool warning,
                  display::DisplayRefreshKind refresh_kind) noexcept {
    ++begin_count;
    last_warning = warning;
    last_refresh_kind = refresh_kind;
    if (!begin_result || open || width_px != growbox::clay_ui::kDisplayWidth ||
        height_px != growbox::clay_ui::kDisplayHeight ||
        refresh_kind == display::DisplayRefreshKind::None) {
      return false;
    }
    open = true;
    return true;
  }

  std::uint8_t* framebufferData() noexcept {
    return open && expose_framebuffer ? framebuffer.data() : nullptr;
  }

  std::size_t framebufferBytes() const noexcept {
    return reported_framebuffer_bytes;
  }

  bool setContentRegion(const display::DisplayRegion& region) noexcept {
    ++set_content_region_count;
    if (!open || !set_content_region_result) {
      return false;
    }
    content_region = region;
    return true;
  }

  bool endFrame() noexcept {
    ++end_count;
    if (!open || !end_result) {
      return false;
    }
    open = false;
    return true;
  }

  void cancelFrame() noexcept {
    ++cancel_count;
    open = false;
  }

  std::array<std::uint8_t, growbox::clay_ui::kFrameBytes> framebuffer{};
  std::size_t reported_framebuffer_bytes{growbox::clay_ui::kFrameBytes};
  std::size_t begin_count{0U};
  std::size_t end_count{0U};
  std::size_t cancel_count{0U};
  std::size_t set_content_region_count{0U};
  display::DisplayRegion content_region{};
  display::DisplayRefreshKind last_refresh_kind{display::DisplayRefreshKind::None};
  bool begin_result{true};
  bool end_result{true};
  bool set_content_region_result{true};
  bool expose_framebuffer{true};
  bool last_warning{false};
  bool open{false};
};

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
  assert(summary.content_region.width_px > 0U);
  assert(summary.content_region.height_px > 0U);
  assert(static_cast<std::uint32_t>(summary.content_region.x_px) +
             summary.content_region.width_px <=
         growbox::clay_ui::kDisplayWidth);
  assert(static_cast<std::uint32_t>(summary.content_region.y_px) +
             summary.content_region.height_px <=
         growbox::clay_ui::kDisplayHeight);

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

void testProductionClayBridgeUsesBackendOwnedFramebuffer() {
  const auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};
  frame.page = display::DisplayPage::Environment;
  frame.refresh_kind = display::DisplayRefreshKind::Full;
  frame.refresh_reason = display::DisplayRefreshReason::Initial;
  assert(display::buildDisplayPage(snapshot, frame.page, frame.page_model));

  std::vector<std::uint8_t> arena(growbox::clay_ui::pageRendererArenaBytes());
  FakeFramebufferBackend backend{};
  growbox::clay_ui::RenderSummary summary{};
  assert(display::renderClayDisplayFrame(frame, arena.data(), arena.size(), backend, &summary));
  assert(backend.begin_count == 1U);
  assert(backend.end_count == 1U);
  assert(backend.cancel_count == 0U);
  assert(backend.set_content_region_count == 1U);
  assert(!display::displayRegionEmpty(backend.content_region));
  assert(!backend.open);
  assert(backend.last_refresh_kind == display::DisplayRefreshKind::Full);
  assert(!backend.last_warning);
  assert(summary.render_commands == 25U);
  assert(byteHash(backend.framebuffer.data(), backend.framebuffer.size()) == kGoldenPageHashes[0]);
}

void testProductionClayBridgeCancelsIncompleteFrames() {
  const auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};
  frame.page = display::DisplayPage::Environment;
  frame.refresh_kind = display::DisplayRefreshKind::Partial;
  frame.refresh_reason = display::DisplayRefreshReason::ContentChanged;
  assert(display::buildDisplayPage(snapshot, frame.page, frame.page_model));

  const std::size_t required_arena = growbox::clay_ui::pageRendererArenaBytes();
  std::vector<std::uint8_t> arena(required_arena);

  FakeFramebufferBackend short_buffer{};
  short_buffer.reported_framebuffer_bytes = growbox::clay_ui::kFrameBytes - 1U;
  assert(!display::renderClayDisplayFrame(frame, arena.data(), arena.size(), short_buffer));
  assert(short_buffer.begin_count == 1U);
  assert(short_buffer.end_count == 0U);
  assert(short_buffer.cancel_count == 1U);
  assert(!short_buffer.open);

  FakeFramebufferBackend short_arena{};
  assert(!display::renderClayDisplayFrame(frame, arena.data(), required_arena - 1U, short_arena));
  assert(short_arena.begin_count == 1U);
  assert(short_arena.end_count == 0U);
  assert(short_arena.cancel_count == 1U);
  assert(!short_arena.open);

  FakeFramebufferBackend content_region_failure{};
  content_region_failure.set_content_region_result = false;
  assert(
      !display::renderClayDisplayFrame(frame, arena.data(), arena.size(), content_region_failure));
  assert(content_region_failure.begin_count == 1U);
  assert(content_region_failure.set_content_region_count == 1U);
  assert(content_region_failure.end_count == 0U);
  assert(content_region_failure.cancel_count == 1U);
  assert(!content_region_failure.open);

  FakeFramebufferBackend end_failure{};
  end_failure.end_result = false;
  assert(!display::renderClayDisplayFrame(frame, arena.data(), arena.size(), end_failure));
  assert(end_failure.begin_count == 1U);
  assert(end_failure.set_content_region_count == 1U);
  assert(end_failure.end_count == 1U);
  assert(end_failure.cancel_count == 1U);
  assert(!end_failure.open);

  FakeFramebufferBackend no_refresh{};
  frame.refresh_kind = display::DisplayRefreshKind::None;
  assert(!display::renderClayDisplayFrame(frame, arena.data(), arena.size(), no_refresh));
  assert(no_refresh.begin_count == 0U);
  assert(no_refresh.cancel_count == 0U);
}

} // namespace

int main() {
  testPresenterNavigationAndFourClayPages();
  testWarningStateIsVisibleInClayFrame();
  testProductionClayBridgeUsesBackendOwnedFramebuffer();
  testProductionClayBridgeCancelsIncompleteFrames();
  return 0;
}
