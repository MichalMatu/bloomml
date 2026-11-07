#pragma once

#include "IpoBoardAdmission.h"
#include "IpoGraph.h"
#include "climate/runtime/schedule/RuntimeScheduleProfile.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

namespace growbox::app::climate_io::runtime::profile {

// Portable deterministic, length-bounded binary snapshot. No pointers, strings,
// heap, raw padded structs or arbitrary JSON. It is not an actuation authority.
// This codec is for the fully validated graph + day/night schedule, not v4/v5/v6
// inference tensors. Firmware admission is required before any persisted apply.
constexpr std::uint32_t kProfileMagic = 0x324F5049U; // IPO2 little endian
constexpr std::uint16_t kProfileWireVersion = 1U;
constexpr std::size_t kMaximumProfileWireBytes = 1536U;
static_assert(sizeof(float) == 4U, "IpoProfileWire requires IEEE-754 32-bit float");

struct DurableIpoProfile {
  ::growbox::ipo::Graph graph{};
  RuntimeScheduleProfile schedule{};
  std::uint32_t generation{1U};
};
struct EncodedIpoProfile {
  std::array<std::uint8_t, kMaximumProfileWireBytes> bytes{};
  std::size_t size{0U};
};
enum class IpoProfileCodecStatus : std::uint8_t {
  Ok, InvalidLength, InvalidMagic, UnsupportedVersion, ChecksumMismatch,
  InvalidGraph, InvalidSchedule, UnsupportedBoard
};

inline std::uint32_t profileCrc32(const std::uint8_t* data,
                                  std::size_t size) noexcept {
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (std::uint8_t bit = 0; bit < 8U; ++bit) {
      crc = (crc >> 1U) ^ ((crc & 1U) != 0U ? 0xEDB88320U : 0U);
    }
  }
  return ~crc;
}

class ProfileWriter final {
public:
  explicit ProfileWriter(EncodedIpoProfile& target) noexcept : target_(target) {
    target_ = {};
  }
  void u8(std::uint8_t x) noexcept {
    if (position_ >= target_.bytes.size()) { valid_ = false; return; }
    target_.bytes[position_++] = x;
  }
  void u16(std::uint16_t x) noexcept {
    u8(static_cast<std::uint8_t>(x));
    u8(static_cast<std::uint8_t>(x >> 8U));
  }
  void u32(std::uint32_t x) noexcept {
    for (std::uint8_t i = 0; i < 4U; ++i) u8(static_cast<std::uint8_t>(x >> (8U * i)));
  }
  void u64(std::uint64_t x) noexcept {
    for (std::uint8_t i = 0; i < 8U; ++i) u8(static_cast<std::uint8_t>(x >> (8U * i)));
  }
  void f32(float x) noexcept {
    std::uint32_t bits = 0U;
    std::memcpy(&bits, &x, sizeof(x));
    u32(bits);
  }
  std::size_t position() const noexcept { return position_; }
  bool valid() const noexcept { return valid_; }
  void finish() noexcept {
    target_.size = valid_ ? position_ : 0U;
  }
private:
  EncodedIpoProfile& target_;
  std::size_t position_{0U};
  bool valid_{true};
};

class ProfileReader final {
public:
  ProfileReader(const std::uint8_t* data, std::size_t size) noexcept : data_(data), size_(size) {}
  std::uint8_t u8() noexcept {
    if (position_ >= size_) { valid_ = false; return 0U; }
    return data_[position_++];
  }
  std::uint16_t u16() noexcept {
    std::uint16_t value = u8();
    value |= static_cast<std::uint16_t>(u8()) << 8U;
    return value;
  }
  std::uint32_t u32() noexcept {
    std::uint32_t value = 0U;
    for (std::uint8_t i = 0; i < 4U; ++i) value |= static_cast<std::uint32_t>(u8()) << (8U * i);
    return value;
  }
  std::uint64_t u64() noexcept {
    std::uint64_t value = 0U;
    for (std::uint8_t i = 0; i < 8U; ++i) value |= static_cast<std::uint64_t>(u8()) << (8U * i);
    return value;
  }
  float f32() noexcept {
    const std::uint32_t bits = u32();
    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
  }
  bool valid() const noexcept { return valid_; }
  std::size_t remaining() const noexcept { return size_ - position_; }
private:
  const std::uint8_t* data_;
  std::size_t size_;
  std::size_t position_{0U};
  bool valid_{true};
};

inline IpoProfileCodecStatus validateDurableIpoProfile(
    const DurableIpoProfile& snapshot) noexcept {
  if (snapshot.generation == 0U ||
      ::growbox::ipo::validateGraph(snapshot.graph) != ::growbox::ipo::GraphStatus::Ok)
    return IpoProfileCodecStatus::InvalidGraph;
  // Even if the process graph contains CO2-safety -> exhaust, disabling the
  // only physical exhaust capability would make that safety path ineffective.
  // The current CrowPanel adapter must reject such a stored candidate.
  if (!validateRuntimeScheduleProfile(snapshot.schedule) ||
      !snapshot.schedule.exhaust_fan_available)
    return IpoProfileCodecStatus::InvalidSchedule;
  if (!::growbox::ipo::assessCrowPanelReference(snapshot.graph).matches_compiled_board)
    return IpoProfileCodecStatus::UnsupportedBoard;
  return IpoProfileCodecStatus::Ok;
}

inline IpoProfileCodecStatus encodeIpoProfile(
    const DurableIpoProfile& snapshot, EncodedIpoProfile& encoded) noexcept {
  encoded = {};
  const auto admission = validateDurableIpoProfile(snapshot);
  if (admission != IpoProfileCodecStatus::Ok) return admission;
  const auto& g = snapshot.graph;
  const auto& p = snapshot.schedule;
  ProfileWriter w(encoded);
  w.u32(kProfileMagic);
  w.u16(kProfileWireVersion);
  w.u32(snapshot.generation);
  w.u8(g.device_count);
  w.u8(g.signal_count);
  w.u8(g.actuator_count);
  w.u8(g.process_count);
  w.u8(g.link_count);
  w.u16(p.version);
  w.u8(p.day_start_hour);
  w.u8(p.night_start_hour);
  w.u8(static_cast<std::uint8_t>(p.humidity_mode));
  w.u64(p.sensor_timeout_ms);
  for (const auto& t : {p.day, p.night}) {
    w.f32(t.temperature_c);
    w.f32(t.relative_humidity_pct);
    w.f32(t.air_vpd_kpa);
  }
  w.u8(static_cast<std::uint8_t>(p.exhaust_fan_available ? 1U : 0U));
  w.u8(static_cast<std::uint8_t>(p.humidifier_available ? 1U : 0U));
  for (std::size_t i = 0; i < g.device_count; ++i) {
    const auto& d = g.devices[i];
    w.u16(d.id);
    w.u8(static_cast<std::uint8_t>(d.driver));
    w.u32(d.physical_locator);
    w.u8(d.enabled ? 1U : 0U);
  }
  for (std::size_t i = 0; i < g.signal_count; ++i) {
    const auto& s = g.signals[i];
    w.u8(static_cast<std::uint8_t>(s.role));
    w.u16(s.device_id);
    w.u8(static_cast<std::uint8_t>(s.channel));
    w.u8(s.priority);
  }
  for (std::size_t i = 0; i < g.actuator_count; ++i) {
    const auto& a = g.actuators[i];
    w.u16(a.id);
    w.u16(a.device_id);
    w.u8(static_cast<std::uint8_t>(a.channel));
    w.u8(static_cast<std::uint8_t>(a.role));
    w.u8(a.safe_off ? 1U : 0U);
    w.u32(a.min_on_ms);
    w.u32(a.min_off_ms);
  }
  for (std::size_t i = 0; i < g.process_count; ++i) {
    const auto& p1 = g.processes[i];
    w.u16(p1.id);
    w.u8(static_cast<std::uint8_t>(p1.kind));
    w.u8(p1.enabled ? 1U : 0U);
  }
  for (std::size_t i = 0; i < g.link_count; ++i) {
    const auto& l = g.links[i];
    w.u16(l.process_id);
    w.u16(l.actuator_id);
    w.u8(static_cast<std::uint8_t>(l.effect));
  }
  if (!w.valid() || w.position() + sizeof(std::uint32_t) > kMaximumProfileWireBytes) {
    encoded = {};
    return IpoProfileCodecStatus::InvalidLength;
  }
  const std::uint32_t crc = profileCrc32(encoded.bytes.data(), w.position());
  w.u32(crc);
  w.finish();
  return w.valid() ? IpoProfileCodecStatus::Ok : IpoProfileCodecStatus::InvalidLength;
}

inline IpoProfileCodecStatus decodeIpoProfile(
    const std::uint8_t* data, std::size_t size,
    DurableIpoProfile& snapshot) noexcept {
  snapshot = {};
  if (data == nullptr || size < 4U + 2U + 4U + 5U + 2U + 1U + 1U + 1U + 8U + 24U + 2U + 4U ||
      size > kMaximumProfileWireBytes) return IpoProfileCodecStatus::InvalidLength;
  const std::uint32_t actual_crc = profileCrc32(data, size - 4U);
  const std::size_t crc_offset = size - 4U;
  ProfileReader trailer(data + crc_offset, 4U);
  if (trailer.u32() != actual_crc) return IpoProfileCodecStatus::ChecksumMismatch;

  ProfileReader r(data, size - 4U);
  if (r.u32() != kProfileMagic) return IpoProfileCodecStatus::InvalidMagic;
  if (r.u16() != kProfileWireVersion) return IpoProfileCodecStatus::UnsupportedVersion;
  DurableIpoProfile incoming{};
  incoming.generation = r.u32();
  auto& g = incoming.graph;
  g.device_count = r.u8();
  g.signal_count = r.u8();
  g.actuator_count = r.u8();
  g.process_count = r.u8();
  g.link_count = r.u8();
  if (g.device_count > ::growbox::ipo::kMaxDevices ||
      g.signal_count > ::growbox::ipo::kMaxSignalSources ||
      g.actuator_count > ::growbox::ipo::kMaxActuators ||
      g.process_count > ::growbox::ipo::kMaxProcesses ||
      g.link_count > ::growbox::ipo::kMaxLinks) return IpoProfileCodecStatus::InvalidLength;
  auto& p = incoming.schedule;
  p.version = r.u16();
  p.day_start_hour = r.u8();
  p.night_start_hour = r.u8();
  p.humidity_mode = static_cast<::growbox::climate::HumidityControlMode>(r.u8());
  p.sensor_timeout_ms = r.u64();
  p.day.temperature_c = r.f32();
  p.day.relative_humidity_pct = r.f32();
  p.day.air_vpd_kpa = r.f32();
  p.night.temperature_c = r.f32();
  p.night.relative_humidity_pct = r.f32();
  p.night.air_vpd_kpa = r.f32();
  const auto exhaust_available = r.u8();
  const auto humidifier_available = r.u8();
  if (exhaust_available > 1U || humidifier_available > 1U)
    return IpoProfileCodecStatus::InvalidSchedule;
  p.exhaust_fan_available = exhaust_available == 1U;
  p.humidifier_available = humidifier_available == 1U;
  for (std::size_t i = 0; i < g.device_count; ++i) {
    auto& d = g.devices[i];
    d.id = r.u16();
    d.driver = static_cast<::growbox::ipo::Driver>(r.u8());
    d.physical_locator = r.u32();
    const auto enabled = r.u8();
    if (enabled > 1U) return IpoProfileCodecStatus::InvalidGraph;
    d.enabled = enabled == 1U;
  }
  for (std::size_t i = 0; i < g.signal_count; ++i) {
    auto& s = g.signals[i];
    s.role = static_cast<::growbox::ipo::SignalRole>(r.u8());
    s.device_id = r.u16();
    s.channel = static_cast<::growbox::ipo::Channel>(r.u8());
    s.priority = r.u8();
  }
  for (std::size_t i = 0; i < g.actuator_count; ++i) {
    auto& a = g.actuators[i];
    a.id = r.u16();
    a.device_id = r.u16();
    a.channel = static_cast<::growbox::ipo::Channel>(r.u8());
    a.role = static_cast<::growbox::ipo::OutputRole>(r.u8());
    const auto safe = r.u8();
    if (safe > 1U) return IpoProfileCodecStatus::InvalidGraph;
    a.safe_off = safe == 1U;
    a.min_on_ms = r.u32();
    a.min_off_ms = r.u32();
  }
  for (std::size_t i = 0; i < g.process_count; ++i) {
    auto& process = g.processes[i];
    process.id = r.u16();
    process.kind = static_cast<::growbox::ipo::ProcessKind>(r.u8());
    const auto enabled = r.u8();
    if (enabled > 1U) return IpoProfileCodecStatus::InvalidGraph;
    process.enabled = enabled == 1U;
  }
  for (std::size_t i = 0; i < g.link_count; ++i) {
    auto& link = g.links[i];
    link.process_id = r.u16();
    link.actuator_id = r.u16();
    link.effect = static_cast<::growbox::ipo::Effect>(r.u8());
  }
  if (!r.valid() || r.remaining() != 0U) return IpoProfileCodecStatus::InvalidLength;
  const auto status = validateDurableIpoProfile(incoming);
  if (status != IpoProfileCodecStatus::Ok) return status;
  snapshot = incoming;
  return IpoProfileCodecStatus::Ok;
}

} // namespace growbox::app::climate_io::runtime::profile
