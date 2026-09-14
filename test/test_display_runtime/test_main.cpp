#include "climate/display/DisplayRuntime.h"
#include "climate/output/LampSafety.h"

#include <cassert>
#include <cstring>

namespace display = growbox::app::climate_io::display;
namespace output = growbox::app::output;
namespace stage28d = growbox::app::climate_io::stage28d;
namespace storage = growbox::app::climate_io::storage;

namespace {

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

void testInitialRefreshRetriesUntilRenderedAndRoutineChangesCoalesce() {
  display::DisplayRuntimeConfig config{};
  config.refresh.minimum_refresh_interval_ms = 5'000U;
  config.refresh.full_refresh_every_partial = 10U;
  display::DisplayRuntimeController runtime{config};

  auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};
  assert(runtime.update(snapshot, 0U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(frame.refresh_reason == display::DisplayRefreshReason::Initial);
  assert(frame.page == display::DisplayPage::Environment);
  assert(frame.render_list.command_count == 20U);
  assert(runtime.hasPendingRefresh());

  display::DisplayRuntimeFrame retry{};
  assert(runtime.update(snapshot, 1'000U, retry));
  assert(retry.refresh_kind == display::DisplayRefreshKind::Full);
  assert(retry.refresh_reason == display::DisplayRefreshReason::Initial);
  assert(runtime.hasPendingRefresh());
  assert(runtime.confirmRendered(retry, 1'000U));
  assert(!runtime.hasPendingRefresh());
  assert(!runtime.confirmRendered(retry, 1'001U));

  assert(runtime.update(snapshot, 1'001U, frame));
  assert(!frame.refreshRequired());

  snapshot.temperature_c.value = 24.0F;
  assert(runtime.update(snapshot, 1'002U, frame));
  assert(!frame.refreshRequired());

  assert(runtime.update(snapshot, 6'000U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Partial);
  assert(frame.refresh_reason == display::DisplayRefreshReason::ContentChanged);
  assert(runtime.confirmRendered(frame, 6'000U));
}

void testNavigationRefreshBypassesRoutineInterval() {
  display::DisplayRuntimeConfig config{};
  config.refresh.minimum_refresh_interval_ms = 60'000U;
  display::DisplayRuntimeController runtime{config};
  const auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};

  assert(runtime.update(snapshot, 10U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(runtime.confirmRendered(frame, 10U));

  assert(runtime.handleButton(display::DisplayButton::Next));
  assert(runtime.page() == display::DisplayPage::Outputs);
  assert(runtime.update(snapshot, 11U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Partial);
  assert(frame.refresh_reason == display::DisplayRefreshReason::Navigation);
  assert(frame.page == display::DisplayPage::Outputs);
  assert(std::strcmp(frame.page_model.title.data(), "Outputs") == 0);
  assert(runtime.confirmRendered(frame, 11U));

  assert(runtime.handleButton(display::DisplayButton::Next));
  assert(runtime.page() == display::DisplayPage::System);
  assert(runtime.update(snapshot, 12U, frame));
  assert(frame.refresh_reason == display::DisplayRefreshReason::Navigation);
  assert(frame.page == display::DisplayPage::System);
  assert(std::strcmp(frame.page_model.title.data(), "System") == 0);
  assert(runtime.confirmRendered(frame, 12U));
}

void testWarningIdentityChangesForceImmediateFullRefresh() {
  display::DisplayRuntimeConfig config{};
  config.refresh.minimum_refresh_interval_ms = 60'000U;
  display::DisplayRuntimeController runtime{config};
  auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};

  assert(runtime.update(snapshot, 100U, frame));
  assert(runtime.confirmRendered(frame, 100U));

  snapshot.safety_latched = true;
  snapshot.safety_reason_code =
      static_cast<std::uint32_t>(stage28d::LampSafetyReason::OverTemperature);
  snapshot.lamp.safety_override = true;
  snapshot.lamp.effective_state = output::BinaryOutputState::Off;

  assert(runtime.update(snapshot, 101U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(frame.refresh_reason == display::DisplayRefreshReason::WarningChanged);
  assert(frame.page_model.warning);
  assert(frame.page_model.warning_mask ==
         static_cast<std::uint8_t>(display::DisplayWarning::Safety));
  assert(frame.page_model.safety_warning_reason_code == snapshot.safety_reason_code);
  assert(frame.render_list.warning);
  assert(runtime.confirmRendered(frame, 101U));

  snapshot.safety_reason_code =
      static_cast<std::uint32_t>(stage28d::LampSafetyReason::InvalidConfig);
  assert(runtime.update(snapshot, 102U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(frame.refresh_reason == display::DisplayRefreshReason::WarningChanged);
  assert(frame.page_model.warning_mask ==
         static_cast<std::uint8_t>(display::DisplayWarning::Safety));
  assert(frame.page_model.safety_warning_reason_code == snapshot.safety_reason_code);
  assert(runtime.confirmRendered(frame, 102U));

  snapshot.safety_latched = false;
  snapshot.safety_reason_code = static_cast<std::uint32_t>(stage28d::LampSafetyReason::Safe);
  snapshot.lamp.safety_override = false;
  snapshot.rtc_trusted = false;
  assert(runtime.update(snapshot, 103U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(frame.refresh_reason == display::DisplayRefreshReason::WarningChanged);
  assert(frame.page_model.warning);
  assert(frame.page_model.warning_mask ==
         static_cast<std::uint8_t>(display::DisplayWarning::Clock));
  assert(frame.page_model.safety_warning_reason_code == 0U);
  assert(runtime.confirmRendered(frame, 103U));

  snapshot.temperature_c.value = 24.0F;
  assert(runtime.update(snapshot, 104U, frame));
  assert(!frame.refreshRequired());
}

void testFullRefreshCadenceAdvancesOnlyAfterRenderedFrames() {
  display::DisplayRuntimeConfig config{};
  config.refresh.minimum_refresh_interval_ms = 0U;
  config.refresh.full_refresh_every_partial = 2U;
  display::DisplayRuntimeController runtime{config};
  auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};

  assert(runtime.update(snapshot, 0U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(runtime.confirmRendered(frame, 0U));

  snapshot.temperature_c.value = 23.5F;
  assert(runtime.update(snapshot, 1U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Partial);
  assert(frame.refresh_reason == display::DisplayRefreshReason::ContentChanged);

  display::DisplayRuntimeFrame retry{};
  assert(runtime.update(snapshot, 2U, retry));
  assert(retry.refresh_kind == display::DisplayRefreshKind::Partial);
  assert(retry.refresh_reason == display::DisplayRefreshReason::ContentChanged);
  assert(runtime.confirmRendered(retry, 2U));

  snapshot.temperature_c.value = 23.6F;
  assert(runtime.update(snapshot, 3U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(frame.refresh_reason == display::DisplayRefreshReason::ContentChanged);
  assert(runtime.confirmRendered(frame, 3U));

  runtime.requestRefresh(true);
  assert(runtime.update(snapshot, 4U, frame));
  assert(frame.refresh_kind == display::DisplayRefreshKind::Full);
  assert(frame.refresh_reason == display::DisplayRefreshReason::Requested);
  assert(runtime.confirmRendered(frame, 4U));
}

void testStalePlannedFrameCannotConsumeNewNavigationRequest() {
  display::DisplayRuntimeConfig config{};
  config.refresh.minimum_refresh_interval_ms = 60'000U;
  display::DisplayRuntimeController runtime{config};
  const auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};

  assert(runtime.update(snapshot, 0U, frame));
  assert(runtime.confirmRendered(frame, 0U));

  runtime.requestRefresh();
  assert(runtime.update(snapshot, 1U, frame));
  assert(frame.refresh_reason == display::DisplayRefreshReason::Requested);
  const auto stale_frame = frame;

  assert(runtime.handleButton(display::DisplayButton::Next));
  assert(!runtime.hasPendingRefresh());
  assert(!runtime.confirmRendered(stale_frame, 2U));

  assert(runtime.update(snapshot, 2U, frame));
  assert(frame.refresh_reason == display::DisplayRefreshReason::Navigation);
  assert(frame.page == display::DisplayPage::Outputs);
  assert(runtime.confirmRendered(frame, 2U));
}

void testInvalidGeometryFailsClosedWithoutRefresh() {
  display::DisplayRuntimeConfig config{};
  config.geometry.value_x_px = config.geometry.left_margin_px;
  display::DisplayRuntimeController runtime{config};
  const auto snapshot = nominalSnapshot();
  display::DisplayRuntimeFrame frame{};

  assert(!runtime.update(snapshot, 0U, frame));
  assert(!frame.refreshRequired());
  assert(frame.render_list.command_count == 0U);
  assert(!runtime.hasPendingRefresh());
  assert(!runtime.confirmRendered(frame, 0U));
}

} // namespace

int main() {
  testInitialRefreshRetriesUntilRenderedAndRoutineChangesCoalesce();
  testNavigationRefreshBypassesRoutineInterval();
  testWarningIdentityChangesForceImmediateFullRefresh();
  testFullRefreshCadenceAdvancesOnlyAfterRenderedFrames();
  testStalePlannedFrameCannotConsumeNewNavigationRequest();
  testInvalidGeometryFailsClosedWithoutRefresh();
  return 0;
}
