#pragma once

#include "climate/runtime/profile/IpoProfileJsonDecode.h"
#include "climate/runtime/profile/IpoProfileNvsJournal.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace growbox::app::climate_io::runtime::profile {

// No transport and no active apply: this coordinator handles only a validated
// staging transaction. A future authenticated transport may call it from ONE
// serialized owner task, after focused firmware and power-loss qualification.
// Persistence success never implies physical execution or active runtime state.
enum class IpoStagingStatus : std::uint8_t {
  Prepared, StoredNotApplied, InvalidJson, UnsupportedHardware,
  ProfileCorrupt, StorageUnavailable, GenerationConflict,
  NotPrepared, WriteFailed, ReadbackFailed
};
struct IpoStagePreview {
  IpoStagingStatus status{IpoStagingStatus::InvalidJson};
  std::uint32_t base_generation{0U};
  std::uint32_t candidate_generation{0U};
  std::uint32_t candidate_crc32{0U};
  bool valid_candidate{false};
  bool active_runtime_changed{false};
};

class IpoProfileStagingCoordinator final {
public:
  explicit IpoProfileStagingCoordinator(IpoProfileNvsJournal& journal) noexcept
      : journal_(journal) {}

  IpoStagePreview prepare(const char* json, std::size_t length) noexcept {
    cancel();
    IpoStagePreview preview{};
    const auto current = journal_.load();
    if (current.status == IpoJournalStatus::StorageUnavailable) {
      preview.status = IpoStagingStatus::StorageUnavailable;
      return preview;
    }
    if (current.status == IpoJournalStatus::Corrupt ||
        current.status == IpoJournalStatus::RecoveredFromBackup) {
      preview.status = IpoStagingStatus::ProfileCorrupt;
      return preview;
    }
    if (current.status != IpoJournalStatus::Ok &&
        current.status != IpoJournalStatus::NotFound) {
      preview.status = IpoStagingStatus::StorageUnavailable;
      return preview;
    }
    const std::uint32_t generation = current.status == IpoJournalStatus::Ok
                                         ? current.profile.generation : 0U;
    if (generation == std::numeric_limits<std::uint32_t>::max()) {
      preview.status = IpoStagingStatus::GenerationConflict;
      return preview;
    }
    const auto decoded = decodeIpoDeviceProfileJson(json, length, generation + 1U);
    if (!decoded.admitted) {
      preview.status = decoded.status == IpoJsonStatus::UnsupportedBoard
                           ? IpoStagingStatus::UnsupportedHardware
                           : IpoStagingStatus::InvalidJson;
      return preview;
    }
    EncodedIpoProfile encoded{};
    if (encodeIpoProfile(decoded.profile, encoded) != IpoProfileCodecStatus::Ok ||
        encoded.size == 0U) {
      preview.status = IpoStagingStatus::UnsupportedHardware;
      return preview;
    }
    prepared_ = decoded.profile;
    prepared_encoding_ = encoded;
    base_generation_ = generation;
    prepared_crc32_ = profileCrc32(encoded.bytes.data(), encoded.size);
    ready_ = true;

    preview.status = IpoStagingStatus::Prepared;
    preview.base_generation = generation;
    preview.candidate_generation = prepared_.generation;
    preview.candidate_crc32 = prepared_crc32_;
    preview.valid_candidate = true;
    return preview;
  }

  IpoStagingStatus commitStoredOnly(std::uint32_t expected_candidate_crc,
                                    std::uint32_t expected_base_generation) noexcept {
    if (!ready_) return IpoStagingStatus::NotPrepared;
    if (expected_candidate_crc != prepared_crc32_ ||
        expected_base_generation != base_generation_) {
      return IpoStagingStatus::GenerationConflict;
    }
    const IpoJournalStatus journal_status = journal_.commit(prepared_, base_generation_);
    if (journal_status != IpoJournalStatus::Ok) {
      cancel();
      if (journal_status == IpoJournalStatus::GenerationConflict)
        return IpoStagingStatus::GenerationConflict;
      if (journal_status == IpoJournalStatus::Corrupt ||
          journal_status == IpoJournalStatus::RecoveredFromBackup)
        return IpoStagingStatus::ProfileCorrupt;
      if (journal_status == IpoJournalStatus::StorageUnavailable)
        return IpoStagingStatus::StorageUnavailable;
      return IpoStagingStatus::WriteFailed;
    }
    const auto stored = journal_.load();
    if (stored.status != IpoJournalStatus::Ok ||
        stored.profile.generation != prepared_.generation) {
      cancel();
      return IpoStagingStatus::ReadbackFailed;
    }
    EncodedIpoProfile check{};
    if (encodeIpoProfile(stored.profile, check) != IpoProfileCodecStatus::Ok ||
        check.size != prepared_encoding_.size ||
        std::memcmp(check.bytes.data(), prepared_encoding_.bytes.data(), check.size) != 0) {
      cancel();
      return IpoStagingStatus::ReadbackFailed;
    }
    cancel();
    // Durable only. No change to RuntimeScheduleProfileOwner, no output intent.
    return IpoStagingStatus::StoredNotApplied;
  }

  void cancel() noexcept {
    prepared_ = {};
    prepared_encoding_ = {};
    prepared_crc32_ = 0U;
    base_generation_ = 0U;
    ready_ = false;
  }

  bool hasPreparedCandidate() const noexcept { return ready_; }
  // Read-only within the exclusive transaction owner's preview lifetime.
  const DurableIpoProfile* preparedProfile() const noexcept {
    return ready_ ? &prepared_ : nullptr;
  }

private:
  IpoProfileNvsJournal& journal_;
  DurableIpoProfile prepared_{};
  EncodedIpoProfile prepared_encoding_{};
  std::uint32_t prepared_crc32_{0U};
  std::uint32_t base_generation_{0U};
  bool ready_{false};
};

} // namespace growbox::app::climate_io::runtime::profile
