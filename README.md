# BloomML

Native ESP-IDF controller for an ESP32-S3 growbox with deterministic climate control, real sensors, RF433 outputs, durable telemetry, e-ink operator visibility and TinyML shadow/research tooling.

## Current state

The CrowPanel 2.9-inch SSD1680 display path has an integrated software baseline. Physical SCD41-with-sensor-present and manual button/visual qualification remain open.

Clay display Phases 1-6 are integrated, including dirty-region partial transfer, native button input and a read-only four-page chooser; outstanding physical qualification is not a software integration failure. The C++20 Clay 0.14 renderer writes into the single SSD1680 backend-owned framebuffer behind the existing asynchronous display transaction.

Canonical product branch: `main`. The separate `research/simulator-fidelity-kaggle-20261103` branch holds non-production Kaggle/physics/MPC work; it has not been integrated.

**Product direction:** one operator flow for installed sensors/outputs, targets, safe autonomous control and monitoring. Today `tools/panel/` is a local **v4 demo/serial** panel, `web/` is a **v5 research** configurator and geometric 3D preview, while production firmware uses **climate-v6**. These schemas are not interchangeable. See [product direction](docs/PRODUCT_DIRECTION.md), [audit ledger](docs/AUDIT_STATUS.md) and [next-chat handoff](docs/NEXT_CHAT.md).

Read first:

- `docs/README.md` — documentation map;
- `docs/CURRENT_STATUS.md` — active baseline and next work;
- `docs/ARCHITECTURE.md` — runtime ownership/safety boundaries;
- `docs/PROJECT_ROADMAP.md` — ordered product work;
- `docs/DISPLAY_UI_PORT.md` — current display/Clay port contract.

## Core architecture

Production control remains conservative:

- deterministic Rule control is authoritative;
- ML remains shadow/research-only;
- `OutputSupervisor` is the only normal configured physical-output owner;
- requested, resolved, transport-executed and physically observed output state are distinct;
- unavailable transport must never be presented as successful execution;
- the display/UI is observer-only.

Portable climate logic lives under `lib/environment_control/`. Native ESP-IDF runtime/application code lives under `src/`. Browser UX lives under `web/`; simulation/ML tooling lives under `tools/ml/`.

## Display / Clay architecture

`vendor/litegraph_epd_port/` contains curated reference material from `MichalMatu/esp32s3_LiteGraph`. It is excluded from all builds.

Production application code remains C++17; pinned Clay 0.14 requires C++20 and is isolated under `lib/growbox_clay_ui/` with a Clay-free C++17-compatible public boundary. `lib/growbox_display_model/` contains the behavior-free semantic page DTO shared with the presenter.

Production ownership is unchanged by Clay integration:

- `CrowPanelDisplayService` owns the asynchronous worker and one persistent Clay scratch arena in PSRAM;
- `CrowPanelSsd1680DisplayBackend` owns the only 296x128 / 4736-byte framebuffer plus SPI/GPIO/controller access;
- Clay owns layout/rasterization only and receives temporary framebuffer access during an open transaction;
- navigation, refresh confirmation/retry, control and safety remain outside Clay.

For production display compilation use:

```bash
make build-crowpanel
```

That target builds the actual CrowPanel N8R8 + Stage27 NimBLE + Stage27C + `climate-v6-real-inputs` + e-ink path. Generic `make build` uses a different default app mode and is not sufficient evidence for CrowPanel display integration.

## Hardware

Target: ESP32-S3 CrowPanel 2.9-inch e-paper, N8R8 profile.

Authorized growbox serial device in current `AGENTS.md`: `/dev/cu.usbserial-120` (verify fresh Local Agent host state before any operation).
Never use `/dev/cu.usbserial-10`; do not use `/dev/cu.usbserial-1120` without explicit authorization.

Hardware references:

- `docs/IO_MAP.md`
- `docs/RF433_DEVICE_CODES.md`
- `docs/HARDWARE_BRINGUP_CHECKLIST.md`
- `docs/ESP32_S3_SERIAL_PORT_RESET.md`
- `docs/SHELLY_POWER_FEEDBACK.md`

## Firmware stack

- ESP-IDF 5.5.4
- ESP32-S3
- production application component: C++17
- isolated Clay 0.14 component: C++20
- CMake / CTest host tests
- deterministic safety/output supervision
- native SCD41/BLE/RTC/RF433 paths
- native SSD1680 e-ink runtime
- clang-format / clang-tidy / pre-commit quality gates

## Repository layout

```text
components/                 ESP-IDF/local components
config/                     board/runtime profiles
lib/environment_control/    portable climate controller
lib/growbox_display_model/  semantic display DTO
lib/growbox_clay_ui/        isolated Clay layout/raster component
schemas/                    production + research contracts
src/                        native ESP-IDF application/runtime
test/                       C++ host tests
tests/                      Python/tool tests
tools/                      ML/panel/sandbox/serial tooling
vendor/litegraph_epd_port/  read-only Clay/EPD donor snapshot
web/                        browser configurator/chamber UI
docs/                       documentation map + subsystem docs/history
```

See `docs/PROJECT_LAYOUT.md` for placement rules.

## Verification

Repository-only work should use `docs/SANDBOX_EXECUTION_FLOW.md`; Mac/USB/serial/flashing and physical-board evidence use Local Agent according to `AGENTS.md`.

For Clay/display production changes use host/display tests and `make build-crowpanel`. A successful host/simulator/firmware build is software evidence, not physical qualification.

## Documentation policy

Only current specifications, operational references, research material that still backs working tools, and compact history belong in `docs/`. Completed handoffs, temporary prompts and superseded audits stay in Git history.

## License

MIT. Vendored/upstream components retain their upstream notices.
