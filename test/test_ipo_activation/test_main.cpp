#include "IpoBoardAdmission.h"
#include "climate/runtime/profile/IpoProfileActivationTransaction.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace runtime = growbox::app::climate_io::runtime;
namespace profile = growbox::app::climate_io::runtime::profile;
namespace ipo = growbox::ipo;

// Host-only adapter substitutes storage and JSON decoding; the test executes
// the real production transaction, wire validation, staging and RAM owner.
// No ESP-IDF, NVS partition, RF or physical output is touched.
namespace growbox::app::climate_io::runtime::profile {
struct FakeJournal {
  bool has_record{false};
  bool has_backup{false};
  DurableIpoProfile record{};
  DurableIpoProfile backup{};
  bool fail_after_switch{false};
  bool fail_rollback{false};
  bool falsify_crc{false};
  unsigned commits{0U};
  unsigned rollbacks{0U};
};
static FakeJournal fake{};

IpoJournalReadResult IpoProfileNvsJournal::load() noexcept {
  IpoJournalReadResult result{};
  if (!fake.has_record) {
    result.status = IpoJournalStatus::NotFound;
    return result;
  }
  result.status = IpoJournalStatus::Ok;
  result.stored = true;
  result.profile = fake.record;
  EncodedIpoProfile encoded{};
  assert(encodeIpoProfile(fake.record, encoded) == IpoProfileCodecStatus::Ok);
  result.stored_record_fingerprint = profileCrc32(encoded.bytes.data(), encoded.size);
  if (fake.falsify_crc) result.stored_record_fingerprint ^= 1U;
  result.stored_fingerprint_valid = true;
  return result;
}

IpoJournalStatus IpoProfileNvsJournal::commit(
    const DurableIpoProfile& candidate, std::uint32_t expected_generation) noexcept {
  ++fake.commits;
  if (validateDurableIpoProfile(candidate) != IpoProfileCodecStatus::Ok)
    return IpoJournalStatus::InvalidCandidate;
  const std::uint32_t current = fake.has_record ? fake.record.generation : 0U;
  if (current != expected_generation || candidate.generation != current + 1U)
    return IpoJournalStatus::GenerationConflict;
  fake.backup = fake.record;
  fake.has_backup = fake.has_record;
  fake.record = candidate;
  fake.has_record = true;
  return fake.fail_after_switch ? IpoJournalStatus::WriteFailed : IpoJournalStatus::Ok;
}

IpoJournalStatus IpoProfileNvsJournal::rollbackTo(
    std::uint32_t expected_current, std::uint32_t previous) noexcept {
  ++fake.rollbacks;
  if (fake.fail_rollback) return IpoJournalStatus::WriteFailed;
  if (!fake.has_record || fake.record.generation != expected_current)
    return IpoJournalStatus::GenerationConflict;
  if (previous == 0U) {
    fake.has_record = false;
    fake.has_backup = false;
    fake.record = {};
    return IpoJournalStatus::Ok;
  }
  if (!fake.has_backup || fake.backup.generation != previous)
    return IpoJournalStatus::ReadbackFailed;
  fake.record = fake.backup;
  fake.has_backup = false;
  return IpoJournalStatus::Ok;
}

// A fixed, admitted typed candidate isolates activation state semantics from
// parser coverage; the real JSON decoder is qualified separately.
IpoJsonDecodeResult decodeIpoDeviceProfileJson(
    const char* json, std::size_t length, std::uint32_t generation) noexcept {
  IpoJsonDecodeResult result{};
  if (json == nullptr || length != 2U || std::memcmp(json, "ok", 2U) != 0)
    return result;
  result.profile.graph = ipo::makeCrowPanelMintReferenceGraph();
  result.profile.schedule = runtime::makeMintRuntimeScheduleProfile();
  result.profile.schedule.day.temperature_c = 26.0F;
  result.profile.generation = generation;
  result.status = IpoJsonStatus::Ok;
  result.admitted = true;
  return result;
}
} // namespace growbox::app::climate_io::runtime::profile

namespace {
struct FakeGate final : profile::IpoActivationSafetyGate {
  bool authorized{true};
  unsigned fail_safe_at{0U};
  unsigned safe_calls{0U};
  unsigned begins{0U};
  unsigned ends{0U};
  unsigned locks{0U};
  bool begin(const profile::DurableIpoProfile&) noexcept override {
    ++begins;
    return authorized;
  }
  bool stillSafe() noexcept override {
    ++safe_calls;
    return fail_safe_at == 0U || fail_safe_at != safe_calls;
  }
  void emergencyLock() noexcept override { ++locks; }
  void end() noexcept override { ++ends; }
};

void reset() {
  profile::fake = {};
}

void testClosedGateAndNoStoredEqualsActive() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  profile::ClosedIpoActivationGate gate;
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(p.valid_candidate && p.status == profile::IpoStagingStatus::Prepared);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::QualificationMissing);
  assert(profile::fake.commits == 0U && ram.generation() == 0U);
  const auto status = txn.readback();
  assert(status.active_is_builtin_mint && !status.active_is_operator_v2);
  assert(!status.stored_record_valid);
}

void testSuccessfulQualifiedApplyAndRebootSeparation() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  FakeGate gate;
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::AppliedVerified);
  const auto status = txn.readback();
  assert(status.active_is_operator_v2 && !status.active_is_builtin_mint);
  assert(status.active_ram_generation == 1U && status.stored_generation == 1U);
  assert(status.stored_record_fingerprint == p.candidate_crc32);
  assert(ram.active().day.temperature_c == 26.0F);
  assert(gate.ends == 1U && gate.locks == 0U);

  // Reboot reads stored generation 1, but built-in Mint RAM is *not* replaced.
  runtime::RuntimeScheduleProfileOwner cold_boot_ram;
  auto cold_store = journal.load();
  profile::ClosedIpoActivationGate cold_gate;
  profile::IpoProfileActivationTransaction cold(journal, cold_boot_ram,
                                                cold_store, cold_gate);
  const auto cold_truth = cold.readback();
  assert(cold_truth.active_is_builtin_mint && !cold_truth.active_is_operator_v2);
  assert(cold_truth.stored_generation == 1U && cold_truth.active_ram_generation == 0U);
}

void testTokenAndGenerationConflictNoWrites() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  FakeGate gate;
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32 ^ 1U) ==
         profile::IpoActivationResult::TokenConflict);
  assert(gate.begins == 0U && profile::fake.commits == 0U);
  runtime::RuntimeScheduleProfile edited = ram.active();
  edited.day.temperature_c = 25.0F;
  assert(ram.replaceValidated(edited, 0U));
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::GenerationConflict);
  assert(profile::fake.commits == 0U);
}

void testFailAfterStoredCommitRollsBackToEmpty() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  FakeGate gate;
  gate.fail_safe_at = 2U; // fails after NVS commit, before RAM apply
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::RolledBack);
  assert(txn.lastFailure() == profile::IpoActivationResult::SafetyChanged);
  assert(ram.generation() == 0U && !profile::fake.has_record);
  assert(profile::fake.rollbacks == 1U && gate.locks == 0U);
  assert(txn.readback().active_is_builtin_mint);
}

void testFailAfterRamApplyRestoresBoth() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  FakeGate gate;
  gate.fail_safe_at = 3U; // fails after RAM apply, before final readback
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::RolledBack);
  assert(ram.generation() == 2U && !profile::fake.has_record);
  assert(ram.active().day.temperature_c == 24.5F);
  assert(txn.readback().active_is_builtin_mint);
  assert(!txn.readback().active_is_operator_v2);
}

void testPowerLossSimulationAndCRCFailure() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  FakeGate gate;
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  profile::fake.fail_after_switch = true;
  auto p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::RolledBack);
  assert(txn.lastFailure() == profile::IpoActivationResult::StorageFailed);
  assert(!profile::fake.has_record && ram.generation() == 0U);

  profile::fake.fail_after_switch = false;
  profile::fake.falsify_crc = true;
  p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::RolledBack);
  assert(txn.lastFailure() == profile::IpoActivationResult::ReadbackFailed);
  assert(!profile::fake.has_record && txn.readback().active_is_builtin_mint);
}

void testFailedRollbackQuarantinesOutputs() {
  reset();
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  FakeGate gate;
  gate.fail_safe_at = 2U;
  profile::fake.fail_rollback = true;
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::Quarantined);
  assert(gate.locks == 1U && gate.ends == 1U);
  assert(!txn.readback().active_is_operator_v2);
  assert(profile::fake.has_record && ram.generation() == 0U);
}

void testRollbackToPreviousGoodGeneration() {
  reset();
  profile::fake.has_record = true;
  profile::fake.record.graph = ipo::makeCrowPanelMintReferenceGraph();
  profile::fake.record.schedule = runtime::makeMintRuntimeScheduleProfile();
  profile::fake.record.generation = 7U;
  profile::IpoProfileNvsJournal journal;
  runtime::RuntimeScheduleProfileOwner ram;
  auto observed = journal.load();
  const auto old_crc = observed.stored_record_fingerprint;
  FakeGate gate;
  gate.fail_safe_at = 3U;
  profile::IpoProfileActivationTransaction txn(journal, ram, observed, gate);
  const auto p = txn.preview("ok", 2U);
  assert(p.base_generation == 7U);
  assert(txn.activate(p.base_generation, p.candidate_crc32) ==
         profile::IpoActivationResult::RolledBack);
  assert(profile::fake.record.generation == 7U);
  assert(txn.readback().stored_record_fingerprint == old_crc);
  assert(txn.readback().active_is_builtin_mint);
}
} // namespace

int main() {
  testClosedGateAndNoStoredEqualsActive();
  testSuccessfulQualifiedApplyAndRebootSeparation();
  testTokenAndGenerationConflictNoWrites();
  testFailAfterStoredCommitRollsBackToEmpty();
  testFailAfterRamApplyRestoresBoth();
  testPowerLossSimulationAndCRCFailure();
  testFailedRollbackQuarantinesOutputs();
  testRollbackToPreviousGoodGeneration();
  return 0;
}
