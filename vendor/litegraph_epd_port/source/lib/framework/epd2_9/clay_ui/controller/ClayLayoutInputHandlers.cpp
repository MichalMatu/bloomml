#include "epd2_9/clay_ui/controller/ClayLayoutController.h"

#include "epd2_9/clay_ui/modules/menu/model/MenuModel.h"
#include "epd2_9/clay_ui/support/ClayTheme.h"

namespace epd2_9::layout_handlers {
namespace {
int32_t wifiStatusScrollStep() {
  const DenseStatusLayoutMetrics metrics = defaultDenseStatusMetrics();
  return static_cast<int32_t>(metrics.rowHeight) +
         static_cast<int32_t>(metrics.cardGap);
}
}  // namespace

bool handleHomeInput(ClayLayoutController& controller, const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::CrownPress:
      return controller.openMenu();
    case InputEventType::CrownDown:
      return controller.nextHomeScreen();
    case InputEventType::CrownUp:
      return controller.previousHomeScreen();
    default:
      break;
  }
  return false;
}

bool handleMenuInput(ClayLayoutController& controller,
                     const InputEvent& ev,
                     int menuItemCount) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::CrownDown:
      controller.setSelectedIndex(controller.selectedIndex() + 1, menuItemCount);
      return true;
    case InputEventType::CrownUp:
      controller.setSelectedIndex(controller.selectedIndex() - 1, menuItemCount);
      return true;
    case InputEventType::CrownPress:
      return controller.openSelectedMenuItem(menuItemCount);
    case InputEventType::Back:
      return controller.returnHome();
    default:
      break;
  }
  return false;
}

bool handleWifiOverviewInput(ClayLayoutController& controller,
                             const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::Back:
      return controller.returnToPreviousView();
    default:
      break;
  }
  return false;
}

bool handleWifiStatusInput(ClayLayoutController& controller,
                           const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::Back:
      return controller.returnToPreviousView();
    case InputEventType::CrownDown:
      return controller.adjustDetailScroll(wifiStatusScrollStep());
    case InputEventType::CrownUp:
      return controller.adjustDetailScroll(-wifiStatusScrollStep());
    default:
      break;
  }
  return false;
}

bool handleAutomationSettingsInput(ClayLayoutController& controller,
                                   const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::Back:
      return controller.returnToPreviousView();
    default:
      break;
  }
  return false;
}

bool handleSetTimeInput(ClayLayoutController& controller,
                        const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::Back:
      return controller.returnToPreviousView();
    default:
      break;
  }
  return false;
}

bool handleSystemSettingsInput(ClayLayoutController& controller,
                               const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::Back:
      return controller.returnToPreviousView();
    case InputEventType::CrownDown:
      return controller.adjustDetailScroll(controller.systemSettingsScrollStep());
    case InputEventType::CrownUp:
      return controller.adjustDetailScroll(-controller.systemSettingsScrollStep());
    default:
      break;
  }
  return false;
}

bool handleWifiProfileSettingsInput(ClayLayoutController& controller,
                                    const InputEvent& ev) {
  switch (ev.type) {
    case InputEventType::Home:
      return controller.returnHome();
    case InputEventType::Back:
      return controller.returnToPreviousView();
    default:
      break;
  }
  return false;
}

}  // namespace epd2_9::layout_handlers
