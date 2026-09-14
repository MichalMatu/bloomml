# LiteGraph EPD/Clay port vendor snapshot

Curated read-only reference material from `MichalMatu/esp32s3_LiteGraph` for the growbox display/menu/button/simulator stage.

Source commit: `5b8c758c365547ddeaab65bbe9f849bdd071695d`
Growbox baseline when the snapshot started: `39b5c023ea1b426e96b072652fcc2744af523aa6`

## Contract

Nothing below this directory is part of the growbox firmware build. Keep the native ESP-IDF SSD1680 backend, 296x128 framebuffer, asynchronous display worker and observer-only ownership. Port reusable mechanics into normal growbox components in bounded steps with host tests. Do not import GxEPD2, Nodeflow/WiFi application ownership or Arduino hardware ownership.

## Captured reference areas

- Clay scroll state, clipping, monochrome renderer and layout/text/theme helpers;
- full LiteGraph navigation/controller mechanics and menu layout/model reference;
- button/input event model and Arduino reference driver;
- ClayRenderEngine reference including 128 KiB minimum PSRAM arena and current/previous dirty-region union algorithm;
- reusable SDL bitmap-font renderer pieces plus an immutable pin to the 296x128 SDL harness/golden-check source;
- immutable pins to the generic host Clay test suite;
- immutable pin for bundled Clay 0.14.

## Clay 0.14 boundary

The bundled Clay header reports version 0.14 and requires C++20. Growbox production remains C++17. The intended integration is therefore an isolated C++20 component whose public boundary does not expose `Clay_*` types.

The 269888-byte `clay.h` is pinned in `source/lib/thirdparty/clay/CLAY_0_14_SOURCE.lock`. The GitHub connector cannot reuse blob objects across repositories, so the header is not duplicated here; copy it verbatim from the pinned LiteGraph commit when the real C++20 component is created.

## Port rule

Vendor code is evidence/reference, not production code. Adapt algorithms and tests to growbox models; do not add `vendor/litegraph_epd_port` to `EXTRA_COMPONENT_DIRS`, `idf_component_register`, or host build include paths.
