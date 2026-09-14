#include "epd2_9/clay_ui/controller/ClayScrollState.h"

namespace epd2_9 {

namespace {
constexpr int32_t kMinScrollDefault = 0;
}

ClayScrollState::ClayScrollState()
    : _offset(0),
      _maxScroll(0),
      _viewportHeight(0),
      _contentHeight(0),
      _minScroll(kMinScrollDefault) {}

void ClayScrollState::reset() {
  _offset = 0;
  _maxScroll = 0;
  _viewportHeight = 0;
  _contentHeight = 0;
  _minScroll = kMinScrollDefault;
}

void ClayScrollState::configure(int32_t viewportHeight,
                                int32_t contentHeight,
                                int32_t minScroll) {
  _viewportHeight = std::max<int32_t>(0, viewportHeight);
  _contentHeight = std::max<int32_t>(0, contentHeight);
  _minScroll = (minScroll <= 0) ? minScroll : kMinScrollDefault;
  if (_viewportHeight <= 0 || _contentHeight <= _viewportHeight) {
    _maxScroll = _minScroll;
  } else {
    _maxScroll = std::max<int32_t>(_minScroll,
                                   _contentHeight - _viewportHeight);
  }
  _offset = std::clamp(_offset, _minScroll, _maxScroll);
}

void ClayScrollState::configureViewportOnly(int32_t viewportHeight,
                                            int32_t minScroll) {
  configure(viewportHeight, viewportHeight, minScroll);
}

void ClayScrollState::setOffset(int32_t offset) {
  _offset = std::clamp(offset, _minScroll, _maxScroll);
}

bool ClayScrollState::adjust(int32_t delta) {
  const int32_t newOffset =
      std::clamp(_offset + delta, _minScroll, _maxScroll);
  if (newOffset == _offset) {
    return false;
  }
  _offset = newOffset;
  return true;
}

void ClayScrollState::ensureVisible(int32_t top, int32_t bottom) {
  if (_viewportHeight <= 0) {
    return;
  }

  if (top < _offset) {
    setOffset(top);
  } else if (bottom > _offset + _viewportHeight) {
    setOffset(bottom - _viewportHeight);
  }
}

}  // namespace epd2_9
