# Source manifest

Pinned source: `MichalMatu/esp32s3_LiteGraph@5b8c758c365547ddeaab65bbe9f849bdd071695d`.

## Included in this snapshot

- `lib/framework/epd2_9/clay_ui/controller/ClayScrollState.h`
- `lib/framework/epd2_9/clay_ui/controller/ClayScrollState.cpp`
- `lib/framework/epd2_9/clay_ui/render/ClipStack.h`
- `lib/framework/epd2_9/clay_ui/render/ClayRenderer.h`
- `lib/framework/epd2_9/clay_ui/render/ClayRenderer.cpp`
- `lib/framework/epd2_9/clay_ui/support/ClayTheme.h`
- `lib/framework/epd2_9/clay_ui/support/ClayTheme.cpp`
- `lib/framework/epd2_9/clay_ui/support/ClayLayoutPrimitives.h`
- `lib/framework/epd2_9/clay_ui/support/ClayTextUtils.h`
- `lib/framework/epd2_9/clay_ui/support/ClayTextUtils.cpp`
- `lib/framework/epd2_9/drivers/input/InputDriver.h`
- `lib/framework/epd2_9/drivers/input/InputDriver.cpp`

## Import when the corresponding port begins

### Real Clay component

- `lib/thirdparty/clay/README.md`
- `lib/thirdparty/clay/include/clay/clay.h`
- `lib/thirdparty/clay/src/ClayCore.cpp`

Clay header at the pinned source identifies itself as version 0.14 and requires C++20.

### Dirty-region / PSRAM reference

- `lib/framework/epd2_9/clay_ui/render/ClayRenderEngine.h`
- `lib/framework/epd2_9/clay_ui/render/ClayRenderEngine.cpp`
- `lib/framework/epd2_9/core/Memory.h`

Port the dirty-region algorithm into the existing growbox SSD1680 transaction model; do not replace the native backend.

### Navigation/menu

- `lib/framework/epd2_9/clay_ui/controller/ClayLayoutController.h`
- `lib/framework/epd2_9/clay_ui/controller/ClayLayoutController.cpp`
- `lib/framework/epd2_9/clay_ui/controller/ClayLayoutInputHandlers.cpp`
- `lib/framework/epd2_9/clay_ui/modules/menu/model/MenuModel.*`
- `lib/framework/epd2_9/clay_ui/modules/menu/layout/MenuLayout.*`
- `lib/framework/epd2_9/clay_ui/modules/menu/layout/MenuListLayout.*`
- `lib/framework/epd2_9/clay_ui/modules/common/ActionConfirmationDialog.*`

Use the mechanics, not LiteGraph-specific menu entries/presenters.

### Simulator

- `tools/clay_sim/sdl/SdlMain.cpp`
- `tools/clay_sim/sdl/clay_renderer_gfx.cpp`
- `tools/clay_sim/sdl/GfxFontRenderer.*`
- the font assets used by that target

Target growbox geometry remains 296x128. The simulator should become a host-only target and must not enter ESP-IDF firmware dependencies.

### Host tests

- `test/host/clay_ui/CMakeLists.txt`
- `test/host/clay_ui/clay_impl.cpp`
- `test/host/clay_ui/ClayRendererTestFixture.h`
- `test/host/clay_ui/clay_renderer_core_tests.cpp`
- `test/host/clay_ui/clay_renderer_menu_tests.cpp`
- `test/host/clay_ui/test_clay_layout_controller.cpp`
- `test/host/clay_ui/test_clip_stack.cpp`

Adapt tests to growbox namespaces/models instead of copying application-specific assertions.
