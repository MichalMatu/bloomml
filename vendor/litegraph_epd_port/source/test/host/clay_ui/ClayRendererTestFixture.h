#pragma once

#include <cassert>
#include <cstring>
#include <vector>

#include "epd2_9/clay_ui/render/ClayRenderer.h"

using epd2_9::ClayRenderConfig;
using epd2_9::ClayRenderer;
using epd2_9::ClayRenderTarget;

struct MockRenderTarget : public ClayRenderTarget {
  struct DrawCommand {
    enum Type { FillRect, DrawRect, DrawText } type;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint16_t color;
    int16_t clipX = 0;
    int16_t clipY = 0;
    int16_t clipW = 0;
    int16_t clipH = 0;
    const char* text = nullptr;
  };
  std::vector<DrawCommand> commands;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    commands.push_back({DrawCommand::FillRect, x, y, w, h, color, 0, 0, 0, 0, nullptr});
  }
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    commands.push_back({DrawCommand::DrawRect, x, y, w, h, color, 0, 0, 0, 0, nullptr});
  }
  void drawText(const char* text, int16_t length, int16_t x, int16_t y, uint16_t color,
                uint16_t fontId, uint16_t fontSize, int16_t boxWidth, int16_t boxHeight,
                int16_t clipX, int16_t clipY, int16_t clipW, int16_t clipH) override {
    (void)length; (void)fontId; (void)fontSize;
    commands.push_back({DrawCommand::DrawText, x, y, boxWidth, boxHeight, color,
                        clipX, clipY, clipW, clipH, text});
  }
  void setFont(uint16_t) override {}
  void reset() { commands.clear(); }
};

inline Clay_RenderCommand makeRectCommand(Clay_BoundingBox bbox, Clay_Color color) {
  Clay_RenderCommand cmd{};
  cmd.boundingBox = bbox;
  cmd.commandType = CLAY_RENDER_COMMAND_TYPE_RECTANGLE;
  cmd.renderData.rectangle.backgroundColor = color;
  cmd.renderData.rectangle.cornerRadius = {0, 0, 0, 0};
  return cmd;
}

inline Clay_RenderCommand makeBorderCommand(Clay_BoundingBox bbox, Clay_Color color, uint16_t borderWidth) {
  Clay_RenderCommand cmd{};
  cmd.boundingBox = bbox;
  cmd.commandType = CLAY_RENDER_COMMAND_TYPE_BORDER;
  cmd.renderData.border.color = color;
  cmd.renderData.border.cornerRadius = {0, 0, 0, 0};
  cmd.renderData.border.width.left = borderWidth;
  cmd.renderData.border.width.right = borderWidth;
  cmd.renderData.border.width.top = borderWidth;
  cmd.renderData.border.width.bottom = borderWidth;
  cmd.renderData.border.width.betweenChildren = 0;
  return cmd;
}

inline Clay_RenderCommand makeTextCommand(Clay_BoundingBox bbox, const char* text,
                                          uint16_t fontId, uint16_t fontSize, Clay_Color color) {
  Clay_RenderCommand cmd{};
  cmd.boundingBox = bbox;
  cmd.commandType = CLAY_RENDER_COMMAND_TYPE_TEXT;
  const int32_t length = static_cast<int32_t>(std::strlen(text));
  cmd.renderData.text.stringContents.length = length;
  cmd.renderData.text.stringContents.chars = text;
  cmd.renderData.text.stringContents.baseChars = text;
  cmd.renderData.text.textColor = color;
  cmd.renderData.text.fontId = fontId;
  cmd.renderData.text.fontSize = fontSize;
  cmd.renderData.text.letterSpacing = 0;
  cmd.renderData.text.lineHeight = fontSize;
  return cmd;
}

inline Clay_BoundingBox makeBBox(float x, float y, float w, float h) {
  Clay_BoundingBox bbox{};
  bbox.x = x; bbox.y = y; bbox.width = w; bbox.height = h;
  return bbox;
}
