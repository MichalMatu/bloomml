#include "ClayRendererTestFixture.h"

#include <cstdio>

#include "epd2_9/clay_ui/render/ClipStack.h"

namespace {

void run(const char* name, void (*test)()) {
  std::printf("Clay renderer core: %s\n", name);
  test();
}

void test_renderer_single_level_clip() {
  MockRenderTarget target;
  ClayRenderer renderer(target);
  Clay_RenderCommand commands[2]{};
  commands[0] = makeRectCommand(makeBBox(0, 0, 100, 100), {255, 255, 255, 255});
  commands[1] = makeRectCommand(makeBBox(10, 10, 80, 80), {0, 0, 0, 255});
  Clay_RenderCommandArray cmdArray{.capacity = 2, .length = 2, .internalArray = commands};
  ClayRenderConfig config{296, 128};
  renderer.render(cmdArray, config);
  assert(target.commands.size() == 2);
  assert(target.commands[0].type == MockRenderTarget::DrawCommand::FillRect);
  assert(target.commands[0].x == 0 && target.commands[0].y == 0);
  assert(target.commands[1].x == 10 && target.commands[1].y == 10);
}

void test_renderer_border_command() {
  MockRenderTarget target;
  ClayRenderer renderer(target);
  Clay_RenderCommand commands[1]{};
  commands[0] = makeBorderCommand(makeBBox(5, 6, 40, 30), {0, 0, 0, 255}, 2);
  Clay_RenderCommandArray cmdArray{.capacity = 1, .length = 1, .internalArray = commands};
  ClayRenderConfig config{296, 128};
  renderer.render(cmdArray, config);
  assert(target.commands.size() == 1);
  const auto& cmd = target.commands[0];
  assert(cmd.type == MockRenderTarget::DrawCommand::DrawRect);
  assert(cmd.x == 5 && cmd.y == 6 && cmd.w == 40 && cmd.h == 30);
}

void test_renderer_text_command_propagates_active_clip() {
  MockRenderTarget target;
  ClayRenderer renderer(target);
  static constexpr char kSampleText[] = "Overflow candidate";
  Clay_RenderCommand commands[3]{};
  commands[0].commandType = CLAY_RENDER_COMMAND_TYPE_SCISSOR_START;
  commands[0].boundingBox = makeBBox(24, 18, 60, 22);
  commands[1] = makeTextCommand(makeBBox(12, 22, 100, 14), kSampleText, 0, 14, {0, 0, 0, 255});
  commands[2].commandType = CLAY_RENDER_COMMAND_TYPE_SCISSOR_END;
  Clay_RenderCommandArray cmdArray{.capacity = 3, .length = 3, .internalArray = commands};
  ClayRenderConfig config{296, 128};
  renderer.render(cmdArray, config);
  assert(target.commands.size() == 1);
  const auto& recorded = target.commands[0];
  assert(recorded.type == MockRenderTarget::DrawCommand::DrawText);
  assert(recorded.clipX == 24 && recorded.clipY == 18 && recorded.clipW == 60 && recorded.clipH == 22);
}

}  // namespace

void run_clay_renderer_core_tests() {
  run("renderer_single_level_clip", test_renderer_single_level_clip);
  run("renderer_border_command", test_renderer_border_command);
  run("renderer_text_command_propagates_active_clip", test_renderer_text_command_propagates_active_clip);
}
