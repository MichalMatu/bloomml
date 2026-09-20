#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

#include <clay/clay.h>

#include "ClayRenderer.h"
#include "ClipStack.h"
#include "growbox_clay_ui/HostSimulator.h"

namespace {
using growbox::clay_ui::internal::ClayRenderConfig;
using growbox::clay_ui::internal::ClayRenderer;
using growbox::clay_ui::internal::ClayRenderTarget;
using growbox::clay_ui::internal::ClipStack;

struct MockTarget final : ClayRenderTarget {
  struct Fill {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
  };
  std::vector<Fill> fills;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t) override {
    fills.push_back({x, y, w, h});
  }
  void drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) override {}
  void drawText(const char*, int16_t, int16_t, int16_t, uint16_t, uint16_t, uint16_t, int16_t,
                int16_t, int16_t, int16_t, int16_t, int16_t) override {}
  void setFont(uint16_t) override {}
};

Clay_BoundingBox box(float x, float y, float w, float h) {
  return Clay_BoundingBox{x, y, w, h};
}

std::uint64_t frameHash(const growbox::clay_ui::MonochromeFrame& frame) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const std::uint8_t byte : frame.bytes) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

growbox::display_model::DisplayPageModel samplePage() {
  growbox::display_model::DisplayPageModel page{};
  std::snprintf(page.title.data(), page.title.size(), "Environment");
  page.line_count = 3U;
  std::snprintf(page.lines[0].label.data(), page.lines[0].label.size(), "Temp");
  std::snprintf(page.lines[0].value.data(), page.lines[0].value.size(), "23.4 C");
  std::snprintf(page.lines[1].label.data(), page.lines[1].label.size(), "RH");
  std::snprintf(page.lines[1].value.data(), page.lines[1].value.size(), "61.2 %%");
  std::snprintf(page.lines[2].label.data(), page.lines[2].label.size(), "Mode");
  std::snprintf(page.lines[2].value.data(), page.lines[2].value.size(), "AUTO");
  return page;
}

void testGeometry() {
  static_assert(growbox::clay_ui::kDisplayWidth == 296);
  static_assert(growbox::clay_ui::kDisplayHeight == 128);
  static_assert(growbox::clay_ui::kFrameStrideBytes == 37);
  static_assert(growbox::clay_ui::kFrameBytes == 4736);
}

void testClipStackIntersection() {
  ClipStack clips;
  clips.push(10, 8, 30, 20);
  clips.push(20, 0, 30, 20);
  int16_t x = 0;
  int16_t y = 0;
  int16_t w = 60;
  int16_t h = 60;
  assert(clips.clipRectangle(x, y, w, h));
  assert(x == 20 && y == 8 && w == 20 && h == 12);
  clips.pop();
  assert(clips.clipRectangle(x, y, w, h));
}

void testRendererClipsRectangle() {
  MockTarget target;
  ClayRenderer renderer(target);
  Clay_RenderCommand commands[3]{};
  commands[0].commandType = CLAY_RENDER_COMMAND_TYPE_SCISSOR_START;
  commands[0].boundingBox = box(10, 8, 30, 20);
  commands[1].commandType = CLAY_RENDER_COMMAND_TYPE_RECTANGLE;
  commands[1].boundingBox = box(0, 0, 60, 60);
  commands[1].renderData.rectangle.backgroundColor = Clay_Color{0, 0, 0, 255};
  commands[2].commandType = CLAY_RENDER_COMMAND_TYPE_SCISSOR_END;
  Clay_RenderCommandArray array{.capacity = 3, .length = 3, .internalArray = commands};
  renderer.render(array, ClayRenderConfig{296, 128});
  assert(target.fills.size() == 1);
  assert(target.fills[0].x == 10 && target.fills[0].y == 8);
  assert(target.fills[0].w == 30 && target.fills[0].h == 20);
}

void testHostSimulator() {
  growbox::clay_ui::MonochromeFrame frame;
  growbox::clay_ui::RenderSummary summary;
  assert(growbox::clay_ui::renderHostSmoke(frame, &summary));
  assert(summary.width == 296 && summary.height == 128);
  assert(summary.render_commands > 0);
  assert(summary.black_pixels > 0);
  assert(summary.black_pixels < 296U * 128U);
  assert(frame.bytes.size() == 4736U);
}

void testSemanticPageRenderingIsDeterministic() {
  const auto page = samplePage();
  growbox::clay_ui::MonochromeFrame first{};
  growbox::clay_ui::MonochromeFrame second{};
  growbox::clay_ui::RenderSummary summary{};
  assert(growbox::clay_ui::renderHostPage(page, first, &summary));
  assert(growbox::clay_ui::renderHostPage(page, second));
  assert(first.bytes == second.bytes);
  assert(summary.render_commands >= page.line_count * 2U);
  assert(summary.black_pixels > 0U);

  auto warning_page = page;
  warning_page.warning = true;
  growbox::clay_ui::MonochromeFrame warning{};
  assert(growbox::clay_ui::renderHostPage(warning_page, warning));
  assert(frameHash(first) != frameHash(warning));
}

void testInvalidSemanticPageFailsClosed() {
  growbox::display_model::DisplayPageModel page{};
  growbox::clay_ui::MonochromeFrame frame{};
  assert(!growbox::clay_ui::renderHostPage(page, frame));

  page = samplePage();
  page.line_count = page.lines.size() + 1U;
  assert(!growbox::clay_ui::renderHostPage(page, frame));
}

} // namespace

int main() {
  std::puts("growbox Clay phase2: geometry");
  testGeometry();
  std::puts("growbox Clay phase2: clip stack");
  testClipStackIntersection();
  std::puts("growbox Clay phase2: renderer clipping");
  testRendererClipsRectangle();
  std::puts("growbox Clay phase2: 296x128 host simulator");
  testHostSimulator();
  std::puts("growbox Clay phase2: semantic page rendering");
  testSemanticPageRenderingIsDeterministic();
  testInvalidSemanticPageFailsClosed();
  return 0;
}
