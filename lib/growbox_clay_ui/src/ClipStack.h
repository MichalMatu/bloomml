#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace growbox::clay_ui::internal {

class ClipStack {
public:
  struct ClipRegion {
    bool active = false;
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
  };

  void reset() {
    _depth = 0;
  }

  void push(int16_t x, int16_t y, int16_t w, int16_t h) {
    ClipRegion region;
    region.active = (w > 0) && (h > 0);
    region.x = x;
    region.y = y;
    region.w = w;
    region.h = h;

    if (_depth > 0) {
      region = intersect(_stack[_depth - 1], region);
    }

    if (_depth < kMaxDepth) {
      _stack[_depth++] = region;
    } else {
      _stack[kMaxDepth - 1] = region;
    }
  }

  void pop() {
    if (_depth > 0) {
      --_depth;
    }
  }

  const ClipRegion* current() const {
    if (_depth == 0) {
      return nullptr;
    }
    return &_stack[_depth - 1];
  }

  bool clipRectangle(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const {
    const ClipRegion* clip = current();
    if (!clip || !clip->active) {
      return w > 0 && h > 0;
    }

    const int16_t right = x + w;
    const int16_t bottom = y + h;
    const int16_t clipRight = clip->x + clip->w;
    const int16_t clipBottom = clip->y + clip->h;

    const int16_t newX = std::max<int16_t>(x, clip->x);
    const int16_t newY = std::max<int16_t>(y, clip->y);
    const int16_t newRight = std::min<int16_t>(right, clipRight);
    const int16_t newBottom = std::min<int16_t>(bottom, clipBottom);

    if (newRight <= newX || newBottom <= newY) {
      return false;
    }

    x = newX;
    y = newY;
    w = newRight - newX;
    h = newBottom - newY;
    return true;
  }

  bool isVisible(int16_t x, int16_t y, int16_t w, int16_t h) const {
    if (w <= 0 || h <= 0) {
      return false;
    }
    const ClipRegion* clip = current();
    if (!clip || !clip->active) {
      return true;
    }
    const int16_t clipRight = clip->x + clip->w;
    const int16_t clipBottom = clip->y + clip->h;
    return (x < clipRight) && (x + w > clip->x) && (y < clipBottom) && (y + h > clip->y);
  }

private:
  static constexpr size_t kMaxDepth = 8;
  std::array<ClipRegion, kMaxDepth> _stack{};
  size_t _depth = 0;

  static ClipRegion intersect(const ClipRegion& a, const ClipRegion& b) {
    if (!a.active) {
      return b;
    }
    if (!b.active) {
      return a;
    }

    const int16_t newX = std::max<int16_t>(a.x, b.x);
    const int16_t newY = std::max<int16_t>(a.y, b.y);
    const int16_t newRight = std::min<int16_t>(a.x + a.w, b.x + b.w);
    const int16_t newBottom = std::min<int16_t>(a.y + a.h, b.y + b.h);

    ClipRegion result;
    if (newRight > newX && newBottom > newY) {
      result.active = true;
      result.x = newX;
      result.y = newY;
      result.w = static_cast<int16_t>(newRight - newX);
      result.h = static_cast<int16_t>(newBottom - newY);
    } else {
      result.active = false;
    }
    return result;
  }
};

} // namespace growbox::clay_ui::internal
