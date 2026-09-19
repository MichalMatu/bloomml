#include "growbox_clay_ui/HostSimulator.h"

static_assert(growbox::clay_ui::kDisplayWidth == 296);
static_assert(growbox::clay_ui::kDisplayHeight == 128);
static_assert(growbox::clay_ui::kFrameBytes == 4736);

int growbox_clay_public_api_cxx17_probe() {
  growbox::clay_ui::MonochromeFrame frame;
  return frame.bytes.size() == growbox::clay_ui::kFrameBytes ? 0 : 1;
}
