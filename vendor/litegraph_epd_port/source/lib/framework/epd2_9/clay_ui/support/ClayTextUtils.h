#pragma once

#include <clay/clay.h>

#include <cstddef>
#include <string>

namespace epd2_9::clay_text {

Clay_String makeStatic(const char* literal);
Clay_String makeDynamic(const char* text);
Clay_String makeDynamic(const char* text, std::size_t length);
inline Clay_String makeDynamic(const std::string& text) {
  return makeDynamic(text.c_str(), text.size());
}
bool hasText(const char* text);

}  // namespace epd2_9::clay_text
