#include "climate/display/input/DisplayButtonStateMachine.h"

#include <cassert>

namespace display = growbox::app::climate_io::display;

namespace {

void testStableEdgeDebounceResetsOnEveryRawChange() {
  display::DisplayButtonStateMachine state({40U, 700U});
  display::DisplayButtonEvent event{};
  state.reset(false, 0U);

  assert(!state.update(display::DisplayButton::Next, true, 10U, event));
  assert(!state.update(display::DisplayButton::Next, false, 20U, event));
  assert(!state.update(display::DisplayButton::Next, true, 25U, event));
  assert(!state.update(display::DisplayButton::Next, true, 64U, event));
  assert(!state.stablePressed());
  assert(!state.update(display::DisplayButton::Next, true, 65U, event));
  assert(state.stablePressed());

  assert(!state.update(display::DisplayButton::Next, false, 100U, event));
  assert(!state.update(display::DisplayButton::Next, true, 110U, event));
  assert(!state.update(display::DisplayButton::Next, false, 115U, event));
  assert(!state.update(display::DisplayButton::Next, false, 154U, event));
  assert(state.stablePressed());
  assert(state.update(display::DisplayButton::Next, false, 155U, event));
  assert(!state.stablePressed());
  assert(event.button == display::DisplayButton::Next);
  assert(event.gesture == display::DisplayButtonGesture::Press);
  assert(event.timestamp_ms == 155U);
}

void testShortPressEmitsOnlyAfterStableRelease() {
  display::DisplayButtonStateMachine state({40U, 700U});
  display::DisplayButtonEvent event{};
  state.reset(false, 0U);

  assert(!state.update(display::DisplayButton::Ok, true, 100U, event));
  assert(!state.update(display::DisplayButton::Ok, true, 140U, event));
  assert(state.stablePressed());
  assert(!state.update(display::DisplayButton::Ok, false, 220U, event));
  assert(!state.update(display::DisplayButton::Ok, false, 259U, event));
  assert(state.update(display::DisplayButton::Ok, false, 260U, event));
  assert(event.button == display::DisplayButton::Ok);
  assert(event.gesture == display::DisplayButtonGesture::Press);
}

void testLongPressUsesStablePressStartAndSuppressesShortPress() {
  display::DisplayButtonStateMachine state({40U, 700U});
  display::DisplayButtonEvent event{};
  state.reset(false, 0U);

  assert(!state.update(display::DisplayButton::Ok, true, 100U, event));
  assert(!state.update(display::DisplayButton::Ok, true, 140U, event));
  assert(!state.update(display::DisplayButton::Ok, true, 799U, event));
  assert(state.update(display::DisplayButton::Ok, true, 800U, event));
  assert(event.button == display::DisplayButton::Ok);
  assert(event.gesture == display::DisplayButtonGesture::LongPress);
  assert(event.timestamp_ms == 800U);

  assert(!state.update(display::DisplayButton::Ok, true, 900U, event));
  assert(!state.update(display::DisplayButton::Ok, false, 910U, event));
  assert(!state.update(display::DisplayButton::Ok, false, 950U, event));
  assert(!state.stablePressed());
}

void testCandidateReleasePausesLongPressUntilContactReturns() {
  display::DisplayButtonStateMachine state({40U, 700U});
  display::DisplayButtonEvent event{};
  state.reset(false, 0U);

  assert(!state.update(display::DisplayButton::Back, true, 100U, event));
  assert(!state.update(display::DisplayButton::Back, true, 140U, event));
  assert(!state.update(display::DisplayButton::Back, false, 795U, event));
  assert(!state.update(display::DisplayButton::Back, false, 810U, event));
  assert(state.update(display::DisplayButton::Back, true, 820U, event));
  assert(event.button == display::DisplayButton::Back);
  assert(event.gesture == display::DisplayButtonGesture::LongPress);
}

void testUninitializedStatePrimesWithoutSyntheticEvent() {
  display::DisplayButtonStateMachine state({40U, 700U});
  display::DisplayButtonEvent event{};
  assert(!state.initialized());
  assert(!state.update(display::DisplayButton::Home, true, 1'000U, event));
  assert(state.initialized());
  assert(state.stablePressed());
  assert(!state.update(display::DisplayButton::Home, true, 1'699U, event));
  assert(state.update(display::DisplayButton::Home, true, 1'700U, event));
  assert(event.gesture == display::DisplayButtonGesture::LongPress);
}

} // namespace

int main() {
  testStableEdgeDebounceResetsOnEveryRawChange();
  testShortPressEmitsOnlyAfterStableRelease();
  testLongPressUsesStablePressStartAndSuppressesShortPress();
  testCandidateReleasePausesLongPressUntilContactReturns();
  testUninitializedStatePrimesWithoutSyntheticEvent();
  return 0;
}
