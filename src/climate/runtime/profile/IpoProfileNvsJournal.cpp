#include "climate/runtime/profile/IpoProfileNvsJournal.h"

#include <nvs.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace growbox::app::climate_io::runtime::profile {
namespace {

constexpr char kNamespace[] = "gb_ipoprof"; // distinct from output-policy namespace
constexpr char kActiveKey[] = "active";
constexpr char kSlotA[] = "slot_a";
constexpr char kSlotB[] = "slot_b";

const char* slotKey(std::uint8_t slot) noexcept {
  return slot == 0U ? kSlotA : kSlotB;
}

bool keyPresent(nvs_handle_t handle, const char* key) noexcept {
  std::size_t size = 0U;
  return nvs_get_blob(handle, key, nullptr, &size) == ESP_OK;
}

IpoProfileCodecStatus decodeSlot(nvs_handle_t handle, std::uint8_t slot,
                                 DurableIpoProfile& profile,
                                 std::uint32_t* fingerprint = nullptr) noexcept {
  profile = {};
  std::size_t size = 0U;
  if (nvs_get_blob(handle, slotKey(slot), nullptr, &size) != ESP_OK ||
      size == 0U || size > kMaximumProfileWireBytes) {
    return IpoProfileCodecStatus::InvalidLength;
  }
  EncodedIpoProfile bytes{};
  bytes.size = size;
  std::size_t read_size = size;
  if (nvs_get_blob(handle, slotKey(slot), bytes.bytes.data(), &read_size) != ESP_OK ||
      read_size != size) {
    return IpoProfileCodecStatus::InvalidLength;
  }
  const auto status = decodeIpoProfile(bytes.bytes.data(), bytes.size, profile);
  if (status == IpoProfileCodecStatus::Ok && fingerprint != nullptr) {
    // Cached at NVS boot read so status/diagnostics do not allocate or encode
    // a second large profile buffer on the service-console task stack.
    *fingerprint = profileCrc32(bytes.bytes.data(), bytes.size);
  }
  return status;
}

bool matchesEncoded(nvs_handle_t handle, std::uint8_t slot,
                    const EncodedIpoProfile& expected) noexcept {
  std::size_t size = 0U;
  if (nvs_get_blob(handle, slotKey(slot), nullptr, &size) != ESP_OK ||
      size != expected.size) return false;
  EncodedIpoProfile on_disk{};
  std::size_t read_size = size;
  if (nvs_get_blob(handle, slotKey(slot), on_disk.bytes.data(), &read_size) != ESP_OK ||
      read_size != size) return false;
  return std::memcmp(on_disk.bytes.data(), expected.bytes.data(), size) == 0;
}

} // namespace

IpoJournalReadResult IpoProfileNvsJournal::load() noexcept {
  IpoJournalReadResult result{};
  nvs_handle_t handle{};
  const esp_err_t open = nvs_open(kNamespace, NVS_READONLY, &handle);
  if (open == ESP_ERR_NVS_NOT_FOUND) {
    result.status = IpoJournalStatus::NotFound;
    return result;
  }
  if (open != ESP_OK) return result;

  std::uint8_t active = 0U;
  const esp_err_t marker = nvs_get_u8(handle, kActiveKey, &active);
  if (marker == ESP_ERR_NVS_NOT_FOUND) {
    // Distinguish a genuinely new namespace from a torn/corrupt transaction.
    result.status = !keyPresent(handle, kSlotA) && !keyPresent(handle, kSlotB)
                        ? IpoJournalStatus::NotFound : IpoJournalStatus::Corrupt;
    nvs_close(handle);
    return result;
  }
  if (marker != ESP_OK || active > 1U) {
    result.status = IpoJournalStatus::Corrupt;
    nvs_close(handle);
    return result;
  }

  DurableIpoProfile selected{};
  if (decodeSlot(handle, active, selected, &result.stored_record_fingerprint) ==
      IpoProfileCodecStatus::Ok) {
    result.status = IpoJournalStatus::Ok;
    result.profile = selected;
    result.stored = true;
    result.stored_fingerprint_valid = true;
    nvs_close(handle);
    return result;
  }

  // Never promote or overwrite corrupt state on read. Allow a known-good slot
  // to be reported for separate operator-approved recovery.
  DurableIpoProfile previous{};
  if (decodeSlot(handle, static_cast<std::uint8_t>(1U - active), previous,
                 &result.stored_record_fingerprint) == IpoProfileCodecStatus::Ok) {
    result.status = IpoJournalStatus::RecoveredFromBackup;
    result.profile = previous;
    result.stored = true;
    result.selected_backup = true;
    result.stored_fingerprint_valid = true;
  } else {
    result.status = IpoJournalStatus::Corrupt;
  }
  nvs_close(handle);
  return result;
}

IpoJournalStatus IpoProfileNvsJournal::commit(
    const DurableIpoProfile& candidate, std::uint32_t expected_generation) noexcept {
  if (validateDurableIpoProfile(candidate) != IpoProfileCodecStatus::Ok) {
    return IpoJournalStatus::InvalidCandidate;
  }

  const IpoJournalReadResult current = load();
  if (current.status != IpoJournalStatus::Ok &&
      current.status != IpoJournalStatus::NotFound) {
    return current.status == IpoJournalStatus::StorageUnavailable
               ? IpoJournalStatus::StorageUnavailable : IpoJournalStatus::Corrupt;
  }
  const std::uint32_t active_generation =
      current.status == IpoJournalStatus::Ok ? current.profile.generation : 0U;
  if (active_generation != expected_generation ||
      expected_generation == std::numeric_limits<std::uint32_t>::max() ||
      candidate.generation != expected_generation + 1U) {
    return IpoJournalStatus::GenerationConflict;
  }

  EncodedIpoProfile bytes{};
  if (encodeIpoProfile(candidate, bytes) != IpoProfileCodecStatus::Ok) {
    return IpoJournalStatus::InvalidCandidate;
  }

  nvs_handle_t handle{};
  if (nvs_open(kNamespace, NVS_READWRITE, &handle) != ESP_OK) {
    return IpoJournalStatus::StorageUnavailable;
  }
  std::uint8_t existing_slot = 1U;
  if (current.status == IpoJournalStatus::Ok) {
    const esp_err_t marker = nvs_get_u8(handle, kActiveKey, &existing_slot);
    if (marker != ESP_OK || existing_slot > 1U) {
      nvs_close(handle);
      return IpoJournalStatus::Corrupt;
    }
  }
  const std::uint8_t next_slot = static_cast<std::uint8_t>(1U - existing_slot);

  // Phase 1: write+commit inactive slot only. Active record still points at
  // known-good data. Verify exact bytes and codec before selector flip.
  esp_err_t error = nvs_set_blob(handle, slotKey(next_slot),
                                 bytes.bytes.data(), bytes.size);
  if (error == ESP_OK) error = nvs_commit(handle);
  if (error != ESP_OK) {
    nvs_close(handle);
    return IpoJournalStatus::WriteFailed;
  }
  DurableIpoProfile verified{};
  if (!matchesEncoded(handle, next_slot, bytes) ||
      decodeSlot(handle, next_slot, verified) != IpoProfileCodecStatus::Ok ||
      verified.generation != candidate.generation) {
    nvs_close(handle);
    return IpoJournalStatus::ReadbackFailed;
  }

  // Phase 2: flip selector. A power loss before this point keeps the previous
  // slot active; after commit, a read-back is mandatory for success.
  error = nvs_set_u8(handle, kActiveKey, next_slot);
  if (error == ESP_OK) error = nvs_commit(handle);
  if (error != ESP_OK) {
    nvs_close(handle);
    return IpoJournalStatus::WriteFailed;
  }
  std::uint8_t confirmed_slot = 0xffU;
  error = nvs_get_u8(handle, kActiveKey, &confirmed_slot);
  nvs_close(handle);
  if (error != ESP_OK || confirmed_slot != next_slot) {
    return IpoJournalStatus::ReadbackFailed;
  }
  // A positive journal commit means durable configuration bytes only, not
  // runtime activation, OutputSupervisor success or physical confirmation.
  return IpoJournalStatus::Ok;
}

} // namespace growbox::app::climate_io::runtime::profile
