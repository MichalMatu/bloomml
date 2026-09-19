#include "growbox_clay_ui/HostSimulator.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <clay/clay.h>

#include "ClayRenderer.h"

namespace growbox::clay_ui {
namespace {

constexpr std::size_t kMinimumArenaBytes = 128U * 1024U;
constexpr std::uint16_t kBlack = 0x0000;

class FrameTarget final : public internal::ClayRenderTarget {
public:
  explicit FrameTarget(MonochromeFrame& frame) : frame_(frame) {}

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    const int left = std::max<int>(0, x);
    const int top = std::max<int>(0, y);
    const int right = std::min<int>(kDisplayWidth, static_cast<int>(x) + w);
    const int bottom = std::min<int>(kDisplayHeight, static_cast<int>(y) + h);
    for (int py = top; py < bottom; ++py) {
      for (int px = left; px < right; ++px) {
        setPixel(static_cast<std::uint16_t>(px), static_cast<std::uint16_t>(py), color == kBlack);
      }
    }
  }

  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    if (w <= 0 || h <= 0) {
      return;
    }
    fillRect(x, y, w, 1, color);
    fillRect(x, static_cast<int16_t>(y + h - 1), w, 1, color);
    fillRect(x, y, 1, h, color);
    fillRect(static_cast<int16_t>(x + w - 1), y, 1, h, color);
  }

  void drawText(const char* text, int16_t length, int16_t x, int16_t y, uint16_t color,
                uint16_t fontId, uint16_t fontSize, int16_t boxWidth, int16_t boxHeight,
                int16_t clipX, int16_t clipY, int16_t clipW, int16_t clipH) override {
    (void)fontId;
    (void)fontSize;
    if (!text || length <= 0 || boxWidth <= 0 || boxHeight <= 0) {
      return;
    }
    const int right = x + boxWidth;
    const int bottom = y + boxHeight;
    int cursor = x;
    for (int16_t i = 0; i < length && cursor + 2 < right; ++i, cursor += 4) {
      if (text[i] == ' ') {
        continue;
      }
      int glyphX = cursor;
      int glyphY = y;
      int glyphW = 3;
      int glyphH = std::min<int>(5, bottom - y);
      if (clipW > 0 && clipH > 0) {
        const int clipRight = clipX + clipW;
        const int clipBottom = clipY + clipH;
        const int nx = std::max<int>(glyphX, clipX);
        const int ny = std::max<int>(glyphY, clipY);
        const int nr = std::min<int>(glyphX + glyphW, clipRight);
        const int nb = std::min<int>(glyphY + glyphH, clipBottom);
        glyphX = nx;
        glyphY = ny;
        glyphW = nr - nx;
        glyphH = nb - ny;
      }
      if (glyphW > 0 && glyphH > 0) {
        fillRect(static_cast<int16_t>(glyphX), static_cast<int16_t>(glyphY),
                 static_cast<int16_t>(glyphW), static_cast<int16_t>(glyphH), color);
      }
    }
  }

  void setFont(uint16_t) override {}

private:
  MonochromeFrame& frame_;

  void setPixel(std::uint16_t x, std::uint16_t y, bool black) {
    if (x >= kDisplayWidth || y >= kDisplayHeight) {
      return;
    }
    const std::size_t index = static_cast<std::size_t>(y) * kFrameStrideBytes + x / 8U;
    const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
    if (black) {
      frame_.bytes[index] |= mask;
    } else {
      frame_.bytes[index] &= static_cast<std::uint8_t>(~mask);
    }
  }
};

void handleClayError(Clay_ErrorData) {}

} // namespace

bool MonochromeFrame::pixel(std::uint16_t x, std::uint16_t y) const noexcept {
  if (x >= kDisplayWidth || y >= kDisplayHeight) {
    return false;
  }
  const std::size_t index = static_cast<std::size_t>(y) * kFrameStrideBytes + x / 8U;
  const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
  return (bytes[index] & mask) != 0U;
}

bool renderHostSmoke(MonochromeFrame& frame, RenderSummary* summary) noexcept {
  try {
    frame.bytes.fill(0U);
    Clay_SetMaxElementCount(256);
    Clay_SetMaxMeasureTextCacheWordCount(512);
    const std::size_t arenaBytes = std::max<std::size_t>(kMinimumArenaBytes, Clay_MinMemorySize());
    std::vector<std::uint8_t> arenaMemory(arenaBytes);
    Clay_Arena arena =
        Clay_CreateArenaWithCapacityAndMemory(arenaMemory.size(), arenaMemory.data());
    Clay_ErrorHandler handler{handleClayError, nullptr};
    Clay_Context* context = Clay_Initialize(
        arena,
        Clay_Dimensions{static_cast<float>(kDisplayWidth), static_cast<float>(kDisplayHeight)},
        handler);
    if (!context) {
      return false;
    }

    Clay_SetLayoutDimensions(
        Clay_Dimensions{static_cast<float>(kDisplayWidth), static_cast<float>(kDisplayHeight)});
    Clay_BeginLayout();
    CLAY(CLAY_ID("Phase1Root"),
         {.layout = {.sizing = {CLAY_SIZING_FIXED(static_cast<float>(kDisplayWidth)),
                                CLAY_SIZING_FIXED(static_cast<float>(kDisplayHeight))}},
          .backgroundColor = {255, 255, 255, 255}}) {
      CLAY(CLAY_ID("Phase1Clip"),
           {.layout = {.sizing = {CLAY_SIZING_FIXED(104), CLAY_SIZING_FIXED(64)}},
            .clip = {.horizontal = true, .vertical = true}}) {
        CLAY(CLAY_ID("Phase1Panel"),
             {.layout = {.sizing = {CLAY_SIZING_FIXED(132), CLAY_SIZING_FIXED(76)}},
              .backgroundColor = {0, 0, 0, 255}}) {}
      }
    }
    Clay_RenderCommandArray commands = Clay_EndLayout();

    FrameTarget target(frame);
    internal::ClayRenderer renderer(target);
    renderer.render(commands, internal::ClayRenderConfig{kDisplayWidth, kDisplayHeight});

    std::size_t blackPixels = 0;
    for (std::uint16_t y = 0; y < kDisplayHeight; ++y) {
      for (std::uint16_t x = 0; x < kDisplayWidth; ++x) {
        blackPixels += frame.pixel(x, y) ? 1U : 0U;
      }
    }
    if (summary) {
      summary->width = kDisplayWidth;
      summary->height = kDisplayHeight;
      summary->black_pixels = blackPixels;
      summary->render_commands = static_cast<std::size_t>(commands.length);
    }
    return commands.length > 0 && blackPixels > 0 && blackPixels < (kDisplayWidth * kDisplayHeight);
  } catch (...) {
    return false;
  }
}

bool writePbm(const MonochromeFrame& frame, const char* path) noexcept {
  if (!path) {
    return false;
  }
  std::FILE* file = std::fopen(path, "wb");
  if (!file) {
    return false;
  }
  const int header = std::fprintf(file, "P4\n%u %u\n", static_cast<unsigned>(kDisplayWidth),
                                  static_cast<unsigned>(kDisplayHeight));
  const std::size_t written = std::fwrite(frame.bytes.data(), 1, frame.bytes.size(), file);
  const int closed = std::fclose(file);
  return header > 0 && written == frame.bytes.size() && closed == 0;
}

} // namespace growbox::clay_ui
