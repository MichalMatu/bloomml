#include "growbox_clay_ui/HostSimulator.h"
#include "growbox_display_model/DisplayPageModel.h"

static_assert(growbox::clay_ui::kDisplayWidth == 296);
static_assert(growbox::clay_ui::kDisplayHeight == 128);
static_assert(growbox::clay_ui::kFrameBytes == 4736);
static_assert(growbox::display_model::DisplayPageModel::kMaxLines == 10U);

int growbox_clay_public_api_cxx17_probe() {
  growbox::clay_ui::MonochromeFrame frame;
  growbox::display_model::DisplayPageModel page;
  return frame.bytes.size() == growbox::clay_ui::kFrameBytes && page.lines.size() == 10U ? 0 : 1;
}
