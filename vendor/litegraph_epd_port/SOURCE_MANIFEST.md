# Source manifest

Pinned source: `MichalMatu/esp32s3_LiteGraph@5b8c758c365547ddeaab65bbe9f849bdd071695d`.

## Copied reference code

- `lib/framework/epd2_9/clay_ui/controller/ClayScrollState.*`
- `lib/framework/epd2_9/clay_ui/controller/ClayLayoutController.*`
- `lib/framework/epd2_9/clay_ui/controller/ClayLayoutInputHandlers.cpp`
- `lib/framework/epd2_9/clay_ui/ClayViewState.h`
- `lib/framework/epd2_9/clay_ui/render/ClipStack.h`
- `lib/framework/epd2_9/clay_ui/render/ClayRenderer.*`
- `lib/framework/epd2_9/clay_ui/render/ClayRenderEngine.*`
- `lib/framework/epd2_9/clay_ui/support/ClayTheme.*`
- `lib/framework/epd2_9/clay_ui/support/ClayLayoutPrimitives.h`
- `lib/framework/epd2_9/clay_ui/support/ClayTextUtils.*`
- `lib/framework/epd2_9/clay_ui/modules/menu/model/MenuModel.*`
- `lib/framework/epd2_9/clay_ui/modules/menu/layout/MenuLayout.*`
- `lib/framework/epd2_9/clay_ui/modules/menu/layout/MenuListLayout.*`
- `lib/framework/epd2_9/clay_ui/modules/menu/preview/MenuPreviewLayout.*`
- `lib/framework/epd2_9/drivers/input/InputDriver.*`
- `lib/framework/epd2_9/core/Memory.h` as PSRAM-allocation reference only
- `lib/thirdparty/clay/README.md` and `src/ClayCore.cpp`
- reusable SDL `GfxFontRenderer.*` and `GfxFontData.cpp`
- generic host references: `clay_impl.cpp`, `test_clip_stack.cpp`, `ClayRendererTestFixture.h`, `clay_renderer_core_tests.cpp`

## Immutable source pins

### Clay 0.14

`source/lib/thirdparty/clay/CLAY_0_14_SOURCE.lock` pins the exact 269888-byte single header, blob `58006d208f7e58d646578b42524068f44445a4dc`. Copy it verbatim only when creating the isolated C++20 component.

### SDL simulator

`source/tools/clay_sim/SIMULATOR_SOURCE.lock` pins `SdlMain.cpp`, `clay_renderer_gfx.cpp` and renderer/font blobs. The harness is useful, but its sample screens are LiteGraph-specific. Rebuild the growbox simulator around Environment / Outputs / System / Diagnostics while retaining 296x128 geometry and headless golden checks.

### Remaining host tests

`source/test/host/clay_ui/HOST_TEST_SOURCE.lock` pins the broader CMake, renderer/menu/controller test suite. Generic renderer/clipping tests are copied next to it; application-specific assertions stay pinned until adapted.

## Adaptation rules

- keep the current native SSD1680 backend and async worker;
- move only the dirty-region calculation into growbox refresh planning;
- replace `Memory.h`/Arduino PSRAM helpers with native ESP-IDF `heap_caps_*` allocation in the real port;
- rewrite physical buttons with ESP-IDF GPIO and a host-testable debounce/long-press state machine;
- keep current growbox `DisplaySnapshot`/presenter truth boundary;
- no LiteGraph WiFi/Nodeflow presenters in production growbox;
- vendor remains excluded from all builds.
