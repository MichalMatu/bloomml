#pragma once

/**
 * @file ClayLayoutController.h
 * @brief Input-driven view and page state controller for the Clay E-Ink UI.
 */

#include <array>
#include <cstddef>
#include <cstdint>

#include "epd2_9/clay_ui/ClayViewState.h"
#include "epd2_9/clay_ui/controller/ClayScrollState.h"
#include "epd2_9/drivers/input/InputDriver.h"

namespace epd2_9 {

class ClayLayoutController;

namespace layout_handlers {
bool handleHomeInput(ClayLayoutController& controller, const InputEvent& ev);
bool handleMenuInput(ClayLayoutController& controller,
                     const InputEvent& ev,
                     int menuItemCount);
bool handleWifiOverviewInput(ClayLayoutController& controller,
                             const InputEvent& ev);
bool handleWifiStatusInput(ClayLayoutController& controller,
                           const InputEvent& ev);
bool handleAutomationSettingsInput(ClayLayoutController& controller,
                                   const InputEvent& ev);
bool handleSetTimeInput(ClayLayoutController& controller,
                        const InputEvent& ev);
bool handleSystemSettingsInput(ClayLayoutController& controller,
                               const InputEvent& ev);
bool handleWifiProfileSettingsInput(ClayLayoutController& controller,
                                    const InputEvent& ev);
}  // namespace layout_handlers

class ClayLayoutController {
public:
  static constexpr int32_t kDefaultSystemSettingsScrollStep = 38;

  ClayLayoutController();
  void reset();
  bool handleInput(const InputEvent& ev, int menuItemCount);
  bool openMenu(bool restoreSelection = true);
  bool openSelectedMenuItem(int menuItemCount);
  bool returnToMenu(bool restoreSelection = true);
  bool returnToPreviousView();
  bool returnHome();
  bool openView(ClayViewMode targetView);
  bool adjustDetailScroll(int32_t delta);
  bool nextHomeScreen();
  bool previousHomeScreen();
  void setHomeScreenCount(std::size_t count);
  ClayViewMode viewMode() const;
  std::size_t activeHomeScreenIndex() const;
  std::size_t homeScreenCount() const { return _homeScreenCount; }
  int selectedIndex() const;
  void setSelectedIndex(int index, int menuItemCount);
  void updateScrollMetrics(int displayHeight,
                           int rowHeight,
                           int rowGap,
                           int viewportPadding,
                           int menuItemCount,
                           bool displayReady);
  void ensureSelectionVisible(int rowHeight, int rowGap);
  int32_t scrollOffset() const { return _menuScroll.offset(); }
  int32_t maxScroll() const { return _menuScroll.maxScroll(); }
  int32_t viewportHeight() const { return _menuScroll.viewportHeight(); }
  int32_t contentHeight() const { return _menuScroll.contentHeight(); }
  void updateDetailScrollMetrics(int viewportHeight, int contentHeight);
  int32_t detailScrollOffset() const { return _detailScroll.offset(); }
  int32_t detailMaxScroll() const { return _detailScroll.maxScroll(); }
  int32_t systemSettingsScrollStep() const { return _systemSettingsScrollStep; }
  void resetDetailScroll();
  void scrollDetailToEnd();
  void ensureDetailSelectionVisible(int selectedIndex,
                                    int rowHeight,
                                    int rowGap);

private:
  struct ViewStackEntry {
    ClayViewMode mode;
    int32_t detailScrollOffset;
  };

  static constexpr std::size_t kMaxViewStackDepth = 4;
  static int wrapIndex(int index, int itemCount);
  bool pushViewState(ClayViewMode mode, int32_t detailScrollOffset);
  bool popViewState(ViewStackEntry& entry);
  void rememberMenuState();
  void restoreMenuState();
  bool openViewFromMenu(ClayViewMode targetView);

  ClayViewMode _viewMode;
  int _selectedIndex;
  ClayScrollState _menuScroll;
  ClayScrollState _detailScroll;
  int _menuSelectedIndex;
  int32_t _menuScrollSnapshot;
  std::size_t _activeHomeScreenIndex;
  std::size_t _homeScreenCount;
  int32_t _systemSettingsScrollStep;
  std::array<ViewStackEntry, kMaxViewStackDepth> _viewStack;
  std::size_t _viewStackSize;
};

}  // namespace epd2_9
