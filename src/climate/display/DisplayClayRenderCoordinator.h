#pragma once

#include "climate/display/DisplayDirtyRegion.h"
#include "climate/display/DisplayRuntime.h"
#include "growbox_clay_ui/PageRenderer.h"

#include <cstddef>

namespace growbox::app::climate_io::display {

// Worker-safe Clay half of the display transaction. The backend remains the
// single owner of the framebuffer and physical refresh. Clay receives only a
// caller-owned scratch arena and the currently open framebuffer bytes.
template <typename Backend>
bool renderClayDisplayFrame(const DisplayRuntimeFrame& frame, void* clay_arena,
                            std::size_t clay_arena_bytes, Backend& backend,
                            ::growbox::clay_ui::RenderSummary* summary = nullptr) noexcept {
  if (!frame.refreshRequired() || clay_arena == nullptr || clay_arena_bytes == 0U) {
    return false;
  }

  if (!backend.beginFrame(::growbox::clay_ui::kDisplayWidth, ::growbox::clay_ui::kDisplayHeight,
                          frame.page_model.warning, frame.refresh_kind)) {
    return false;
  }

  ::growbox::clay_ui::RenderSummary local_summary{};
  auto* active_summary = summary != nullptr ? summary : &local_summary;
  std::uint8_t* framebuffer = backend.framebufferData();
  if (framebuffer == nullptr || backend.framebufferBytes() < ::growbox::clay_ui::kFrameBytes ||
      !::growbox::clay_ui::renderPageToMonochrome(frame.page_model, framebuffer,
                                                  backend.framebufferBytes(), clay_arena,
                                                  clay_arena_bytes, active_summary)) {
    backend.cancelFrame();
    return false;
  }

  const auto& content = active_summary->content_region;
  const DisplayRegion content_region{content.x_px, content.y_px, content.width_px,
                                     content.height_px};
  if (!backend.setContentRegion(content_region) || !backend.endFrame()) {
    backend.cancelFrame();
    return false;
  }
  return true;
}

} // namespace growbox::app::climate_io::display
