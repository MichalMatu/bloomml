# Growbox ML Controller

Native ESP-IDF controller for an ESP32-S3 growbox with deterministic climate control, real sensors, RF433 outputs, durable telemetry, e-ink operator visibility and TinyML shadow/research tooling.

## Current state

The CrowPanel 2.9-inch SSD1680 display and SCD41 runtime path are operational enough for normal development; the SCD41 recovery closeout is no longer the active blocker.

The active next stage is a bounded display UI port: isolated Clay layout, 296x128 host simulator, menu/navigation and native buttons while preserving the existing native SSD1680 backend, async worker and observer-only ownership.

Canonical branch: `main`.

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

## Display port preparation

`vendor/litegraph_epd_port/` contains curated reference material from `MichalMatu/esp32s3_LiteGraph` for later Clay/menu/button/simulator work. It is excluded from all builds.

Current production application code is C++17; pinned Clay 0.14 requires C++20. The intended first integration is a separate C++20 component with no public `Clay_*` types, not a broad firmware language-standard migration.

## Hardware

Target: ESP32-S3 CrowPanel 2.9-inch e-paper, N8R8 profile.

Authorized growbox serial device: `/dev/cu.usbserial-1130`.
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

A successful host/simulator/firmware build is software evidence, not physical qualification.

## Documentation policy

Only current specifications, operational references, research material that still backs working tools, and compact history belong in `docs/`. Completed handoffs, temporary prompts and superseded audits stay in Git history.

## License

MIT. Vendored/upstream components retain their upstream notices.
