#pragma once

#include "climate/display/DisplayDirtyRegion.h"
#include "climate/display/DisplayRuntime.h"
#include "growbox_clay_ui/PageRenderer.h"

#include <cstddef>

namespace growbox::app::climate_io::display {

// Worker-safe Clay half of the display transaction. The backend remains the
// single owner of the framebuffer and physical refresh. Clay receives only a
// caller-owned scratch arena and the currently open framebuffer bytes. The
// caller supplies a layout-derived physical transfer region; the renderer never
// reaches into SSD1680 hardware ownership.
template <typename Backend>
bool renderClayDisplayFrame(const DisplayRuntimeFrame& frame, const DisplayRegion& transfer_region,
                            void* clay_arena, std::size_t clay_arena_bytes, Backend& backend,
                            ::growbox::clay_ui::RenderSummary* summary = nullptr) noexcept {
  if (!frame.refreshRequired() || clay_arena == nullptr || clay_arena_bytes == 0U ||
      displayRegionEmpty(transfer_region)) {
    return false;
  }

  if (!backend.beginFrame(::growbox::clay_ui::kDisplayWidth, ::growbox::clay_ui::kDisplayHeight,
                          frame.page_model.warning, frame.refresh_kind)) {
    return false;
  }

  std::uint8_t* framebuffer = backend.framebufferData();
  if (framebuffer == nullptr || backend.framebufferBytes() < ::growbox::clay_ui::kFrameBytes ||
      !::growbox::clay_ui::renderPageToMonochrome(frame.page_model, framebuffer,
                                                  backend.framebufferBytes(), clay_arena,
                                                  clay_arena_bytes, summary) ||
      !backend.setTransferRegion(transfer_region) || !backend.endFrame()) {
    backend.cancelFrame();
    return false;
  }
  return true;
}

} // namespace growbox::app::climate_io::display
