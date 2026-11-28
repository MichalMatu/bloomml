#include "IpoBoardAdmission.h"
#include "IpoOutputPlan.h"
#include "IpoSignalSelector.h"
#include "IpoInputReadingRegistry.h"
#include "climate/input/ConfiguredClockSource.h"
#include "climate/runtime/profile/IpoProfileWire.h"
#include "climate/runtime/profile/IpoProfileReadback.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

namespace ipo = growbox::ipo;
namespace runtime = growbox::app::climate_io::runtime;
namespace profile = growbox::app::climate_io::runtime::profile;

namespace {

void testMvpOutputPlans() {
  for (unsigned mask = 0; mask < 128U; ++mask) {
    ipo::Graph graph{};
    graph.processes[graph.process_count++] = {1U, ipo::ProcessKind::Co2Safety, true};
    unsigned expected = 0U;
    for (unsigned role = 0; role < 7U; ++role) {
      if (!(mask & (1U << role))) continue;
      const auto id = static_cast<std::uint16_t>(role + 1U);
      graph.devices[graph.device_count++] = {id, ipo::Driver::GpioRelay, id, true};
      graph.actuators[graph.actuator_count++] = {
          static_cast<std::uint16_t>(id + 20U), id, ipo::Channel::Switch,
          static_cast<ipo::OutputRole>(role), true, 1000U, 2000U};
      ++expected;
    }
    auto plan = ipo::buildOutputPlan(graph);
    assert(plan.valid() && plan.count == expected);
    for (unsigned role = 0; role < 7U; ++role) {
      const auto* entry = plan.find(static_cast<ipo::OutputRole>(role));
      assert((entry != nullptr) == bool(mask & (1U << role)));
      if (entry) {
        assert(entry->endpoint == role + 21U && entry->device_id == role + 1U);
        assert(entry->min_on_ms == 1000U && entry->min_off_ms == 2000U);
      }
    }
  }
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  assert(ipo::buildOutputPlan(graph).valid());
  graph.actuators[1].role = graph.actuators[0].role;
  graph.link_count = 0U;
  auto rejected = ipo::buildOutputPlan(graph);
  assert(rejected.status == ipo::OutputPlanStatus::DuplicateRole && rejected.count == 0U);
  graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.actuators[0].device_id = 999U;
  assert(!ipo::buildOutputPlan(graph).valid());
  graph = ipo::makeCrowPanelMintReferenceGraph();
  for (auto& device : graph.devices)
    if (device.id == graph.actuators[0].device_id) device.enabled = false;
  assert(ipo::buildOutputPlan(graph).status == ipo::OutputPlanStatus::DisabledDevice);
}

void testReferenceHardwareAdmission() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::Ok);
  const auto admission = ipo::assessCrowPanelReference(graph);
  assert(admission.matches_compiled_board);
  assert(!admission.apply_authorized);
  assert(admission.status == ipo::CrowPanelAdmissionStatus::CompatibleReferenceOnly);
}

void testCo2BindingCannotBeDeleted() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  // Remove only CO2 source, preserving clock and all physical routes.
  for (unsigned i = 2U; i + 1U < graph.signal_count; ++i) {
    graph.signals[i] = graph.signals[i + 1U];
  }
  --graph.signal_count;
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::Ok);
  const auto admission = ipo::assessCrowPanelReference(graph);
  assert(admission.status == ipo::CrowPanelAdmissionStatus::MissingRequiredInput);
  assert(!admission.matches_compiled_board);
}

void testRequiredRoutingCannotBeDeleted() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  // Delete only the protected co2_safety -> exhaust routing edge.
  for (unsigned i = 3U; i + 1U < graph.link_count; ++i) {
    graph.links[i] = graph.links[i + 1U];
  }
  --graph.link_count;
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::Ok);
  assert(ipo::assessCrowPanelReference(graph).status ==
         ipo::CrowPanelAdmissionStatus::MissingRequiredLink);
}

void testDwellAndPhysicalChannelClaimsFailClosed() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.actuators[0].min_off_ms = 119'999U;
  assert(ipo::assessCrowPanelReference(graph).status ==
         ipo::CrowPanelAdmissionStatus::UnsafeDwell);

  graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.actuators[1].device_id = graph.actuators[0].device_id;
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::DuplicatePhysicalEndpoint);
  assert(!ipo::assessCrowPanelReference(graph).matches_compiled_board);

  graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.devices[4].driver = ipo::Driver::GpioRelay;
  assert(ipo::assessCrowPanelReference(graph).status ==
         ipo::CrowPanelAdmissionStatus::UnsupportedDriver);
}

void testInvalidSensorAndPriorityFallback() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  // Generic selector supports a secondary SCD41 temperature channel, but
  // CrowPanel admission still rejects it for physical deployment.
  graph.signals[graph.signal_count++] = {
      ipo::SignalRole::InsideTemperature, 2U, ipo::Channel::Temperature, 1U};
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::Ok);
  assert(ipo::assessCrowPanelReference(graph).status ==
         ipo::CrowPanelAdmissionStatus::UnsupportedInputBinding);

  ipo::SignalReading observations[]{
      {1U, ipo::Channel::Temperature, 24.0, 30'000U, true},
      {2U, ipo::Channel::Temperature, 23.0, 1'000U, true},
  };
  auto selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, observations, 2U, 30'000U);
  assert(selected.usable() && selected.device_id == 1U && !selected.fallback);

  observations[0].age_ms = 30'001U;
  selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, observations, 2U, 30'000U);
  assert(selected.usable() && selected.device_id == 2U && selected.fallback);

  observations[1].age_ms = 30'001U;
  selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, observations, 2U, 30'000U);
  assert(!selected.usable() && selected.status == ipo::SignalSelectionStatus::NoFreshSource);

  observations[0].age_ms = 0U;
  observations[0].value = std::numeric_limits<double>::quiet_NaN();
  selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, observations, 2U, 30'000U);
  assert(!selected.usable());
  observations[0].value = 1.0e100;
  selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, observations, 2U, 30'000U);
  assert(!selected.usable()); // finite double must not overflow float

  observations[0].value = 24.0;
  ipo::SignalReading duplicates[]{observations[0], observations[0]};
  selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, duplicates, 2U, 30'000U);
  assert(!selected.usable() &&
         selected.status == ipo::SignalSelectionStatus::AmbiguousReading);
  selected = ipo::selectFreshSignal(
      graph, ipo::SignalRole::InsideTemperature, observations, 65U, 30'000U);
  assert(selected.status == ipo::SignalSelectionStatus::Capacity);
}

void testInputRegistryConfigurations() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  ipo::InputReadingRegistry frame(graph);
  const auto t = ipo::SignalRole::InsideTemperature;
  const auto rh = ipo::SignalRole::InsideHumidity;
  const auto co2 = ipo::SignalRole::InsideCo2;
  const auto clock = ipo::SignalRole::ClockTime;
  constexpr std::uint64_t timeout = 30000U;
  const auto fill = [&]() {
    assert(frame.record({1U, ipo::Channel::Temperature, 24.0, 10U, true}));
    assert(frame.record({1U, ipo::Channel::Humidity, 58.0, 10U, true}));
    assert(frame.record({2U, ipo::Channel::Temperature, 23.0, 20U, true}));
    assert(frame.record({2U, ipo::Channel::Humidity, 61.0, 20U, true}));
    assert(frame.record({2U, ipo::Channel::Co2, 650.0, 20U, true}));
    assert(frame.record({3U, ipo::Channel::Temperature, 22.0, 15U, true}));
    assert(frame.record({3U, ipo::Channel::Humidity, 52.0, 15U, true}));
    assert(frame.record({4U, ipo::Channel::UnixTime, 1795824001.0, 0U, true}));
  };
  fill();
  assert(frame.select(t, timeout).device_id == 1U);
  assert(frame.select(rh, timeout).device_id == 1U);
  assert(frame.select(co2, timeout).value == 650.0);
  assert(frame.select(clock, timeout).value == 1795824001.0);

  // Swap only signal bindings. Existing processes and actuator routes stay intact.
  graph.signals[0].device_id = 2U;
  graph.signals[1].device_id = 2U;
  frame.clear();
  assert(frame.record({2U, ipo::Channel::Temperature, 23.0, 0U, true}));
  assert(frame.record({2U, ipo::Channel::Humidity, 61.0, 0U, true}));
  assert(frame.record({2U, ipo::Channel::Co2, 650.0, 0U, true}));
  assert(frame.select(t, timeout).device_id == 2U);
  assert(frame.select(rh, timeout).value == 61.0);
  assert(frame.select(co2, timeout).usable());
  assert(!frame.select(clock, timeout).usable());
  assert(frame.select(clock, timeout).unavailable_sources == 1U);

  // BLE-only climate: no fabricated CO2; independent RTC may still be present.
  graph.signals[0].device_id = 3U;
  graph.signals[1].device_id = 1U;
  frame.clear();
  assert(frame.record({3U, ipo::Channel::Temperature, 22.0, 0U, true}));
  assert(frame.record({1U, ipo::Channel::Humidity, 58.0, 0U, true}));
  assert(frame.select(t, timeout).device_id == 3U);
  assert(frame.select(rh, timeout).device_id == 1U);
  assert(!frame.select(co2, timeout).usable());

  graph.signals[graph.signal_count++] = {t, 2U, ipo::Channel::Temperature, 1U};
  frame.clear();
  assert(frame.record({3U, ipo::Channel::Temperature, 22.0, timeout + 1U, true}));
  assert(frame.record({2U, ipo::Channel::Temperature, 23.0, 0U, true}));
  auto selected = frame.select(t, timeout);
  assert(selected.usable() && selected.fallback && selected.device_id == 2U);
  assert(selected.stale_sources == 1U);

  frame.clear();
  assert(frame.record({3U, ipo::Channel::Temperature, 22.0, 0U, false}));
  assert(frame.record({2U, ipo::Channel::Temperature, 23.0, 0U, true}));
  selected = frame.select(t, timeout);
  assert(selected.usable() && selected.invalid_sources == 1U);
  assert(!frame.record({2U, ipo::Channel::Temperature, 23.0, 0U, true}));
  assert(frame.select(t, timeout).status == ipo::SignalSelectionStatus::AmbiguousReading);
  frame.clear();
  assert(!frame.record({99U, ipo::Channel::Temperature, 23.0, 0U, true}));
  assert(!frame.select(t, timeout).usable());
  frame.clear();
  assert(frame.record({3U, ipo::Channel::Temperature, 22.0,
      std::numeric_limits<std::uint64_t>::max(), true}));
  assert(!frame.select(t, std::numeric_limits<std::uint64_t>::max()).usable());
  assert(frame.select(t, timeout).unavailable_sources == 2U);
  assert(!ipo::assessCrowPanelReference(graph).apply_authorized);
}

void testPhysicalLocatorIndependentOfConfiguredId() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.devices[0].id = 41U;
  graph.signals[0].device_id = 41U;
  graph.signals[1].device_id = 41U;
  ipo::InputReadingRegistry frame(graph);
  assert(frame.hasDevice(ipo::Driver::Tp357, 1U));
  assert(!frame.hasDevice(ipo::Driver::Tp357, 2U));
  assert(frame.recordFor(ipo::Driver::Tp357, 1U, ipo::Channel::Temperature, 26.0, 2U, true));
  const auto t = frame.select(ipo::SignalRole::InsideTemperature, 30U);
  assert(t.usable() && t.device_id == 41U && t.value == 26.0);
  assert(!frame.recordFor(ipo::Driver::Tp357, 2U, ipo::Channel::Temperature, 1.0, 0U, true));
  assert(frame.select(ipo::SignalRole::InsideTemperature, 30U).usable());
  frame.clear();
  graph.devices[0].enabled = false;
  assert(!frame.hasDevice(ipo::Driver::Tp357, 1U));
  assert(!frame.select(ipo::SignalRole::InsideTemperature, 30U).usable());
}

class FakeRtc final : public growbox::app::climate_io::ClimateClockSource {
public:
  bool sample(std::uint64_t, growbox::app::climate_io::ClimateWallClockSnapshot& out) noexcept override {
    ++calls;
    out = snapshot;
    return available;
  }
  growbox::app::climate_io::ClimateWallClockSnapshot snapshot{true, 1795824001ULL};
  bool available{true};
  unsigned calls{0U};
};

void testConfiguredClockSource() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.devices[3].id = 43U;
  graph.signals[5].device_id = 43U;
  FakeRtc rtc;
  runtime::ConfiguredClockSource source(graph, rtc);
  growbox::app::climate_io::ClimateWallClockSnapshot clock{};
  assert(source.sample(1000U, clock) && clock.unix_time_s == 1795824001ULL);
  assert(source.lastSelection().device_id == 43U);
  assert(source.sample(1000U, clock) && rtc.calls == 1U);
  rtc.snapshot.valid = false;
  assert(!source.sample(2000U, clock) && !clock.valid && clock.unix_time_s == 0U);
  assert(source.lastSelection().invalid_sources == 1U);
  rtc.available = false;
  assert(!source.sample(3000U, clock));
  assert(source.lastSelection().unavailable_sources == 1U);
  graph.devices[3].enabled = false;
  assert(!source.sample(4000U, clock) && rtc.calls == 3U);
  graph.devices[3].enabled = true;
  --graph.signal_count;
  rtc.available = true;
  rtc.snapshot.valid = true;
  assert(!source.sample(5000U, clock));
  assert(source.lastSelection().status == ipo::SignalSelectionStatus::MissingRole);
}

void testOptionalElectricalChannelsRemainDraftOnly() {
  auto graph = ipo::makeCrowPanelMintReferenceGraph();
  graph.devices[graph.device_count++] = {42U, ipo::Driver::ShellyMeter, 1U, true};
  graph.signals[graph.signal_count++] = {
      ipo::SignalRole::ActivePower, 42U, ipo::Channel::ActivePower, 0U};
  graph.signals[graph.signal_count++] = {
      ipo::SignalRole::Energy, 42U, ipo::Channel::Energy, 0U};
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::Ok);
  ipo::InputReadingRegistry frame(graph);
  assert(!frame.select(ipo::SignalRole::ActivePower, 30000U).usable());
  assert(frame.record({42U, ipo::Channel::ActivePower, 0.0, 1U, true}));
  assert(frame.record({42U, ipo::Channel::Energy, 150.0, 1U, true}));
  assert(frame.select(ipo::SignalRole::ActivePower, 30000U).usable());
  assert(frame.select(ipo::SignalRole::Energy, 30000U).value == 150.0);
  assert(!frame.select(ipo::SignalRole::Energy, 0U).usable());
  const auto admission = ipo::assessCrowPanelReference(graph);
  assert(!admission.matches_compiled_board && !admission.apply_authorized);
  graph.signals[graph.signal_count - 1U].channel = ipo::Channel::ActivePower;
  assert(ipo::validateGraph(graph) == ipo::GraphStatus::ChannelMismatch);
  assert(!ipo::driverProvides(ipo::Driver::ShellyMeter, ipo::Channel::Switch));
}

void testSymbolicOnlyArbitration() {
  const auto graph = ipo::makeCrowPanelMintReferenceGraph();
  const ipo::Proposal proposals[]{
      {201U, 101U, ipo::Effect::Ventilate, 0.45F},
      {202U, 101U, ipo::Effect::Ventilate, 0.90F},
      {203U, 101U, ipo::Effect::Ventilate, 1.0F}, // safety is a separate owner
      {204U, 102U, ipo::Effect::Illuminate, 1.0F},
      {204U, 101U, ipo::Effect::Ventilate, 1.0F}, // not linked
  };
  const auto result = ipo::previewCompatibleIntents(
      graph, proposals, sizeof(proposals) / sizeof(proposals[0]));
  assert(result.status == ipo::GraphStatus::Ok);
  assert(result.count == 2U);
  assert(result.rejected == 2U);
  assert(result.intents[0].actuator_id == 101U);
  assert(result.intents[0].level == 0.90F);
  assert(result.intents[0].contributor_count == 2U);
  assert(result.intents[1].actuator_id == 102U);
}

void testScheduleBoundariesAndOwner() {
  auto schedule = runtime::makeMintRuntimeScheduleProfile();
  assert(runtime::validateRuntimeScheduleProfile(schedule));
  assert(runtime::isRuntimeDaytime(6U, schedule));
  assert(!runtime::isRuntimeDaytime(22U, schedule));
  schedule.day_start_hour = 20U;
  schedule.night_start_hour = 7U;
  assert(runtime::isRuntimeDaytime(23U, schedule));
  assert(runtime::isRuntimeDaytime(2U, schedule));
  assert(!runtime::isRuntimeDaytime(12U, schedule));

  schedule.day.air_vpd_kpa = std::numeric_limits<float>::quiet_NaN();
  assert(!runtime::validateRuntimeScheduleProfile(schedule));

  runtime::RuntimeScheduleProfileOwner owner{};
  assert(owner.bootDefaultMint());
  assert(owner.generation() == 0U);
  assert(!owner.replaceValidated(schedule, 0U));
  assert(owner.generation() == 0U);
}

void testCo2ScheduleAndLegacyWire() {
  auto schedule = runtime::makeMintRuntimeScheduleProfile();
  const auto original_hash = runtime::runtimeScheduleFingerprint(schedule);
  schedule.day.co2_enabled = true;
  schedule.day.co2_ppm = 900.0F;
  schedule.night.co2_enabled = true;
  schedule.night.co2_ppm = 600.0F;
  assert(runtime::validateRuntimeScheduleProfile(schedule));
  assert(runtime::runtimeScheduleFingerprint(schedule) != original_hash);
  growbox::app::climate_io::ClimateScheduleConfigSnapshot selected{};
  assert(runtime::projectRuntimeSchedule(6U, schedule, selected));
  assert(selected.targets.co2_enabled && selected.targets.co2_ppm == 900.0F);
  assert(!selected.capabilities.co2_doser);
  assert(runtime::projectRuntimeSchedule(22U, schedule, selected));
  assert(selected.targets.co2_enabled && selected.targets.co2_ppm == 600.0F);
  profile::DurableIpoProfile source{};
  source.graph = ipo::makeCrowPanelMintReferenceGraph();
  source.schedule = schedule;
  profile::EncodedIpoProfile blob{};
  assert(profile::encodeIpoProfile(source, blob) == profile::IpoProfileCodecStatus::Ok);
  profile::DurableIpoProfile decoded{};
  assert(profile::decodeIpoProfile(blob.bytes.data(), blob.size, decoded) ==
         profile::IpoProfileCodecStatus::Ok);
  assert(decoded.schedule.day.co2_enabled && decoded.schedule.day.co2_ppm == 900.0F);
  assert(decoded.schedule.night.co2_enabled && decoded.schedule.night.co2_ppm == 600.0F);
  // Construct the previous layout by removing exactly the added 10 bytes
  // and restoring the original version and CRC, independently of the decoder.
  constexpr std::size_t co2_offset = 4+2+4+5+2+1+1+1+8+24;
  assert(blob.bytes[co2_offset] == 1U);
  auto legacy = blob;
  for (std::size_t i = co2_offset; i + 10 < blob.size; ++i)
    legacy.bytes[i] = blob.bytes[i+10];
  legacy.size -= 10;
  legacy.bytes[4] = 1U;
  const auto crc = profile::profileCrc32(legacy.bytes.data(), legacy.size-4U);
  for (unsigned i = 0; i < 4; ++i)
    legacy.bytes[legacy.size-4U+i] = static_cast<std::uint8_t>(crc >> (8U*i));
  assert(profile::decodeIpoProfile(legacy.bytes.data(), legacy.size, decoded) ==
         profile::IpoProfileCodecStatus::Ok);
  assert(!decoded.schedule.day.co2_enabled && decoded.schedule.day.co2_ppm == 0.0F);
  assert(!decoded.schedule.night.co2_enabled && decoded.schedule.night.co2_ppm == 0.0F);
  source.schedule.day.co2_ppm = 1800.0F;
  assert(profile::encodeIpoProfile(source, blob) == profile::IpoProfileCodecStatus::InvalidSchedule);
  source.schedule.day.co2_enabled = false;
  source.schedule.day.co2_ppm = 900.0F;
  assert(!runtime::validateRuntimeScheduleProfile(source.schedule));
}

void testFullTypedWireRoundtripAndCorruption() {
  profile::DurableIpoProfile p{};
  p.graph = ipo::makeCrowPanelMintReferenceGraph();
  p.schedule = runtime::makeMintRuntimeScheduleProfile();
  p.generation = 7U;
  assert(profile::validateDurableIpoProfile(p) == profile::IpoProfileCodecStatus::Ok);

  profile::EncodedIpoProfile blob{};
  assert(profile::encodeIpoProfile(p, blob) == profile::IpoProfileCodecStatus::Ok);
  assert(blob.size > 60U && blob.size <= profile::kMaximumProfileWireBytes);
  profile::DurableIpoProfile decoded{};
  assert(profile::decodeIpoProfile(blob.bytes.data(), blob.size, decoded) ==
         profile::IpoProfileCodecStatus::Ok);
  assert(decoded.generation == 7U);
  assert(decoded.graph.device_count == 7U);
  assert(decoded.graph.link_count == 5U);
  assert(decoded.schedule.day.temperature_c == 24.5F);
  assert(decoded.schedule.night.relative_humidity_pct == 65.0F);

  blob.bytes[20U] ^= 0x80U;
  assert(profile::decodeIpoProfile(blob.bytes.data(), blob.size, decoded) ==
         profile::IpoProfileCodecStatus::ChecksumMismatch);
  assert(decoded.generation == 1U); // output reset, not corrupted untrusted data

  assert(profile::decodeIpoProfile(nullptr, 0U, decoded) ==
         profile::IpoProfileCodecStatus::InvalidLength);
  assert(profile::decodeIpoProfile(blob.bytes.data(), blob.size - 1U, decoded) !=
         profile::IpoProfileCodecStatus::Ok);
  p.schedule.exhaust_fan_available = false;
  assert(profile::encodeIpoProfile(p, blob) ==
         profile::IpoProfileCodecStatus::InvalidSchedule);
  p.schedule.exhaust_fan_available = true;
  p.graph.actuators[0].safe_off = false;
  assert(profile::encodeIpoProfile(p, blob) ==
         profile::IpoProfileCodecStatus::InvalidGraph);
  assert(blob.size == 0U);
}

void testStoredProfileIsNotActiveProfileEvidence() {
  runtime::RuntimeScheduleProfileOwner active{};
  profile::IpoJournalReadResult store{};
  store.status = profile::IpoJournalStatus::Ok;
  store.stored = true;
  store.profile.graph = ipo::makeCrowPanelMintReferenceGraph();
  store.profile.schedule = runtime::makeMintRuntimeScheduleProfile();
  store.profile.generation = 11U;
  store.stored_record_fingerprint = 0x1234ABCDU;
  store.stored_fingerprint_valid = true;
  const auto truth = profile::buildIpoProfileReadback(active, store);
  assert(truth.runtime_valid);
  assert(truth.active_is_builtin_mint);
  assert(!truth.active_is_operator_v2);
  assert(truth.active_ram_generation == 0U);
  assert(truth.stored_record_valid);
  assert(truth.stored_generation == 11U);
  assert(truth.stored_fingerprint_valid);
  assert(truth.stored_record_fingerprint == 0x1234ABCDU);

  store.status = profile::IpoJournalStatus::RecoveredFromBackup;
  store.selected_backup = true;
  const auto backup = profile::buildIpoProfileReadback(active, store);
  assert(backup.stored_is_backup && !backup.active_is_operator_v2);

  store.status = profile::IpoJournalStatus::Corrupt;
  const auto corrupt = profile::buildIpoProfileReadback(active, store);
  assert(!corrupt.stored_record_valid);
  assert(!corrupt.stored_fingerprint_valid);
  assert(!corrupt.active_is_operator_v2);
}

} // namespace

int main() {
  testMvpOutputPlans();
  testCo2ScheduleAndLegacyWire();
  testReferenceHardwareAdmission();
  testCo2BindingCannotBeDeleted();
  testRequiredRoutingCannotBeDeleted();
  testDwellAndPhysicalChannelClaimsFailClosed();
  testInvalidSensorAndPriorityFallback();
  testInputRegistryConfigurations();
  testPhysicalLocatorIndependentOfConfiguredId();
  testConfiguredClockSource();
  testOptionalElectricalChannelsRemainDraftOnly();
  testSymbolicOnlyArbitration();
  testScheduleBoundariesAndOwner();
  testFullTypedWireRoundtripAndCorruption();
  testStoredProfileIsNotActiveProfileEvidence();
  return 0;
}
