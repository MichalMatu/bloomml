#include "epd2_9/clay_ui/controller/ClayLayoutController.h"

#include "epd2_9/clay_ui/modules/menu/model/MenuModel.h"

namespace epd2_9 {
namespace {

std::size_t wrapHomeScreenIndex(int index, std::size_t screenCount) {
  const int count = screenCount > 0 ? static_cast<int>(screenCount) : 1;
  const int mod = index % count;
  const int wrapped = mod < 0 ? mod + count : mod;
  return static_cast<std::size_t>(wrapped);
}

}  // namespace

ClayLayoutController::ClayLayoutController()
    : _viewMode(ClayViewMode::Home),
      _selectedIndex(0),
      _menuScroll(),
      _detailScroll(),
      _menuSelectedIndex(0),
      _menuScrollSnapshot(0),
      _activeHomeScreenIndex(0),
      _homeScreenCount(1),
      _systemSettingsScrollStep(kDefaultSystemSettingsScrollStep),
      _viewStack{},
      _viewStackSize(0) {}

int ClayLayoutController::wrapIndex(int index, int itemCount) {
  if (itemCount <= 0) {
    return 0;
  }
  const int mod = index % itemCount;
  return mod < 0 ? mod + itemCount : mod;
}

void ClayLayoutController::reset() {
  _viewMode = ClayViewMode::Home;
  _selectedIndex = 0;
  _menuScroll.reset();
  _detailScroll.reset();
  _menuSelectedIndex = 0;
  _menuScrollSnapshot = 0;
  _activeHomeScreenIndex = 0;
  _homeScreenCount = 1;
  _systemSettingsScrollStep = kDefaultSystemSettingsScrollStep;
  _viewStackSize = 0;
}

bool ClayLayoutController::handleInput(const InputEvent& ev,
                                       int menuItemCount) {
  switch (_viewMode) {
    case ClayViewMode::Home:
      return layout_handlers::handleHomeInput(*this, ev);
    case ClayViewMode::Menu:
      return layout_handlers::handleMenuInput(*this, ev, menuItemCount);
    case ClayViewMode::WifiOverview:
      return layout_handlers::handleWifiOverviewInput(*this, ev);
    case ClayViewMode::WifiStatus:
      return layout_handlers::handleWifiStatusInput(*this, ev);
    case ClayViewMode::AutomationSettings:
      return layout_handlers::handleAutomationSettingsInput(*this, ev);
    case ClayViewMode::SetTime:
      return layout_handlers::handleSetTimeInput(*this, ev);
    case ClayViewMode::SystemSettings:
      return layout_handlers::handleSystemSettingsInput(*this, ev);
    case ClayViewMode::WifiProfileSettings:
      return layout_handlers::handleWifiProfileSettingsInput(*this, ev);
  }
  return false;
}

bool ClayLayoutController::openMenu(bool restoreSelection) {
  if (_viewMode == ClayViewMode::Menu) {
    return false;
  }
  _viewMode = ClayViewMode::Menu;
  _viewStackSize = 0;
  _detailScroll.setOffset(0);
  if (restoreSelection) {
    restoreMenuState();
  }
  return true;
}

bool ClayLayoutController::openSelectedMenuItem(int menuItemCount) {
  if (_viewMode != ClayViewMode::Menu || menuItemCount <= 0) {
    return false;
  }

  const auto& items = clay_menu::menuItems();
  if (_selectedIndex < 0 ||
      _selectedIndex >= static_cast<int>(items.size()) ||
      _selectedIndex >= menuItemCount) {
    return false;
  }

  return openViewFromMenu(items[static_cast<size_t>(_selectedIndex)].targetView);
}

bool ClayLayoutController::returnToMenu(bool restoreSelection) {
  if (_viewMode == ClayViewMode::Menu) {
    return false;
  }

  while (_viewStackSize > 0) {
    ViewStackEntry previous{};
    if (!popViewState(previous)) {
      break;
    }

    if (previous.mode == ClayViewMode::Menu) {
      _viewMode = ClayViewMode::Menu;
      _detailScroll.setOffset(0);
      if (restoreSelection) {
        restoreMenuState();
      }
      return true;
    }
  }

  if (!openMenu(restoreSelection)) {
    return false;
  }

  _detailScroll.setOffset(0);
  return true;
}

bool ClayLayoutController::returnToPreviousView() {
  if (_viewMode == ClayViewMode::Home) {
    return false;
  }

  ViewStackEntry previous{};
  if (!popViewState(previous)) {
    return false;
  }

  _viewMode = previous.mode;
  if (_viewMode == ClayViewMode::Menu) {
    restoreMenuState();
    _detailScroll.setOffset(0);
  } else {
    _detailScroll.setOffset(previous.detailScrollOffset);
  }
  return true;
}

bool ClayLayoutController::returnHome() {
  if (_viewMode == ClayViewMode::Home) {
    return false;
  }
  if (_viewMode == ClayViewMode::Menu) {
    rememberMenuState();
  }
  _viewMode = ClayViewMode::Home;
  _viewStackSize = 0;
  _detailScroll.setOffset(0);
  return true;
}

bool ClayLayoutController::openView(ClayViewMode targetView) {
  if (targetView == ClayViewMode::Menu || targetView == ClayViewMode::Home) {
    return false;
  }
  if (targetView == _viewMode) {
    return false;
  }
  if (!pushViewState(_viewMode, _detailScroll.offset())) {
    return false;
  }
  if (_viewMode == ClayViewMode::Menu) {
    rememberMenuState();
  }

  _detailScroll.setOffset(0);
  _viewMode = targetView;
  return true;
}

bool ClayLayoutController::adjustDetailScroll(int32_t delta) {
  if (delta == 0) {
    return false;
  }
  return _detailScroll.adjust(delta);
}

bool ClayLayoutController::nextHomeScreen() {
  if (_viewMode != ClayViewMode::Home) {
    return false;
  }

  const std::size_t next =
      wrapHomeScreenIndex(static_cast<int>(_activeHomeScreenIndex) + 1, _homeScreenCount);
  if (next == _activeHomeScreenIndex) {
    return false;
  }

  _activeHomeScreenIndex = next;
  return true;
}

bool ClayLayoutController::previousHomeScreen() {
  if (_viewMode != ClayViewMode::Home) {
    return false;
  }

  const std::size_t previous =
      wrapHomeScreenIndex(static_cast<int>(_activeHomeScreenIndex) - 1, _homeScreenCount);
  if (previous == _activeHomeScreenIndex) {
    return false;
  }

  _activeHomeScreenIndex = previous;
  return true;
}

void ClayLayoutController::setHomeScreenCount(std::size_t count) {
  _homeScreenCount = count > 0 ? count : 1;
  if (_activeHomeScreenIndex >= _homeScreenCount) {
    _activeHomeScreenIndex = 0;
  }
}

ClayViewMode ClayLayoutController::viewMode() const {
  return _viewMode;
}

std::size_t ClayLayoutController::activeHomeScreenIndex() const {
  return _activeHomeScreenIndex;
}

int ClayLayoutController::selectedIndex() const {
  return _selectedIndex;
}

void ClayLayoutController::setSelectedIndex(int index, int menuItemCount) {
  _selectedIndex = ClayLayoutController::wrapIndex(index, menuItemCount);
}

void ClayLayoutController::updateScrollMetrics(int displayHeight,
                                               int rowHeight,
                                               int rowGap,
                                               int viewportPadding,
                                               int menuItemCount,
                                               bool displayReady) {
  const int32_t rowStride = static_cast<int32_t>(rowHeight) +
                            static_cast<int32_t>(rowGap);
  if (rowStride > 0) {
    _systemSettingsScrollStep = rowStride;
  }

  if (_viewMode != ClayViewMode::Menu) {
    _menuScroll.configure(displayHeight, displayHeight);
    _menuScroll.setOffset(0);
    return;
  }

  if (!displayReady) {
    const int32_t items = static_cast<int32_t>(menuItemCount);
    const int32_t contentHeight =
        items * rowHeight +
        (items > 0 ? (items - 1) * rowGap : 0);
    _menuScroll.reset();
    _menuScroll.configure(0, contentHeight);
    _menuScroll.setOffset(0);
    return;
  }

  int32_t viewport = displayHeight - 2 * viewportPadding;
  if (viewport < 0) {
    viewport = 0;
  }
  const int32_t items = static_cast<int32_t>(menuItemCount);
  const int32_t contentHeight =
      items * rowHeight +
      (items > 0 ? (items - 1) * rowGap : 0);
  _menuScroll.configure(viewport, contentHeight);
}

void ClayLayoutController::ensureSelectionVisible(int rowHeight,
                                                  int rowGap) {
  if (_viewMode != ClayViewMode::Menu) {
    return;
  }
  if (_menuScroll.viewportHeight() <= 0) {
    return;
  }

  const int32_t itemStride = rowHeight + rowGap;
  const int32_t itemTop = _selectedIndex * itemStride;
  const int32_t itemBottom = itemTop + rowHeight;

  _menuScroll.ensureVisible(itemTop, itemBottom);
}

void ClayLayoutController::updateDetailScrollMetrics(int viewportHeight,
                                                     int contentHeight) {
  _detailScroll.configure(viewportHeight, contentHeight);
}

void ClayLayoutController::resetDetailScroll() {
  _detailScroll.setOffset(0);
}

void ClayLayoutController::scrollDetailToEnd() {
  _detailScroll.setOffset(_detailScroll.maxScroll());
}

void ClayLayoutController::ensureDetailSelectionVisible(int selectedIndex,
                                                        int rowHeight,
                                                        int rowGap) {
  if (selectedIndex < 0) {
    return;
  }
  if (_detailScroll.viewportHeight() <= 0) {
    return;
  }
  if (rowHeight <= 0) {
    return;
  }

  const int32_t stride = rowHeight + rowGap;
  const int32_t itemTop = static_cast<int32_t>(selectedIndex) * stride;
  const int32_t itemBottom = itemTop + rowHeight;
  _detailScroll.ensureVisible(itemTop, itemBottom);
}

void ClayLayoutController::rememberMenuState() {
  _menuSelectedIndex = _selectedIndex;
  _menuScrollSnapshot = _menuScroll.offset();
}

void ClayLayoutController::restoreMenuState() {
  _selectedIndex = _menuSelectedIndex;
  _menuScroll.setOffset(_menuScrollSnapshot);
}

bool ClayLayoutController::openViewFromMenu(ClayViewMode targetView) {
  if (targetView == ClayViewMode::Home) {
    if (_viewMode == ClayViewMode::Home) {
      return false;
    }
    _viewMode = ClayViewMode::Home;
    _viewStackSize = 0;
    _detailScroll.setOffset(0);
    return true;
  }
  return openView(targetView);
}

bool ClayLayoutController::pushViewState(ClayViewMode mode,
                                         int32_t detailScrollOffset) {
  if (_viewStackSize >= _viewStack.size()) {
    return false;
  }

  _viewStack[_viewStackSize++] = ViewStackEntry{mode, detailScrollOffset};
  return true;
}

bool ClayLayoutController::popViewState(ViewStackEntry& entry) {
  if (_viewStackSize == 0) {
    return false;
  }

  entry = _viewStack[--_viewStackSize];
  return true;
}

}  // namespace epd2_9
