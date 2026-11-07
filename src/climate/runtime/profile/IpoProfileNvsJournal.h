#pragma once

#include "climate/runtime/profile/IpoProfileWire.h"

#include <cstdint>

namespace growbox::app::climate_io::runtime::profile {

// Separate namespace from OutputSupervisor output-state persistence.
// No global NVS init or erase here: RuntimeNvsOwner remains sole owner.
// This adapter is staged and NOT wired to boot/apply in this sprint.
enum class IpoJournalStatus : std::uint8_t {
  Ok, NotFound, RecoveredFromBackup, StorageUnavailable,
  Corrupt, InvalidCandidate, GenerationConflict, WriteFailed, ReadbackFailed
};

struct IpoJournalReadResult {
  IpoJournalStatus status{IpoJournalStatus::StorageUnavailable};
  DurableIpoProfile profile{};
  bool stored{false};
  bool selected_backup{false};
  std::uint32_t stored_record_fingerprint{0U}; // CRC32 of complete encoded record
  bool stored_fingerprint_valid{false};
};

class IpoProfileNvsJournal final {
public:
  IpoJournalReadResult load() noexcept;

  // Never a physical apply: stores an admitted, explicitly validated
  // candidate in the other slot, verifies it and only then flips selector.
  // Expected generation prevents stale overwrites. No output state mutation.
  IpoJournalStatus commit(const DurableIpoProfile& candidate,
                          std::uint32_t expected_generation) noexcept;
};

} // namespace growbox::app::climate_io::runtime::profile
