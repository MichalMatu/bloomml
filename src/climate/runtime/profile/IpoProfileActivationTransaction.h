#pragma once

#include "climate/runtime/profile/IpoProfileReadback.h"
#include "climate/runtime/profile/IpoProfileStagingCoordinator.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::runtime::profile {

// Caller must serialize operations outside the 1-second control hot path.
// Gate contract: independently qualify board SHA, sensor freshness, lamp
// interlocks and a quiesced OutputSupervisor; keep the interlock throughout
// commit/apply/readback; emergencyLock must be latched beyond end().
class IpoActivationSafetyGate {
public:
  virtual ~IpoActivationSafetyGate() = default;
  virtual bool begin(const DurableIpoProfile& candidate) noexcept = 0;
  virtual bool stillSafe() noexcept = 0;
  virtual void emergencyLock() noexcept = 0;
  virtual void end() noexcept = 0;
};

// Production remains fail-closed until exact-SHA hardware qualification and
// a supervisor-owned safe transition are explicitly implemented.
class ClosedIpoActivationGate final : public IpoActivationSafetyGate {
public:
  bool begin(const DurableIpoProfile&) noexcept override { return false; }
  bool stillSafe() noexcept override { return false; }
  void emergencyLock() noexcept override {}
  void end() noexcept override {}
};

enum class IpoActivationResult : std::uint8_t {
  Idle, Prepared, NotPrepared, TokenConflict, GenerationConflict,
  InvalidCandidate, QualificationMissing, StorageFailed, SafetyChanged,
  RuntimeApplyFailed, ReadbackFailed, RolledBack, Quarantined, AppliedVerified
};

class IpoProfileActivationTransaction final {
public:
  IpoProfileActivationTransaction(IpoProfileNvsJournal& journal,
                                  RuntimeScheduleProfileOwner& runtime,
                                  IpoJournalReadResult& observed_store,
                                  IpoActivationSafetyGate& gate) noexcept
      : journal_(journal), runtime_(runtime), observed_store_(observed_store),
        gate_(gate), staging_(journal) {}

  // The preview token is the exact wire CRC32 + stored base generation.
  // Preview never changes NVS, active RAM, RF or other physical outputs.
  IpoStagePreview preview(const char* json, std::size_t length) noexcept {
    preview_ = staging_.prepare(json, length);
    expected_runtime_generation_ = runtime_.generation();
    expected_runtime_hash_ = runtimeScheduleFingerprint(runtime_.active());
    last_ = preview_.valid_candidate ? IpoActivationResult::Prepared
                                     : IpoActivationResult::InvalidCandidate;
    return preview_;
  }

  void cancelPreview() noexcept {
    cancelPrepared();
    last_ = IpoActivationResult::Idle;
  }

  IpoActivationResult activate(std::uint32_t base_generation,
                               std::uint32_t candidate_crc32) noexcept {
    if (!preview_.valid_candidate || !staging_.hasPreparedCandidate() ||
        staging_.preparedProfile() == nullptr)
      return finish(IpoActivationResult::NotPrepared);
    if (preview_.base_generation != base_generation ||
        preview_.candidate_crc32 != candidate_crc32)
      return finish(IpoActivationResult::TokenConflict);
    if (runtime_.generation() != expected_runtime_generation_ ||
        runtimeScheduleFingerprint(runtime_.active()) != expected_runtime_hash_) {
      cancelPrepared();
      return finish(IpoActivationResult::GenerationConflict);
    }

    const DurableIpoProfile candidate = *staging_.preparedProfile();
    if (candidate.generation != preview_.candidate_generation ||
        validateDurableIpoProfile(candidate) != IpoProfileCodecStatus::Ok) {
      cancelPrepared();
      return finish(IpoActivationResult::InvalidCandidate);
    }
    if (!gate_.begin(candidate))
      return finish(IpoActivationResult::QualificationMissing);

    const IpoJournalReadResult before = journal_.load();
    const std::uint32_t old_stored_generation = before.status == IpoJournalStatus::Ok
                                                   ? before.profile.generation : 0U;
    if ((before.status != IpoJournalStatus::Ok &&
         before.status != IpoJournalStatus::NotFound) ||
        old_stored_generation != base_generation ||
        runtime_.generation() != expected_runtime_generation_ ||
        runtimeScheduleFingerprint(runtime_.active()) != expected_runtime_hash_) {
      gate_.end();
      cancelPrepared();
      return finish(IpoActivationResult::GenerationConflict);
    }
    if (!gate_.stillSafe()) {
      gate_.end();
      cancelPrepared();
      return finish(IpoActivationResult::SafetyChanged);
    }

    const RuntimeScheduleProfile previous_runtime = runtime_.active();
    const bool prior_mint = origin_mint_;
    const bool prior_v2 = origin_v2_;
    const std::uint32_t prior_crc = verified_store_crc_;
    const std::uint32_t prior_generation = verified_store_generation_;

    // Roll back both boundaries and verify their actual readback, including
    // the initial empty-NVS case. Never report success if either is uncertain.
    const auto rollback = [&](IpoActivationResult reason, bool ram_changed) noexcept {
      last_failure_ = reason;
      bool ram_ok = true;
      if (ram_changed)
        ram_ok = runtime_.replaceValidated(previous_runtime, runtime_.generation());
      const IpoJournalReadResult after_failure = journal_.load();
      bool nvs_ok = false;
      if (after_failure.status == IpoJournalStatus::Ok &&
          after_failure.profile.generation == candidate.generation)
        nvs_ok = journal_.rollbackTo(candidate.generation, old_stored_generation) ==
                 IpoJournalStatus::Ok;
      else
        nvs_ok = sameStore(before, after_failure);
      observed_store_ = journal_.load();
      nvs_ok = nvs_ok && sameStore(before, observed_store_);
      ram_ok = ram_ok && runtime_.valid() &&
          runtimeScheduleFingerprint(runtime_.active()) ==
              runtimeScheduleFingerprint(previous_runtime);
      if (!ram_ok || !nvs_ok) {
        origin_mint_ = false;
        origin_v2_ = false;
        gate_.emergencyLock();
        gate_.end();
        cancelPrepared();
        return finish(IpoActivationResult::Quarantined);
      }
      origin_mint_ = prior_mint;
      origin_v2_ = prior_v2;
      verified_store_crc_ = prior_crc;
      verified_store_generation_ = prior_generation;
      verified_runtime_generation_ = runtime_.generation();
      verified_runtime_hash_ = runtimeScheduleFingerprint(runtime_.active());
      gate_.end();
      cancelPrepared();
      return finish(IpoActivationResult::RolledBack);
    };

    const IpoStagingStatus saved = staging_.commitStoredOnly(candidate_crc32, base_generation);
    observed_store_ = journal_.load();
    if (saved != IpoStagingStatus::StoredNotApplied)
      return rollback(IpoActivationResult::StorageFailed, false);
    if (!gate_.stillSafe())
      return rollback(IpoActivationResult::SafetyChanged, false);

    // Only the shared schedule/Rule RAM owner is modified here. No direct
    // OutputSupervisor, RF433, GPIO, lamp-safety or ML command is created.
    if (!runtime_.replaceValidated(candidate.schedule, expected_runtime_generation_))
      return rollback(IpoActivationResult::RuntimeApplyFailed, false);
    if (!gate_.stillSafe())
      return rollback(IpoActivationResult::SafetyChanged, true);

    observed_store_ = journal_.load();
    const bool stored_matches = observed_store_.status == IpoJournalStatus::Ok &&
        observed_store_.stored && observed_store_.stored_fingerprint_valid &&
        observed_store_.profile.generation == candidate.generation &&
        observed_store_.stored_record_fingerprint == candidate_crc32;
    const bool ram_matches = runtime_.valid() &&
        runtime_.generation() == expected_runtime_generation_ + 1U &&
        runtimeScheduleFingerprint(runtime_.active()) ==
            runtimeScheduleFingerprint(candidate.schedule);
    if (!stored_matches || !ram_matches)
      return rollback(IpoActivationResult::ReadbackFailed, true);

    origin_mint_ = false;
    origin_v2_ = true;
    verified_store_crc_ = candidate_crc32;
    verified_store_generation_ = candidate.generation;
    verified_runtime_generation_ = runtime_.generation();
    verified_runtime_hash_ = runtimeScheduleFingerprint(runtime_.active());
    gate_.end();
    cancelPrepared();
    last_failure_ = IpoActivationResult::Idle;
    return finish(IpoActivationResult::AppliedVerified);
  }

  // Observer truth is derived from actual RAM and NVS evidence. A stored
  // record alone cannot make active_is_operator_v2 true, even after reboot.
  IpoProfileReadback readback() const noexcept {
    IpoProfileReadback out = buildIpoProfileReadback(runtime_, observed_store_);
    const bool verified = origin_v2_ && out.runtime_valid &&
        out.stored_record_valid && !out.stored_is_backup &&
        out.stored_fingerprint_valid &&
        out.active_ram_generation == verified_runtime_generation_ &&
        out.active_schedule_hash == verified_runtime_hash_ &&
        out.stored_generation == verified_store_generation_ &&
        out.stored_record_fingerprint == verified_store_crc_;
    out.active_is_operator_v2 = verified;
    out.active_is_builtin_mint = !verified && origin_mint_ &&
        out.active_schedule_hash ==
            runtimeScheduleFingerprint(makeMintRuntimeScheduleProfile());
    return out;
  }

  IpoActivationResult lastResult() const noexcept { return last_; }
  IpoActivationResult lastFailure() const noexcept { return last_failure_; }

private:
  static bool sameStore(const IpoJournalReadResult& a,
                        const IpoJournalReadResult& b) noexcept {
    if (a.status == IpoJournalStatus::NotFound)
      return b.status == IpoJournalStatus::NotFound;
    return a.status == IpoJournalStatus::Ok && b.status == IpoJournalStatus::Ok &&
        a.stored && b.stored && a.profile.generation == b.profile.generation &&
        a.stored_fingerprint_valid && b.stored_fingerprint_valid &&
        a.stored_record_fingerprint == b.stored_record_fingerprint;
  }
  void cancelPrepared() noexcept {
    staging_.cancel();
    preview_ = {};
  }
  IpoActivationResult finish(IpoActivationResult outcome) noexcept {
    last_ = outcome;
    return outcome;
  }

  IpoProfileNvsJournal& journal_;
  RuntimeScheduleProfileOwner& runtime_;
  IpoJournalReadResult& observed_store_;
  IpoActivationSafetyGate& gate_;
  IpoProfileStagingCoordinator staging_;
  IpoStagePreview preview_{};
  std::uint32_t expected_runtime_generation_{0U};
  std::uint32_t expected_runtime_hash_{0U};
  bool origin_mint_{true};
  bool origin_v2_{false};
  std::uint32_t verified_store_generation_{0U};
  std::uint32_t verified_store_crc_{0U};
  std::uint32_t verified_runtime_generation_{0U};
  std::uint32_t verified_runtime_hash_{0U};
  IpoActivationResult last_{IpoActivationResult::Idle};
  IpoActivationResult last_failure_{IpoActivationResult::Idle};
};

} // namespace growbox::app::climate_io::runtime::profile
