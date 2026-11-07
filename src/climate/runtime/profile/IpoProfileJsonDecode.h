#pragma once

#include "climate/runtime/profile/IpoProfileWire.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::runtime::profile {

// Bounded operator JSON (24 devices / 16 signal roles / 48 links can exceed 8 KB).
// Parsed only in the privileged staging path, never in the one-second control tick.
// The JS exporter counts compact UTF-8 bytes against the same 32 KiB limit.
inline constexpr std::size_t kMaximumIpoProfileJsonBytes = 32768U;

enum class IpoJsonStatus : std::uint8_t {
  Ok, InvalidLength, InvalidJson, UnsupportedSchema, InvalidFields,
  InvalidReference, InvalidGraph, UnsupportedBoard
};

struct IpoJsonDecodeResult {
  IpoJsonStatus status{IpoJsonStatus::InvalidJson};
  DurableIpoProfile profile{};
  bool admitted{false};  // storage candidate only; NOT runtime apply authority
};

// Strict, fail-closed decoder from v2 operator JSON to a bounded *execution*
// snapshot. Runtime numeric ids are canonicalized; labels/ID strings remain
// external UI metadata and are not part of the binary execution snapshot.
// No NVS, RF, GPIO, ML mode switching or physical commands in this function.
IpoJsonDecodeResult decodeIpoDeviceProfileJson(const char* bytes,
                                                std::size_t byte_count,
                                                std::uint32_t target_generation) noexcept;

} // namespace growbox::app::climate_io::runtime::profile
