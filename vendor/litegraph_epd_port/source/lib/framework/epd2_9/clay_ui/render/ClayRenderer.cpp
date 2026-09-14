#include "epd2_9/clay_ui/render/ClayRenderer.h"

namespace epd2_9 {

namespace {
constexpr uint16_t kColorWhite = 0xFFFF;
constexpr uint16_t kColorBlack = 0x0000;

inline bool isColorDark(const Clay_Color& color) {
  const int sum = static_cast<int>(color.r) + static_cast<int>(color.g) +
                  static_cast<int>(color.b);
  return sum < 3 * 128;
}

uint16_t clayColorToGx(const Clay_Color& color) {
  return isColorDark(color) ? kColorBlack : kColorWhite;
}

}  // namespace

ClayRenderer::ClayRenderer(ClayRenderTarget& target)
    : _target(target) {
  _clipStack.reset();
}

void ClayRenderer::render(Clay_RenderCommandArray commands,
                          const ClayRenderConfig& config) {
  (void)config;
  _clipStack.reset();
  for (int32_t i = 0; i < commands.length; ++i) {
    Clay_RenderCommand* command = Clay_RenderCommandArray_Get(&commands, i);
    if (!command) {
      continue;
    }
    switch (command->commandType) {
      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        handleRectangle(*command);
        break;
      case CLAY_RENDER_COMMAND_TYPE_BORDER:
        handleBorder(*command);
        break;
      case CLAY_RENDER_COMMAND_TYPE_TEXT:
        handleText(*command);
        break;
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        _clipStack.push(static_cast<int16_t>(command->boundingBox.x),
                        static_cast<int16_t>(command->boundingBox.y),
                        static_cast<int16_t>(command->boundingBox.width),
                        static_cast<int16_t>(command->boundingBox.height));
        break;
      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        _clipStack.pop();
        break;
      default:
        break;
    }
  }
}

void ClayRenderer::handleRectangle(const Clay_RenderCommand& command) {
  if (!isVisible(command)) {
    return;
  }
  const auto& rect = command.renderData.rectangle;
  const auto color = clayColorToGx(rect.backgroundColor);
  int16_t x = static_cast<int16_t>(command.boundingBox.x);
  int16_t y = static_cast<int16_t>(command.boundingBox.y);
  int16_t w = static_cast<int16_t>(command.boundingBox.width);
  int16_t h = static_cast<int16_t>(command.boundingBox.height);
  if (!clipRectangle(x, y, w, h)) {
    return;
  }
  _target.fillRect(x, y, w, h, color);
}

void ClayRenderer::handleBorder(const Clay_RenderCommand& command) {
  if (!isVisible(command)) {
    return;
  }
  const auto& border = command.renderData.border;
  const auto color = clayColorToGx(border.color);
  const auto x = static_cast<int16_t>(command.boundingBox.x);
  const auto y = static_cast<int16_t>(command.boundingBox.y);
  const auto w = static_cast<int16_t>(command.boundingBox.width);
  const auto h = static_cast<int16_t>(command.boundingBox.height);

  if (const auto* clip = _clipStack.current(); clip && clip->active) {
    const int16_t clipRight = clip->x + clip->w;
    const int16_t clipBottom = clip->y + clip->h;
    if (x < clip->x || y < clip->y || x + w > clipRight || y + h > clipBottom) {
      return;
    }
  }

  // Only handle uniform borders for now.
  if (border.width.left > 0 && border.width.left == border.width.right &&
      border.width.left == border.width.top &&
      border.width.left == border.width.bottom) {
    _target.drawRect(x, y, w, h, color);
  }
}

void ClayRenderer::handleText(const Clay_RenderCommand& command) {
  if (!isVisible(command)) {
    return;
  }
  const auto& text = command.renderData.text;
  const auto color = clayColorToGx(text.textColor);
  int16_t clipX = 0;
  int16_t clipY = 0;
  int16_t clipW = 0;
  int16_t clipH = 0;
  if (const auto* clip = _clipStack.current(); clip && clip->active) {
    clipX = clip->x;
    clipY = clip->y;
    clipW = clip->w;
    clipH = clip->h;
  }

  _target.setFont(text.fontId);
  _target.drawText(text.stringContents.chars,
                   static_cast<int16_t>(text.stringContents.length),
                   static_cast<int16_t>(command.boundingBox.x),
                   static_cast<int16_t>(command.boundingBox.y),
                   color,
                   text.fontId,
                   text.fontSize,
                   static_cast<int16_t>(command.boundingBox.width),
                   static_cast<int16_t>(command.boundingBox.height),
                   clipX,
                   clipY,
                   clipW,
                   clipH);
}

bool ClayRenderer::isVisible(const Clay_RenderCommand& command) const {
  const int16_t x = static_cast<int16_t>(command.boundingBox.x);
  const int16_t y = static_cast<int16_t>(command.boundingBox.y);
  const int16_t w = static_cast<int16_t>(command.boundingBox.width);
  const int16_t h = static_cast<int16_t>(command.boundingBox.height);
  return _clipStack.isVisible(x, y, w, h);
}

bool ClayRenderer::clipRectangle(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const {
  return _clipStack.clipRectangle(x, y, w, h);
}

Clay_Color toClayColor(uint32_t rgba) {
  Clay_Color color{};
  color.r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
  color.g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
  color.b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
  color.a = static_cast<uint8_t>(rgba & 0xFF);
  return color;
}

uint16_t clayColorToMono(const Clay_Color& color) {
  return clayColorToGx(color);
}

}  // namespace epd2_9
