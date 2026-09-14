#include "epd2_9/clay_ui/support/ClayTextUtils.h"

#include <cstring>

namespace epd2_9::clay_text {

Clay_String makeStatic(const char* literal) {
  Clay_String str{};
  const char* source = literal ? literal : "";
  str.isStaticallyAllocated = true;
  str.length = static_cast<int32_t>(std::strlen(source));
  str.chars = source;
  return str;
}

Clay_String makeDynamic(const char* text) {
  Clay_String str{};
  const char* source = text ? text : "";
  str.isStaticallyAllocated = false;
  str.length = static_cast<int32_t>(std::strlen(source));
  str.chars = source;
  return str;
}

Clay_String makeDynamic(const char* text, std::size_t length) {
  Clay_String str{};
  str.isStaticallyAllocated = false;
  str.length = static_cast<int32_t>(length);
  str.chars = text ? text : "";
  return str;
}

bool hasText(const char* text) {
  return text && text[0] != '\0';
}

}  // namespace epd2_9::clay_text
