#include "GfxFontRenderer.h"

namespace gfx_font {
namespace {
const GFXfont* g_fonts[3] = {nullptr, nullptr, nullptr};
}

void init() {
  g_fonts[0] = &FreeSans9pt7b;
  g_fonts[1] = &FreeSans12pt7b;
  g_fonts[2] = &FreeSans18pt7b;
}

const GFXfont* getFontById(uint32_t fontId) {
  return fontId < 3 ? g_fonts[fontId] : g_fonts[0];
}

void getTextBounds(const char* text, int length, const GFXfont* font,
                   int* bx, int* by, int* bw, int* bh) {
  if (!font || !text || length <= 0) {
    *bx = *by = *bw = *bh = 0;
    return;
  }
  int minX = 0, minY = 0, maxX = 0, maxY = 0, cursorX = 0;
  bool first = true;
  for (int i = 0; i < length; ++i) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (c < font->first || c > font->last) continue;
    c -= font->first;
    const GFXglyph* glyph = &font->glyph[c];
    const int gx1 = cursorX + glyph->xOffset;
    const int gy1 = glyph->yOffset;
    const int gx2 = gx1 + glyph->width;
    const int gy2 = gy1 + glyph->height;
    if (first) {
      minX = gx1; minY = gy1; maxX = gx2; maxY = gy2; first = false;
    } else {
      if (gx1 < minX) minX = gx1;
      if (gy1 < minY) minY = gy1;
      if (gx2 > maxX) maxX = gx2;
      if (gy2 > maxY) maxY = gy2;
    }
    cursorX += glyph->xAdvance;
  }
  *bx = minX; *by = minY; *bw = maxX - minX; *bh = maxY - minY;
}

void measureText(const char* text, int length, const GFXfont* font, int* width, int* height) {
  if (!font || !text || length <= 0) {
    *width = 0; *height = 0; return;
  }
  int bx, by, bw, bh;
  getTextBounds(text, length, font, &bx, &by, &bw, &bh);
  *width = bw;
  *height = bh > 0 ? bh : font->yAdvance;
}

void renderText(SDL_Renderer* renderer, const char* text, int length,
                int x, int y, int boxHeight, const GFXfont* font, SDL_Color color) {
  if (!font || !text || length <= 0 || !renderer) return;
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  int bx, by, bw, bh;
  getTextBounds(text, length, font, &bx, &by, &bw, &bh);
  const int effectiveBoxHeight = boxHeight > 0 ? boxHeight : bh;
  const int verticalOffset = (effectiveBoxHeight - bh) / 2;
  const int baselineY = y + verticalOffset - by;
  int cursorX = x - bx;
  for (int i = 0; i < length; ++i) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (c < font->first || c > font->last) continue;
    c -= font->first;
    const GFXglyph* glyph = &font->glyph[c];
    const uint8_t* bitmap = font->bitmap;
    uint16_t bo = glyph->bitmapOffset;
    const uint8_t w = glyph->width;
    const uint8_t h = glyph->height;
    const int8_t xo = glyph->xOffset;
    const int8_t yo = glyph->yOffset;
    uint8_t bits = 0, bit = 0;
    for (uint8_t yy = 0; yy < h; ++yy) {
      for (uint8_t xx = 0; xx < w; ++xx) {
        if (!(bit++ & 7)) bits = bitmap[bo++];
        if (bits & 0x80) SDL_RenderDrawPoint(renderer, cursorX + xo + xx, baselineY + yo + yy);
        bits <<= 1;
      }
    }
    cursorX += glyph->xAdvance;
  }
}

int getLineHeight(const GFXfont* font) { return font ? font->yAdvance : 14; }

}  // namespace gfx_font
