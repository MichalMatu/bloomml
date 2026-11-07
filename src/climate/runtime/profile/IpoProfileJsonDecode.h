#pragma once

#include "climate/runtime/profile/IpoProfileWire.h"

#include <cstddef>
#include <cstdint>

namespace growbox::app::climate_io::runtime::profile {

inline constexpr std::size_t kMaximumIpoProfileJsonBytes = 8192U;

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
