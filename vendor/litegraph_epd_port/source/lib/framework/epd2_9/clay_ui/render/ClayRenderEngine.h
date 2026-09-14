#pragma once

#include <cstddef>
#include <cstdint>

#include <clay/clay.h>

namespace epd2_9 {

class ClayRenderEngine {
public:
  ClayRenderEngine();
  ~ClayRenderEngine();

  bool ensureInitialized(int displayWidth, int displayHeight);
  bool isInitialized() const { return _initialized; }

  void beginLayout(int width, int height);
  Clay_RenderCommandArray endLayout();
  void render(const Clay_RenderCommandArray& commands,
              bool forceFullRefresh = false);

private:
  size_t arenaSizeBytes() const;
  void releaseArena();

  static void handleErrors(Clay_ErrorData errorData);
  static Clay_Dimensions measureText(Clay_StringSlice text,
                                     Clay_TextElementConfig* config,
                                     void* userData);

  Clay_Arena _arena;
  void* _arenaBuffer;
  bool _hasLastPartialRegion;
  int16_t _lastPartialX;
  int16_t _lastPartialY;
  uint16_t _lastPartialW;
  uint16_t _lastPartialH;
  bool _initialized;
};

}  // namespace epd2_9
