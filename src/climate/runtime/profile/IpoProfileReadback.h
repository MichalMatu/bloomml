#pragma once

#include "climate/runtime/profile/IpoProfileNvsJournal.h"
#include "climate/runtime/schedule/RuntimeScheduleProfile.h"

#include <cstdint>

namespace growbox::app::climate_io::runtime::profile {

// One observer-only projection, suitable for console/telemetry and a future
// local device status API. Never equate stored bytes with active control state.
struct IpoProfileReadback final {
  bool runtime_valid{false};
  bool active_is_builtin_mint{true};
  bool active_is_operator_v2{false};  // no v2 apply authority in this version
  std::uint32_t active_ram_generation{0U};
  std::uint32_t active_schedule_hash{0U};
  IpoJournalStatus journal_status{IpoJournalStatus::StorageUnavailable};
  bool stored_record_valid{false};
  bool stored_is_backup{false};
  std::uint32_t stored_generation{0U};
  std::uint32_t stored_record_fingerprint{0U};
  bool stored_fingerprint_valid{false};
};

inline IpoProfileReadback buildIpoProfileReadback(
    const RuntimeScheduleProfileOwner& active,
    const IpoJournalReadResult& store) noexcept {
  IpoProfileReadback result{};
  result.runtime_valid = active.valid();
  result.active_is_builtin_mint = active.bootDefaultMint();
  result.active_ram_generation = active.generation();
  if (result.runtime_valid) {
    result.active_schedule_hash = runtimeScheduleFingerprint(active.active());
  }
  result.journal_status = store.status;
  result.stored_record_valid = store.stored &&
      (store.status == IpoJournalStatus::Ok ||
       store.status == IpoJournalStatus::RecoveredFromBackup);
  result.stored_is_backup = result.stored_record_valid && store.selected_backup;
  if (result.stored_record_valid) {
    result.stored_generation = store.profile.generation;
    result.stored_fingerprint_valid = store.stored_fingerprint_valid;
    if (result.stored_fingerprint_valid) {
      result.stored_record_fingerprint = store.stored_record_fingerprint;
    }
  }
  // Critically: stored_record_valid is NEVER runtime-activation evidence.
  // The installed source is still Mint regardless of NVS contents.
  return result;
}

} // namespace growbox::app::climate_io::runtime::profile
