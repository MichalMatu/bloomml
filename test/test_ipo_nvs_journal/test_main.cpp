#include "nvs.h"
#include "IpoBoardAdmission.h"
#include "climate/runtime/profile/IpoProfileNvsJournal.h"
#include "climate/runtime/schedule/RuntimeScheduleProfile.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
using Keyspace = std::map<std::string, Bytes>;
using Partition = std::map<std::string, Keyspace>;
struct Handle {
  std::string ns;
  Keyspace staged;
  bool writable{false};
};
Partition persistent;
std::map<nvs_handle_t, Handle> handles;
nvs_handle_t next_handle{1};
unsigned commit_counter{0};
unsigned fail_commit_number{0};
bool fail_after_persist{false};

void reset() {
  persistent.clear();
  handles.clear();
  next_handle = 1;
  commit_counter = 0;
  fail_commit_number = 0;
  fail_after_persist = false;
}
void reboot() {
  // Forget volatile NVS handles and uncommitted mutations. Only successfully
  // persisted pages survive the simulated power cut.
  handles.clear();
  commit_counter = 0;
}
void failCommit(unsigned ordinal, bool after_persist) {
  commit_counter = 0;
  fail_commit_number = ordinal;
  fail_after_persist = after_persist;
}
void persist(const Handle& h) {
  if (h.staged.empty()) persistent.erase(h.ns);
  else persistent[h.ns] = h.staged;
}
bool hasKey(const char* ns, const char* key) {
  const auto it = persistent.find(ns);
  return it != persistent.end() && it->second.count(key) != 0;
}
void corrupt(const char* ns, const char* key) {
  auto& bytes = persistent.at(ns).at(key);
  assert(bytes.size() > 12U);
  bytes[12] ^= 0x40U; // corrupt contents without fixing CRC
}
} // namespace

esp_err_t nvs_open(const char* ns, int mode, nvs_handle_t* handle) {
  if (ns == nullptr || handle == nullptr) return ESP_FAIL;
  if (mode == NVS_READONLY && persistent.count(ns) == 0U)
    return ESP_ERR_NVS_NOT_FOUND;
  const auto id = next_handle++;
  handles[id] = Handle{ns, persistent.count(ns) ? persistent.at(ns) : Keyspace{},
                       mode == NVS_READWRITE};
  *handle = id;
  return ESP_OK;
}
void nvs_close(nvs_handle_t handle) { handles.erase(handle); }
esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key,
                       void* value, std::size_t* length) {
  const auto it = handles.find(handle);
  if (it == handles.end() || key == nullptr || length == nullptr) return ESP_FAIL;
  const auto data = it->second.staged.find(key);
  if (data == it->second.staged.end()) return ESP_ERR_NVS_NOT_FOUND;
  if (value == nullptr) { *length = data->second.size(); return ESP_OK; }
  if (*length < data->second.size()) return ESP_ERR_NVS_INVALID_LENGTH;
  std::memcpy(value, data->second.data(), data->second.size());
  *length = data->second.size();
  return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key,
                       const void* value, std::size_t length) {
  const auto it = handles.find(handle);
  if (it == handles.end() || !it->second.writable || key == nullptr ||
      value == nullptr || length == 0U) return ESP_FAIL;
  const auto* bytes = static_cast<const std::uint8_t*>(value);
  it->second.staged[key] = Bytes(bytes, bytes + length);
  return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t handle, const char* key, std::uint8_t* value) {
  if (value == nullptr) return ESP_FAIL;
  std::size_t length = sizeof(*value);
  const auto status = nvs_get_blob(handle, key, value, &length);
  return status == ESP_OK && length != 1U ? ESP_FAIL : status;
}
esp_err_t nvs_set_u8(nvs_handle_t handle, const char* key, std::uint8_t value) {
  return nvs_set_blob(handle, key, &value, sizeof(value));
}
esp_err_t nvs_erase_all(nvs_handle_t handle) {
  const auto it = handles.find(handle);
  if (it == handles.end() || !it->second.writable) return ESP_FAIL;
  it->second.staged.clear();
  return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle) {
  const auto it = handles.find(handle);
  if (it == handles.end() || !it->second.writable) return ESP_FAIL;
  ++commit_counter;
  const bool fail = fail_commit_number != 0U &&
                    commit_counter == fail_commit_number;
  if (!fail || fail_after_persist) persist(it->second);
  return fail ? ESP_FAIL : ESP_OK;
}

namespace {
namespace profile = growbox::app::climate_io::runtime::profile;
namespace runtime = growbox::app::climate_io::runtime;
profile::DurableIpoProfile candidate(std::uint32_t generation) {
  profile::DurableIpoProfile c{};
  c.graph = growbox::ipo::makeCrowPanelMintReferenceGraph();
  c.schedule = runtime::makeMintRuntimeScheduleProfile();
  c.schedule.day.temperature_c += static_cast<float>(generation);
  c.generation = generation;
  assert(profile::validateDurableIpoProfile(c) == profile::IpoProfileCodecStatus::Ok);
  return c;
}
void seededGeneration1(profile::IpoProfileNvsJournal& journal) {
  assert(journal.commit(candidate(1), 0U) == profile::IpoJournalStatus::Ok);
  assert(journal.load().profile.generation == 1U);
}

void testStoreRebootAndIsolatedRollback() {
  reset();
  persistent["gb_outstate"]["keep"] = Bytes{42U, 77U};
  profile::IpoProfileNvsJournal journal;
  assert(journal.load().status == profile::IpoJournalStatus::NotFound);
  seededGeneration1(journal);
  const auto saved = journal.load();
  assert(saved.stored && saved.stored_fingerprint_valid && !saved.selected_backup);
  profile::EncodedIpoProfile encoded{};
  assert(profile::encodeIpoProfile(candidate(1), encoded) ==
         profile::IpoProfileCodecStatus::Ok);
  assert(saved.stored_record_fingerprint ==
         profile::profileCrc32(encoded.bytes.data(), encoded.size));
  reboot();
  assert(journal.load().profile.generation == 1U);
  assert(journal.rollbackTo(1U, 0U) == profile::IpoJournalStatus::Ok);
  reboot();
  assert(journal.load().status == profile::IpoJournalStatus::NotFound);
  assert(!hasKey("gb_ipoprof", "active"));
  assert(!hasKey("gb_ipoprof", "slot_a"));
  assert(!hasKey("gb_ipoprof", "slot_b"));
  assert(persistent.at("gb_outstate").at("keep") == (Bytes{42U, 77U}));
}
void testTwoPhaseBeforeFlipKeepsPrevious() {
  reset();
  profile::IpoProfileNvsJournal journal;
  seededGeneration1(journal);
  failCommit(2U, false); // second nvs_commit: selector not persisted
  assert(journal.commit(candidate(2), 1U) == profile::IpoJournalStatus::WriteFailed);
  reboot();
  const auto truth = journal.load();
  assert(truth.status == profile::IpoJournalStatus::Ok);
  assert(truth.profile.generation == 1U && !truth.selected_backup);
  assert(hasKey("gb_ipoprof", "slot_b"));
}
void testAfterFlipRequiresRollback() {
  reset();
  profile::IpoProfileNvsJournal journal;
  seededGeneration1(journal);
  failCommit(2U, true); // selector persisted; writer sees failure
  assert(journal.commit(candidate(2), 1U) == profile::IpoJournalStatus::WriteFailed);
  reboot();
  assert(journal.load().profile.generation == 2U);
  assert(journal.rollbackTo(2U, 1U) == profile::IpoJournalStatus::Ok);
  reboot();
  const auto truth = journal.load();
  assert(truth.status == profile::IpoJournalStatus::Ok);
  assert(truth.profile.generation == 1U && !truth.selected_backup);
}
void testInterruptedFirstEverWrite() {
  reset();
  profile::IpoProfileNvsJournal journal;
  failCommit(1U, false);
  assert(journal.commit(candidate(1), 0U) == profile::IpoJournalStatus::WriteFailed);
  reboot();
  assert(journal.load().status == profile::IpoJournalStatus::NotFound);

  reset();
  failCommit(1U, true); // orphaned data with no selector: fail-closed
  assert(journal.commit(candidate(1), 0U) == profile::IpoJournalStatus::WriteFailed);
  reboot();
  assert(journal.load().status == profile::IpoJournalStatus::Corrupt);
}
void testCorruptSelectionBackupAndBothSlots() {
  reset();
  profile::IpoProfileNvsJournal journal;
  seededGeneration1(journal);
  assert(journal.commit(candidate(2), 1U) == profile::IpoJournalStatus::Ok);
  corrupt("gb_ipoprof", "slot_b");
  reboot();
  const auto backup = journal.load();
  assert(backup.status == profile::IpoJournalStatus::RecoveredFromBackup);
  assert(backup.selected_backup && backup.stored && backup.profile.generation == 1U);
  assert(journal.commit(candidate(3), 2U) == profile::IpoJournalStatus::Corrupt);
  corrupt("gb_ipoprof", "slot_a");
  assert(journal.load().status == profile::IpoJournalStatus::Corrupt);
}
void testConflictAndRollbackGuards() {
  reset();
  profile::IpoProfileNvsJournal journal;
  seededGeneration1(journal);
  assert(journal.commit(candidate(2), 0U) == profile::IpoJournalStatus::GenerationConflict);
  assert(journal.rollbackTo(1U, 1U) == profile::IpoJournalStatus::GenerationConflict);
  assert(journal.rollbackTo(7U, 0U) == profile::IpoJournalStatus::GenerationConflict);
  assert(journal.load().profile.generation == 1U);
}
} // namespace

int main() {
  testStoreRebootAndIsolatedRollback();
  testTwoPhaseBeforeFlipKeepsPrevious();
  testAfterFlipRequiresRollback();
  testInterruptedFirstEverWrite();
  testCorruptSelectionBackupAndBothSlots();
  testConflictAndRollbackGuards();
  return 0;
}
