#pragma once

#include "IpoSignalSelector.h"
#include "climate/runtime/profile/IpoProfileNvsJournal.h"

namespace growbox::app::climate_io::runtime::profile {

// Human-readable evidence vocabulary shared by console and future
// observer-only panel. No storage reads, sensor I/O or control side effects.
constexpr const char* journalStatusName(IpoJournalStatus status) noexcept {
  switch (status) {
    case IpoJournalStatus::Ok: return "valid";
    case IpoJournalStatus::NotFound: return "not_found";
    case IpoJournalStatus::RecoveredFromBackup: return "backup_available";
    case IpoJournalStatus::StorageUnavailable: return "storage_unavailable";
    case IpoJournalStatus::Corrupt: return "corrupt";
    case IpoJournalStatus::InvalidCandidate: return "invalid_candidate";
    case IpoJournalStatus::GenerationConflict: return "generation_conflict";
    case IpoJournalStatus::WriteFailed: return "write_failed";
    case IpoJournalStatus::ReadbackFailed: return "readback_failed";
  }
  return "unknown";
}

constexpr const char* signalSelectionStatusName(
    ::growbox::ipo::SignalSelectionStatus status) noexcept {
  using ::growbox::ipo::SignalSelectionStatus;
  switch (status) {
    case SignalSelectionStatus::Ok: return "fresh";
    case SignalSelectionStatus::MissingRole: return "missing_role";
    case SignalSelectionStatus::NoFreshSource: return "missing_or_stale";
    case SignalSelectionStatus::InvalidGraph: return "invalid_graph";
    case SignalSelectionStatus::AmbiguousReading: return "ambiguous";
    case SignalSelectionStatus::Capacity: return "capacity";
  }
  return "unknown";
}

} // namespace growbox::app::climate_io::runtime::profile
