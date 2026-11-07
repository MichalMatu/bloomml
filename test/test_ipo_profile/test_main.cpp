#include "IpoBoardAdmission.h"
#include "IpoSignalSelector.h"
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
  testReferenceHardwareAdmission();
  testCo2BindingCannotBeDeleted();
  testRequiredRoutingCannotBeDeleted();
  testDwellAndPhysicalChannelClaimsFailClosed();
  testInvalidSensorAndPriorityFallback();
  testSymbolicOnlyArbitration();
  testScheduleBoundariesAndOwner();
  testFullTypedWireRoundtripAndCorruption();
  testStoredProfileIsNotActiveProfileEvidence();
  return 0;
}
