#include "growbox_clay_ui/PageRenderer.h"

#include <algorithm>
#include <array>
#include <cstring>

#include <clay/clay.h>

#include "ClayRenderer.h"

namespace growbox::clay_ui {
namespace {

constexpr std::size_t kMinimumArenaBytes = 128U * 1024U;
constexpr std::size_t kArenaAlignmentSlackBytes = 64U;
constexpr std::uint16_t kBlack = 0x0000U;
constexpr std::uint16_t kNormalFontSize = 8U;
constexpr std::uint16_t kTitleFontSize = 14U;
constexpr int32_t kMaxElements = 256;
constexpr int32_t kMaxMeasuredWords = 512;
using Glyph = std::array<std::uint8_t, 7U>;

constexpr Glyph glyphFor(char input) noexcept {
  const char c = input >= 'a' && input <= 'z' ? static_cast<char>(input - 'a' + 'A') : input;
  switch (c) {
  case ' ':
    return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
  case '!':
    return {0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x00U, 0x04U};
  case '%':
    return {0x19U, 0x1AU, 0x02U, 0x04U, 0x08U, 0x0BU, 0x13U};
  case '-':
    return {0x00U, 0x00U, 0x00U, 0x1FU, 0x00U, 0x00U, 0x00U};
  case '.':
    return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x04U};
  case '/':
    return {0x01U, 0x02U, 0x02U, 0x04U, 0x08U, 0x08U, 0x10U};
  case ':':
    return {0x00U, 0x04U, 0x04U, 0x00U, 0x04U, 0x04U, 0x00U};
  case '>':
    return {0x10U, 0x08U, 0x04U, 0x02U, 0x04U, 0x08U, 0x10U};
  case '_':
    return {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU};
  case '0':
    return {0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU};
  case '1':
    return {0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
  case '2':
    return {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x08U, 0x1FU};
  case '3':
    return {0x1EU, 0x01U, 0x01U, 0x0EU, 0x01U, 0x01U, 0x1EU};
  case '4':
    return {0x02U, 0x06U, 0x0AU, 0x12U, 0x1FU, 0x02U, 0x02U};
  case '5':
    return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x01U, 0x01U, 0x1EU};
  case '6':
    return {0x0EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU};
  case '7':
    return {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x08U, 0x08U};
  case '8':
    return {0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU};
  case '9':
    return {0x0EU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x0EU};
  case 'A':
    return {0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
  case 'B':
    return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x11U, 0x11U, 0x1EU};
  case 'C':
    return {0x0EU, 0x11U, 0x10U, 0x10U, 0x10U, 0x11U, 0x0EU};
  case 'D':
    return {0x1EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x1EU};
  case 'E':
    return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU};
  case 'F':
    return {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U};
  case 'G':
    return {0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0FU};
  case 'H':
    return {0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U};
  case 'I':
    return {0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU};
  case 'J':
    return {0x07U, 0x02U, 0x02U, 0x02U, 0x12U, 0x12U, 0x0CU};
  case 'K':
    return {0x11U, 0x12U, 0x14U, 0x18U, 0x14U, 0x12U, 0x11U};
  case 'L':
    return {0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x1FU};
  case 'M':
    return {0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U};
  case 'N':
    return {0x11U, 0x19U, 0x19U, 0x15U, 0x13U, 0x13U, 0x11U};
  case 'O':
    return {0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
  case 'P':
    return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x10U, 0x10U};
  case 'Q':
    return {0x0EU, 0x11U, 0x11U, 0x11U, 0x15U, 0x12U, 0x0DU};
  case 'R':
    return {0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U};
  case 'S':
    return {0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU};
  case 'T':
    return {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U};
  case 'U':
    return {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU};
  case 'V':
    return {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U};
  case 'W':
    return {0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU};
  case 'X':
    return {0x11U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U, 0x11U};
  case 'Y':
    return {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U};
  case 'Z':
    return {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x1FU};
  default:
    return {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x00U, 0x04U};
  }
}

std::uint16_t textScale(std::uint16_t font_size) noexcept {
  return font_size >= 12U ? 2U : 1U;
}

class FrameTarget final : public internal::ClayRenderTarget {
public:
  explicit FrameTarget(std::uint8_t* framebuffer) noexcept : framebuffer_(framebuffer) {}

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
                uint16_t font_id, uint16_t font_size, int16_t box_width, int16_t box_height,
                int16_t clip_x, int16_t clip_y, int16_t clip_w, int16_t clip_h) override {
    (void)font_id;
    if (text == nullptr || length <= 0 || box_width <= 0 || box_height <= 0) {
      return;
    }

    const std::uint16_t scale = textScale(font_size);
    const int glyph_width = 5 * scale;
    const int glyph_height = 7 * scale;
    const int advance = 6 * scale;
    const int right = x + box_width;
    const int bottom = y + box_height;
    int cursor = x;

    for (int16_t i = 0; i < length && cursor + glyph_width <= right; ++i, cursor += advance) {
      const Glyph glyph = glyphFor(text[i]);
      for (std::size_t row = 0U; row < glyph.size(); ++row) {
        for (std::uint16_t column = 0U; column < 5U; ++column) {
          const std::uint8_t mask = static_cast<std::uint8_t>(1U << (4U - column));
          if ((glyph[row] & mask) == 0U) {
            continue;
          }

          int pixel_x = cursor + static_cast<int>(column * scale);
          int pixel_y = y + static_cast<int>(row * scale);
          int pixel_w = scale;
          int pixel_h = scale;
          if (pixel_y >= bottom || pixel_y + pixel_h > bottom || pixel_y >= y + glyph_height) {
            continue;
          }
          if (clip_w > 0 && clip_h > 0) {
            const int clip_right = clip_x + clip_w;
            const int clip_bottom = clip_y + clip_h;
            const int next_x = std::max(pixel_x, static_cast<int>(clip_x));
            const int next_y = std::max(pixel_y, static_cast<int>(clip_y));
            const int next_right = std::min(pixel_x + pixel_w, clip_right);
            const int next_bottom = std::min(pixel_y + pixel_h, clip_bottom);
            pixel_x = next_x;
            pixel_y = next_y;
            pixel_w = next_right - next_x;
            pixel_h = next_bottom - next_y;
          }
          if (pixel_w > 0 && pixel_h > 0) {
            fillRect(static_cast<int16_t>(pixel_x), static_cast<int16_t>(pixel_y),
                     static_cast<int16_t>(pixel_w), static_cast<int16_t>(pixel_h), color);
          }
        }
      }
    }
  }

  void setFont(uint16_t) override {}

private:
  std::uint8_t* framebuffer_{nullptr};

  void setPixel(std::uint16_t x, std::uint16_t y, bool black) noexcept {
    if (framebuffer_ == nullptr || x >= kDisplayWidth || y >= kDisplayHeight) {
      return;
    }
    const std::size_t index = static_cast<std::size_t>(y) * kFrameStrideBytes + x / 8U;
    const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
    if (black) {
      framebuffer_[index] |= mask;
    } else {
      framebuffer_[index] &= static_cast<std::uint8_t>(~mask);
    }
  }
};

void handleClayError(Clay_ErrorData error) noexcept {
  if (error.userData != nullptr) {
    *static_cast<bool*>(error.userData) = true;
  }
}

Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig* config, void*) noexcept {
  const std::uint16_t font_size = config != nullptr ? config->fontSize : kNormalFontSize;
  const std::uint16_t scale = textScale(font_size);
  const float width = text.length > 0 ? static_cast<float>(text.length * 6 * scale - scale) : 0.0F;
  return Clay_Dimensions{width, static_cast<float>(7U * scale)};
}

Clay_String clayString(const char* text) noexcept {
  return Clay_String{false, static_cast<int32_t>(std::strlen(text)), text};
}

template <std::size_t N> bool terminated(const std::array<char, N>& text) noexcept {
  return std::find(text.begin(), text.end(), '\0') != text.end();
}

bool validPage(const ::growbox::display_model::DisplayPageModel& page) noexcept {
  if (!terminated(page.title) || page.title[0] == '\0' || page.line_count == 0U ||
      page.line_count > page.lines.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < page.line_count; ++index) {
    if (!terminated(page.lines[index].label) || !terminated(page.lines[index].value) ||
        page.lines[index].label[0] == '\0') {
      return false;
    }
  }
  return true;
}

std::size_t blackPixelCount(const std::uint8_t* framebuffer) noexcept {
  std::size_t black_pixels = 0U;
  for (std::uint16_t y = 0U; y < kDisplayHeight; ++y) {
    for (std::uint16_t x = 0U; x < kDisplayWidth; ++x) {
      const std::size_t index = static_cast<std::size_t>(y) * kFrameStrideBytes + x / 8U;
      const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8U));
      black_pixels += (framebuffer[index] & mask) != 0U ? 1U : 0U;
    }
  }
  return black_pixels;
}

void configureClayLimits() noexcept {
  Clay_SetCurrentContext(nullptr);
  Clay_SetMaxElementCount(kMaxElements);
  Clay_SetMaxMeasureTextCacheWordCount(kMaxMeasuredWords);
}

void declarePageLayout(const ::growbox::display_model::DisplayPageModel& page) {
  const Clay_Color white{255, 255, 255, 255};
  const Clay_Color black{0, 0, 0, 255};
  const Clay_String title = clayString(page.title.data());

  CLAY(CLAY_ID("GrowboxPageRoot"),
       {.layout = {.sizing = {CLAY_SIZING_FIXED(static_cast<float>(kDisplayWidth)),
                              CLAY_SIZING_FIXED(static_cast<float>(kDisplayHeight))},
                   .padding = {6U, 6U, 5U, 5U},
                   .childGap = 2U,
                   .layoutDirection = CLAY_TOP_TO_BOTTOM},
        .backgroundColor = white}) {
    CLAY(CLAY_ID("GrowboxPageHeader"),
         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14.0F)},
                     .layoutDirection = CLAY_LEFT_TO_RIGHT}}) {
      CLAY(CLAY_ID("GrowboxPageTitle"),
           {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(14.0F)}}}) {
        CLAY_TEXT(title, CLAY_TEXT_CONFIG({.textColor = black,
                                           .fontId = 0U,
                                           .fontSize = kTitleFontSize,
                                           .wrapMode = CLAY_TEXT_WRAP_NONE}));
      }
      if (page.warning) {
        CLAY(CLAY_ID("GrowboxPageWarning"),
             {.layout = {.sizing = {CLAY_SIZING_FIXED(12.0F), CLAY_SIZING_FIXED(14.0F)},
                         .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER}}}) {
          CLAY_TEXT(CLAY_STRING("!"), CLAY_TEXT_CONFIG({.textColor = black,
                                                        .fontId = 0U,
                                                        .fontSize = kTitleFontSize,
                                                        .wrapMode = CLAY_TEXT_WRAP_NONE}));
        }
      }
    }

    CLAY(CLAY_ID("GrowboxPageSeparator"),
         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(1.0F)}},
          .backgroundColor = black}) {}

    CLAY(CLAY_ID("GrowboxPageRows"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                                 .childGap = 1U,
                                                 .layoutDirection = CLAY_TOP_TO_BOTTOM},
                                      .clip = {.vertical = true}}) {
      for (std::size_t index = 0U; index < page.line_count; ++index) {
        const auto& line = page.lines[index];
        const Clay_String label = clayString(line.label.data());
        const Clay_String value = clayString(line.value.data());
        CLAY(CLAY_IDI("GrowboxPageRow", static_cast<std::uint32_t>(index)),
             {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(9.0F)},
                         .childGap = 4U,
                         .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                         .layoutDirection = CLAY_LEFT_TO_RIGHT}}) {
          CLAY(CLAY_IDI("GrowboxPageLabel", static_cast<std::uint32_t>(index)),
               {.layout = {.sizing = {CLAY_SIZING_FIXED(72.0F), CLAY_SIZING_FIXED(8.0F)}}}) {
            CLAY_TEXT(label, CLAY_TEXT_CONFIG({.textColor = black,
                                               .fontId = 0U,
                                               .fontSize = kNormalFontSize,
                                               .wrapMode = CLAY_TEXT_WRAP_NONE}));
          }
          CLAY(CLAY_IDI("GrowboxPageValue", static_cast<std::uint32_t>(index)),
               {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(8.0F)}}}) {
            CLAY_TEXT(value, CLAY_TEXT_CONFIG({.textColor = black,
                                               .fontId = 0U,
                                               .fontSize = kNormalFontSize,
                                               .wrapMode = CLAY_TEXT_WRAP_NONE}));
          }
        }
      }
    }
  }
}

} // namespace

std::size_t pageRendererArenaBytes() noexcept {
  configureClayLimits();
  const std::size_t clay_minimum = Clay_MinMemorySize();
  Clay_SetCurrentContext(nullptr);
  return std::max(kMinimumArenaBytes, clay_minimum + kArenaAlignmentSlackBytes);
}

bool renderPageToMonochrome(const ::growbox::display_model::DisplayPageModel& page,
                            std::uint8_t* framebuffer, std::size_t framebuffer_bytes,
                            void* arena_memory, std::size_t arena_bytes,
                            RenderSummary* summary) noexcept {
  if (!validPage(page) || framebuffer == nullptr || framebuffer_bytes < kFrameBytes ||
      arena_memory == nullptr || arena_bytes < pageRendererArenaBytes()) {
    return false;
  }

  struct ContextReset final {
    ~ContextReset() {
      Clay_SetCurrentContext(nullptr);
    }
  };
  [[maybe_unused]] ContextReset context_reset{};

  std::fill_n(framebuffer, kFrameBytes, std::uint8_t{0U});
  configureClayLimits();

  Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(arena_bytes, arena_memory);
  bool clay_error = false;
  Clay_ErrorHandler handler{handleClayError, &clay_error};
  Clay_Context* context = Clay_Initialize(
      arena, Clay_Dimensions{static_cast<float>(kDisplayWidth), static_cast<float>(kDisplayHeight)},
      handler);
  if (context == nullptr) {
    return false;
  }

  Clay_SetMeasureTextFunction(measureText, nullptr);
  Clay_SetLayoutDimensions(
      Clay_Dimensions{static_cast<float>(kDisplayWidth), static_cast<float>(kDisplayHeight)});
  Clay_BeginLayout();
  declarePageLayout(page);
  Clay_RenderCommandArray commands = Clay_EndLayout();
  if (clay_error || commands.length <= 0) {
    return false;
  }

  FrameTarget target(framebuffer);
  internal::ClayRenderer renderer(target);
  renderer.render(commands, internal::ClayRenderConfig{kDisplayWidth, kDisplayHeight});

  const std::size_t black_pixels = blackPixelCount(framebuffer);
  if (summary != nullptr) {
    summary->width = kDisplayWidth;
    summary->height = kDisplayHeight;
    summary->black_pixels = black_pixels;
    summary->render_commands = static_cast<std::size_t>(commands.length);
  }
  return black_pixels > 0U && black_pixels < (kDisplayWidth * kDisplayHeight);
}

} // namespace growbox::clay_ui
