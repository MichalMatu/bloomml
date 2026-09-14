// Adafruit GFX Bitmap Font Renderer for SDL
// Provides pixel-perfect 1:1 rendering matching EPD display output

#pragma once

#include <SDL.h>
#include <cstdint>

#include "Adafruit_GFX.h"

extern const GFXfont FreeSans9pt7b;
extern const GFXfont FreeSans12pt7b;
extern const GFXfont FreeSans18pt7b;

namespace gfx_font {

void init();
const GFXfont* getFontById(uint32_t fontId);
void measureText(const char* text, int length, const GFXfont* font, int* width, int* height);
void getTextBounds(const char* text, int length, const GFXfont* font,
                   int* x, int* y, int* width, int* height);
void renderText(SDL_Renderer* renderer, const char* text, int length,
                int x, int y, int boxHeight, const GFXfont* font, SDL_Color color);
int getLineHeight(const GFXfont* font);

}  // namespace gfx_font
