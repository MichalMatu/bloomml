#include "climate/output/persistence/OutputPersistenceSchema.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
namespace output = growbox::app::output;
constexpr output::OutputEndpointId kFan = 1U;
constexpr output::OutputEndpointId kLamp = 2U;
constexpr output::OutputEndpointId kHumidifier = 3U;

output::OutputPolicyConfig safePolicy() {
  return output::makeSafeDefaultOutputPolicyConfig(kFan, kLamp, kHumidifier);
}

output::OutputPersistenceSnapshot sampleSnapshot() {
  output::OutputPersistenceSnapshot snapshot{};
  assert(output::makeSafeOutputPersistenceSnapshot(safePolicy(), snapshot));
  snapshot.policy.max_transition_failures = 2U;
  auto* lamp = const_cast<output::OutputEndpointPolicy*>(
      output::findOutputPolicyRole(snapshot.policy, output::OutputEndpointRole::ScheduledLight));
  assert(lamp != nullptr);
  lamp->lifecycle[output::outputLifecycleEventIndex(output::OutputLifecycleEvent::AutomationOff)]
      .delay_ms = 250U;
  snapshot.commands[0].has_last_successful_command = true;
  snapshot.commands[0].state = output::BinaryOutputState::On;
  snapshot.commands[1].has_last_successful_command = true;
  snapshot.commands[1].state = output::BinaryOutputState::Off;
  assert(output::validateOutputPersistenceSnapshot(snapshot) ==
         output::OutputPersistenceStatus::Ok);
  return snapshot;
}

void assertSafeFallback(const output::OutputPersistenceDecodeResult& result,
                        output::OutputPersistenceStatus expected) {
  assert(result.status == expected);
  assert(result.used_safe_defaults);
  assert(output::validateOutputPolicyConfig(result.snapshot.policy) ==
         output::OutputPolicyConfigStatus::Ok);
  assert(result.snapshot.command_count == 3U);
  for (std::size_t index = 0U; index < result.snapshot.command_count; ++index) {
    assert(!result.snapshot.commands[index].has_last_successful_command);
    assert(result.snapshot.commands[index].state == output::BinaryOutputState::Off);
  }
}

void testLegacyV1AndVariableCapacity() {
  const std::uint8_t legacy[] = {71U, 79U, 80U, 49U, 1U, 0U, 134U, 0U, 113U, 125U, 223U, 74U, 1U, 0U, 3U, 1U, 1U, 0U, 0U, 1U, 1U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U, 0U, 0U, 0U, 0U, 1U, 1U, 2U, 0U, 1U, 1U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 3U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 3U, 0U, 2U, 1U, 2U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 2U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 2U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 2U, 0U, 0U, 0U, 0U, 1U, 1U, 3U, 1U, 0U, 1U, 1U, 2U, 0U, 0U, 0U, 3U, 0U, 0U, 0U};
  auto decoded = output::decodeOutputPersistence(legacy, sizeof(legacy), safePolicy());
  assert(decoded.status == output::OutputPersistenceStatus::Ok);
  assert(decoded.snapshot.command_count == 3U);
  assert(decoded.snapshot.commands[0].has_last_successful_command);
  assert(decoded.snapshot.commands[0].state == output::BinaryOutputState::On);
  for (unsigned count = 1; count <= 7; ++count) {
    output::OutputPolicyConfig policy{};
    policy.count = static_cast<std::uint8_t>(count);
    for (unsigned i = 0; i < count; ++i) {
      auto& ep = policy.endpoints[i];
      ep.endpoint = static_cast<output::OutputEndpointId>(20U+i);
      ep.role = static_cast<output::OutputEndpointRole>(i);
      for (auto& a : ep.lifecycle) a.order = static_cast<std::uint8_t>(i);
    }
    output::OutputPersistenceSnapshot snapshot{};
    assert(output::makeSafeOutputPersistenceSnapshot(policy, snapshot));
    output::OutputPersistenceBlob blob{};
    assert(output::encodeOutputPersistence(snapshot, blob) == output::OutputPersistenceStatus::Ok);
    decoded = output::decodeOutputPersistence(blob.bytes.data(), blob.bytes.size(), policy);
    assert(decoded.status == output::OutputPersistenceStatus::Ok);
    assert(decoded.snapshot.command_count == count);
  }
}

void testRoundTrip() {
  const auto source = sampleSnapshot();
  output::OutputPersistenceBlob blob{};
  assert(output::encodeOutputPersistence(source, blob) == output::OutputPersistenceStatus::Ok);
  const auto decoded =
      output::decodeOutputPersistence(blob.bytes.data(), blob.bytes.size(), safePolicy());
  assert(decoded.status == output::OutputPersistenceStatus::Ok);
  assert(!decoded.used_safe_defaults);
  assert(decoded.snapshot.policy.max_transition_failures == 2U);
  const auto* lamp = output::findOutputPolicyRole(decoded.snapshot.policy,
                                                  output::OutputEndpointRole::ScheduledLight);
  assert(lamp != nullptr);
  assert(lamp->lifecycle[output::outputLifecycleEventIndex(
                             output::OutputLifecycleEvent::AutomationOff)]
             .delay_ms == 250U);
  assert(decoded.snapshot.commands[0].endpoint == kFan);
  assert(decoded.snapshot.commands[0].has_last_successful_command);
  assert(decoded.snapshot.commands[0].state == output::BinaryOutputState::On);
  assert(decoded.snapshot.commands[1].endpoint == kLamp);
  assert(decoded.snapshot.commands[1].has_last_successful_command);
  assert(decoded.snapshot.commands[1].state == output::BinaryOutputState::Off);
}

void testCorruptPayloadFallsBack() {
  output::OutputPersistenceBlob blob{};
  assert(output::encodeOutputPersistence(sampleSnapshot(), blob) ==
         output::OutputPersistenceStatus::Ok);
  blob.bytes[output::kOutputPersistenceHeaderSize + 7U] ^= 0x40U;
  assertSafeFallback(
      output::decodeOutputPersistence(blob.bytes.data(), blob.bytes.size(), safePolicy()),
      output::OutputPersistenceStatus::ChecksumMismatch);
}

void testUnknownVersionFallsBack() {
  output::OutputPersistenceBlob blob{};
  assert(output::encodeOutputPersistence(sampleSnapshot(), blob) ==
         output::OutputPersistenceStatus::Ok);
  blob.bytes[4U] = 0xFFU;
  blob.bytes[5U] = 0x7FU;
  assertSafeFallback(
      output::decodeOutputPersistence(blob.bytes.data(), blob.bytes.size(), safePolicy()),
      output::OutputPersistenceStatus::UnsupportedVersion);
}

void testTruncatedPayloadFallsBack() {
  output::OutputPersistenceBlob blob{};
  assert(output::encodeOutputPersistence(sampleSnapshot(), blob) ==
         output::OutputPersistenceStatus::Ok);
  assertSafeFallback(
      output::decodeOutputPersistence(blob.bytes.data(), blob.bytes.size() - 1U, safePolicy()),
      output::OutputPersistenceStatus::InvalidLength);
}

void testMissingPayloadMigratesToSafeDefaultsWithoutCommandTruth() {
  assertSafeFallback(output::decodeOutputPersistence(nullptr, 0U, safePolicy()),
                     output::OutputPersistenceStatus::InvalidLength);
}

void testInvalidSnapshotCannotEncode() {
  auto snapshot = sampleSnapshot();
  snapshot.commands[1].endpoint = snapshot.commands[0].endpoint;
  output::OutputPersistenceBlob blob{};
  assert(output::encodeOutputPersistence(snapshot, blob) ==
         output::OutputPersistenceStatus::InvalidCommandState);
  for (const auto byte : blob.bytes) {
    assert(byte == 0U);
  }
}

void testFormatIsFixedSizeAndCarriesNoPhysicalStateContract() {
  static_assert(output::kOutputPersistenceEncodedSize == 290U);
  static_assert(sizeof(output::DurableOutputCommandState) <= 8U);
  const auto snapshot = sampleSnapshot();
  (void)snapshot;
  // Durable command state intentionally contains only endpoint, presence and binary command state.
}

} // namespace

int main() {
  testLegacyV1AndVariableCapacity();
  testRoundTrip();
  testCorruptPayloadFallsBack();
  testUnknownVersionFallsBack();
  testTruncatedPayloadFallsBack();
  testMissingPayloadMigratesToSafeDefaultsWithoutCommandTruth();
  testInvalidSnapshotCannotEncode();
  testFormatIsFixedSizeAndCarriesNoPhysicalStateContract();
  return 0;
}
