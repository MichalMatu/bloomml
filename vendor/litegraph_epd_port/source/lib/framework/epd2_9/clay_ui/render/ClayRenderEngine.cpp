#include "epd2_9/clay_ui/render/ClayRenderEngine.h"

#include <algorithm>
#include <cstring>
#include <string>

#include <esp_log.h>

#include "epd2_9/clay_ui/render/ClayDisplayTarget.h"
#include "epd2_9/clay_ui/render/ClayRenderer.h"
#include "epd2_9/core/Memory.h"
#include "epd2_9/drivers/display/DisplayDriver.h"
#include "epd2_9/fonts.h"

namespace epd2_9 {
namespace {

constexpr const char* TAG = "ClayRenderEngine";
constexpr int32_t kMaxClayElements = 256;
constexpr int32_t kMaxMeasureTextWords = 512;
constexpr size_t kMinimumArenaBytes = 128 * 1024;
constexpr int kPartialRefreshPadding = 16;

#ifndef CONFIG_CLAY_MEASURE_TEXT_LOG
#define CONFIG_CLAY_MEASURE_TEXT_LOG 0
#endif

const GFXfont* fontForClayId(uint16_t fontId) {
  switch (fontId) {
    case 1:
      return fontMedium();
    case 2:
      return fontLarge();
    case 0:
    default:
      return fontSmall();
  }
}

float fallbackFontHeight(uint16_t fontId) {
  switch (fontId) {
    case 1:
      return 18.0f;
    case 2:
      return 28.0f;
    case 0:
    default:
      return 12.0f;
  }
}

float fallbackWidthFactor(uint16_t fontId) {
  switch (fontId) {
    case 1:
      return 0.62f;
    case 2:
      return 0.66f;
    case 0:
    default:
      return 0.60f;
  }
}

int floorToInt(float value) {
  const int truncated = static_cast<int>(value);
  return value < static_cast<float>(truncated) ? truncated - 1 : truncated;
}

int ceilToInt(float value) {
  const int truncated = static_cast<int>(value);
  return value > static_cast<float>(truncated) ? truncated + 1 : truncated;
}

PartialRegion clampRegion(const PartialRegion& region, int displayWidth, int displayHeight) {
  const int left = std::max(0, std::min<int>(region.x, displayWidth));
  const int top = std::max(0, std::min<int>(region.y, displayHeight));
  const int right = std::max(left, std::min<int>(region.x + region.w, displayWidth));
  const int bottom = std::max(top, std::min<int>(region.y + region.h, displayHeight));
  return PartialRegion{
      static_cast<int16_t>(left),
      static_cast<int16_t>(top),
      static_cast<uint16_t>(right - left),
      static_cast<uint16_t>(bottom - top),
  };
}

PartialRegion unionRegions(const PartialRegion& lhs,
                           const PartialRegion& rhs,
                           int displayWidth,
                           int displayHeight) {
  const int left = std::min<int>(lhs.x, rhs.x);
  const int top = std::min<int>(lhs.y, rhs.y);
  const int right = std::max<int>(lhs.x + lhs.w, rhs.x + rhs.w);
  const int bottom = std::max<int>(lhs.y + lhs.h, rhs.y + rhs.h);
  return clampRegion(PartialRegion{
                         static_cast<int16_t>(left),
                         static_cast<int16_t>(top),
                         static_cast<uint16_t>(right - left),
                         static_cast<uint16_t>(bottom - top),
                     },
                     displayWidth,
                     displayHeight);
}

bool commandWritesVisiblePixels(const Clay_RenderCommand& command) {
  switch (command.commandType) {
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
      return clayColorToMono(command.renderData.rectangle.backgroundColor) != background_color();
    case CLAY_RENDER_COMMAND_TYPE_BORDER:
      return clayColorToMono(command.renderData.border.color) != background_color();
    case CLAY_RENDER_COMMAND_TYPE_TEXT:
    case CLAY_RENDER_COMMAND_TYPE_IMAGE:
      return true;
    default:
      return false;
  }
}

bool includeBounds(PartialRegion& bounds,
                   bool& hasBounds,
                   const Clay_BoundingBox& box,
                   int displayWidth,
                   int displayHeight) {
  int left = floorToInt(box.x) - kPartialRefreshPadding;
  int top = floorToInt(box.y) - kPartialRefreshPadding;
  int right = ceilToInt(box.x + box.width) + kPartialRefreshPadding;
  int bottom = ceilToInt(box.y + box.height) + kPartialRefreshPadding;

  left = std::max(0, std::min(left, displayWidth));
  top = std::max(0, std::min(top, displayHeight));
  right = std::max(left, std::min(right, displayWidth));
  bottom = std::max(top, std::min(bottom, displayHeight));
  if (right <= left || bottom <= top) {
    return false;
  }

  PartialRegion commandRegion = expand_partial_region(left, top, right - left, bottom - top);
  commandRegion = clampRegion(commandRegion, displayWidth, displayHeight);
  if (commandRegion.w == 0 || commandRegion.h == 0) {
    return false;
  }

  bounds = hasBounds ? unionRegions(bounds, commandRegion, displayWidth, displayHeight)
                     : commandRegion;
  hasBounds = true;
  return true;
}

bool calculateDirtyRegion(const Clay_RenderCommandArray& commands,
                          int displayWidth,
                          int displayHeight,
                          PartialRegion& region) {
  bool hasBounds = false;
  Clay_RenderCommandArray mutableCommands = commands;
  for (int32_t i = 0; i < commands.length; ++i) {
    Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&mutableCommands, i);
    if (!command || !commandWritesVisiblePixels(*command)) {
      continue;
    }
    includeBounds(region, hasBounds, command->boundingBox, displayWidth, displayHeight);
  }
  return hasBounds;
}

}  // namespace

ClayRenderEngine::ClayRenderEngine()
    : _arena{},
      _arenaBuffer(nullptr),
      _hasLastPartialRegion(false),
      _lastPartialX(0),
      _lastPartialY(0),
      _lastPartialW(0),
      _lastPartialH(0),
      _initialized(false) {}

ClayRenderEngine::~ClayRenderEngine() {
  releaseArena();
}

bool ClayRenderEngine::ensureInitialized(int displayWidth, int displayHeight) {
  if (_initialized) {
    return true;
  }

  const size_t arenaBytes = arenaSizeBytes();
  ESP_LOGI(TAG, "Allocating Clay arena (%u bytes)", static_cast<unsigned>(arenaBytes));

  _arenaBuffer = psram_malloc(arenaBytes);
  if (!_arenaBuffer) {
    ESP_LOGE(TAG, "Failed to allocate Clay arena (%u bytes)", static_cast<unsigned>(arenaBytes));
    return false;
  }

  _arena = Clay_CreateArenaWithCapacityAndMemory(arenaBytes, _arenaBuffer);

  Clay_Dimensions dims = {
      static_cast<float>(displayWidth),
      static_cast<float>(displayHeight),
  };
  Clay_ErrorHandler handler = {handleErrors, nullptr};
  Clay_Context* context = Clay_Initialize(_arena, dims, handler);
  if (!context) {
    ESP_LOGE(TAG, "Clay_Initialize failed");
    releaseArena();
    return false;
  }

  Clay_SetMaxElementCount(kMaxClayElements);
  Clay_SetMaxMeasureTextCacheWordCount(kMaxMeasureTextWords);
  Clay_SetMeasureTextFunction(measureText, this);

  _initialized = true;
  ESP_LOGI(TAG, "Clay arena ready (bytes=%u)", static_cast<unsigned>(arenaBytes));
  return true;
}

void ClayRenderEngine::beginLayout(int width, int height) {
  Clay_SetLayoutDimensions({
      static_cast<float>(width),
      static_cast<float>(height),
  });
  Clay_BeginLayout();
}

Clay_RenderCommandArray ClayRenderEngine::endLayout() {
  return Clay_EndLayout();
}

void ClayRenderEngine::render(const Clay_RenderCommandArray& commands,
                              bool forceFullRefresh) {
  if (!is_display_ready()) {
    return;
  }

  auto& surface = display_surface();
  auto drawFn = [&]() {
    surface.fillScreen(background_color());
    ClayDisplayTarget target;
    ClayRenderer renderer(target);
    ClayRenderConfig config{
        .width = static_cast<uint16_t>(surface.width()),
        .height = static_cast<uint16_t>(surface.height()),
    };
    renderer.render(commands, config);
  };

  if (forceFullRefresh) {
    draw_full(drawFn);
    PartialRegion currentRegion{};
    _hasLastPartialRegion =
        calculateDirtyRegion(commands, surface.width(), surface.height(), currentRegion);
    if (_hasLastPartialRegion) {
      _lastPartialX = currentRegion.x;
      _lastPartialY = currentRegion.y;
      _lastPartialW = currentRegion.w;
      _lastPartialH = currentRegion.h;
    }
  } else {
    PartialRegion currentRegion{};
    const bool hasCurrentRegion =
        calculateDirtyRegion(commands, surface.width(), surface.height(), currentRegion);
    PartialRegion dirtyRegion = hasCurrentRegion
                                    ? currentRegion
                                    : PartialRegion{
                                          _lastPartialX,
                                          _lastPartialY,
                                          _lastPartialW,
                                          _lastPartialH,
                                      };
    if (_hasLastPartialRegion) {
      PartialRegion lastRegion{
          _lastPartialX,
          _lastPartialY,
          _lastPartialW,
          _lastPartialH,
      };
      dirtyRegion = hasCurrentRegion
                        ? unionRegions(currentRegion, lastRegion, surface.width(), surface.height())
                        : lastRegion;
    }

    if (dirtyRegion.w > 0 && dirtyRegion.h > 0) {
      draw_partial(dirtyRegion.x, dirtyRegion.y, dirtyRegion.w, dirtyRegion.h, drawFn);
    }

    _hasLastPartialRegion = hasCurrentRegion;
    if (_hasLastPartialRegion) {
      _lastPartialX = currentRegion.x;
      _lastPartialY = currentRegion.y;
      _lastPartialW = currentRegion.w;
      _lastPartialH = currentRegion.h;
    } else {
      _lastPartialX = 0;
      _lastPartialY = 0;
      _lastPartialW = 0;
      _lastPartialH = 0;
    }
  }
}

size_t ClayRenderEngine::arenaSizeBytes() const {
  Clay_SetMaxElementCount(kMaxClayElements);
  Clay_SetMaxMeasureTextCacheWordCount(kMaxMeasureTextWords);
  size_t required = Clay_MinMemorySize();
  return required < kMinimumArenaBytes ? kMinimumArenaBytes : required;
}

void ClayRenderEngine::releaseArena() {
  if (_arenaBuffer) {
    psram_free(_arenaBuffer);
    _arenaBuffer = nullptr;
  }
  _initialized = false;
}

void ClayRenderEngine::handleErrors(Clay_ErrorData errorData) {
  ESP_LOGE(TAG, "Clay error: %.*s",
           static_cast<int>(errorData.errorText.length),
           errorData.errorText.chars);
}

Clay_Dimensions ClayRenderEngine::measureText(Clay_StringSlice text,
                                              Clay_TextElementConfig* config,
                                              void* userData) {
  (void)userData;
  const uint16_t fontId = config ? config->fontId : 0;
  const GFXfont* font = fontForClayId(fontId);

  Clay_Dimensions dims{};
  if (text.length <= 0) {
    dims.width = 0.0f;
    dims.height = fallbackFontHeight(fontId);
    return dims;
  }

  if (!is_display_ready()) {
    const float fontHeight = fallbackFontHeight(fontId);
    dims.width = fontHeight * fallbackWidthFactor(fontId) * text.length;
    dims.height = fontHeight;
    return dims;
  }

  const uint16_t lineHeightOverride = config ? config->lineHeight : 0;
  const uint16_t letterSpacing = config ? config->letterSpacing : 0;

  auto& surface = display_surface();
  surface.setFont(font);
  surface.setTextSize(1);

  int16_t refY1 = 0;
  int16_t refX1 = 0;
  uint16_t refH = 0;
  uint16_t refW = 0;
  surface.getTextBounds("Ag", 0, 0, &refX1, &refY1, &refW, &refH);
  (void)refX1;
  (void)refW;
  (void)refY1;
  const float defaultLineHeight =
      refH > 0 ? static_cast<float>(refH) : fallbackFontHeight(fontId);

  float maxWidth = 0.0f;
  float totalHeight = 0.0f;
  const char* base = text.chars;
  const int32_t totalLength = text.length;
  int32_t startIndex = 0;
  std::string scratch;
  if (totalLength > 0) {
    scratch.reserve(static_cast<size_t>(std::min<int32_t>(totalLength, 64)));
  }

  while (startIndex <= totalLength) {
    int32_t endIndex = -1;
    if (startIndex < totalLength) {
      const int32_t span = totalLength - startIndex;
      if (const char* newline =
              static_cast<const char*>(memchr(base + startIndex, '\n', span))) {
        endIndex = static_cast<int32_t>(newline - base);
      }
    }

    if (endIndex < 0) {
      const int32_t remaining = totalLength - startIndex;
      if (remaining > 0) {
        scratch.assign(base + startIndex, static_cast<size_t>(remaining));
      } else {
        scratch.assign(1, ' ');
      }
    } else {
      const int32_t segmentLen = endIndex - startIndex;
      if (segmentLen > 0) {
        scratch.assign(base + startIndex, static_cast<size_t>(segmentLen));
      } else {
        scratch.assign(1, ' ');
      }
    }

    int16_t x1, y1;
    uint16_t w, h;
    surface.getTextBounds(scratch.c_str(), 0, 0, &x1, &y1, &w, &h);

#if CONFIG_CLAY_MEASURE_TEXT_LOG
    ESP_LOGD(TAG,
             "measureText line='%.*s' w=%u h=%u y1=%d",
             static_cast<int>(scratch.size()), scratch.c_str(),
             static_cast<unsigned>(w),
             static_cast<unsigned>(h),
             static_cast<int>(y1));
#endif

    float lineWidth = static_cast<float>(w);
    if (letterSpacing > 0 && scratch.length() > 1) {
      lineWidth += static_cast<float>(letterSpacing * (scratch.length() - 1));
    }
    maxWidth = std::max(maxWidth, lineWidth);

    float effectiveLineHeight = h > 0 ? static_cast<float>(h) : defaultLineHeight;
    if (lineHeightOverride > 0) {
      effectiveLineHeight = static_cast<float>(lineHeightOverride);
    }
    totalHeight += effectiveLineHeight;

    if (endIndex < 0) {
      break;
    }
    startIndex = endIndex + 1;
  }

  surface.setTextSize(1);

  dims.width = maxWidth;
  dims.height = totalHeight;
  return dims;
}

}  // namespace epd2_9
