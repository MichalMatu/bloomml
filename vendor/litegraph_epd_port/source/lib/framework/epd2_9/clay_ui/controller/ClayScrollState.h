#pragma once

#include <algorithm>
#include <cstdint>

namespace epd2_9 {

class ClayScrollState {
 public:
  ClayScrollState();

  void reset();

  void configure(int32_t viewportHeight,
                 int32_t contentHeight,
                 int32_t minScroll = 0);

  void configureViewportOnly(int32_t viewportHeight, int32_t minScroll = 0);

  int32_t offset() const { return _offset; }
  int32_t maxScroll() const { return _maxScroll; }
  int32_t viewportHeight() const { return _viewportHeight; }
  int32_t contentHeight() const { return _contentHeight; }
  int32_t minScroll() const { return _minScroll; }

  void setOffset(int32_t offset);
  bool adjust(int32_t delta);

  void ensureVisible(int32_t top, int32_t bottom);

 private:
  int32_t _offset;
  int32_t _maxScroll;
  int32_t _viewportHeight;
  int32_t _contentHeight;
  int32_t _minScroll;
};

}  // namespace epd2_9

