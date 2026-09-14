# LiteGraph EPD/Clay port vendor snapshot

This directory is a curated, read-only source snapshot from `MichalMatu/esp32s3_LiteGraph` for the next growbox display/menu/button/simulator stage.

Source repository: `MichalMatu/esp32s3_LiteGraph`
Source commit: `5b8c758c365547ddeaab65bbe9f849bdd071695d`
Destination baseline when imported: `growbox-ml-controller` `39b5c023ea1b426e96b072652fcc2744af523aa6`

## Contract

- Nothing below `vendor/litegraph_epd_port/` is part of the growbox firmware build.
- Treat the files as port/reference material, not as a second display implementation.
- Keep the existing native ESP-IDF SSD1680 backend, fixed 296x128 framebuffer, asynchronous display worker and observer-only ownership.
- Port reusable Clay/layout/input ideas into normal growbox components in bounded steps with host tests.
- Do not import GxEPD2, LiteGraph Nodeflow/Wi-Fi presenters or Arduino hardware ownership into production growbox code.
- The LiteGraph Clay package is C++20; growbox production code is currently C++17. A future real Clay integration should use an isolated C++20 component and keep Clay types behind that boundary.

## Included now

The first snapshot contains the small/high-value pieces that are useful for direct adaptation:

- scroll state;
- clipping and monochrome Clay renderer abstraction;
- shared Clay theme/layout primitives/text helpers;
- the button/input event model and current Arduino reference implementation.

Original paths are retained below `source/` so dependencies and provenance stay obvious.

## Deliberately not copied yet

Larger or tightly coupled files are listed in `SOURCE_MANIFEST.md` and should be imported only when their port starts. In particular this includes bundled Clay 0.14, the SDL simulator, full view/controller/presenter graph, host Clay tests and the full `ClayRenderEngine` dirty-region/PSRAM implementation.

This keeps the initial vendor snapshot useful without silently adding a second framework or build dependency.
