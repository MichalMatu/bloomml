#pragma once

#include <cstdint>

#include <clay/clay.h>

#include "epd2_9/clay_ui/render/ClipStack.h"

namespace epd2_9 {
struct ClayRenderConfig {
  uint16_t width;
  uint16_t height;
};

class ClayRenderTarget {
public:
  virtual ~ClayRenderTarget() = default;

  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
  virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
  virtual void drawText(const char* text,
                        int16_t length,
                        int16_t x,
                        int16_t y,
                        uint16_t color,
                        uint16_t fontId,
                        uint16_t fontSize,
                        int16_t boxWidth,
                        int16_t boxHeight,
                        int16_t clipX,
                        int16_t clipY,
                        int16_t clipW,
                        int16_t clipH) = 0;
  virtual void setFont(uint16_t fontId) = 0;
};

class ClayRenderer {
public:
  explicit ClayRenderer(ClayRenderTarget& target);

  void render(Clay_RenderCommandArray commands, const ClayRenderConfig& config);

private:
  ClayRenderTarget& _target;
  ClipStack _clipStack;

  bool isVisible(const Clay_RenderCommand& command) const;
  bool clipRectangle(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const;
  void handleRectangle(const Clay_RenderCommand& command);
  void handleBorder(const Clay_RenderCommand& command);
  void handleText(const Clay_RenderCommand& command);
};

Clay_Color toClayColor(uint32_t rgba);
uint16_t clayColorToMono(const Clay_Color& color);

}  // namespace epd2_9
