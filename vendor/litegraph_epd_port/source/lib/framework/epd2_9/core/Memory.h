// Memory management helpers for EPD
#pragma once

#include <Arduino.h>
#include "hardware/psram/allocators/PSRAMUtils.h"

namespace epd2_9 {

inline void* psram_malloc(size_t size) {
  return PSRAMUtils::smartMalloc(size);
}

inline void* psram_calloc(size_t num, size_t size) {
  return PSRAMUtils::smartCalloc(num, size);
}

inline void* psram_realloc(void* ptr, size_t size) {
  return PSRAMUtils::smartRealloc(ptr, size);
}

inline void psram_free(void* ptr) {
  PSRAMUtils::smartFree(ptr);
}

size_t get_free_heap();
size_t get_free_psram();

} // namespace epd2_9
