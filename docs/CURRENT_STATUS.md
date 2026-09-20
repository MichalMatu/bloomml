# Current controller status

Updated: 2026-09-20
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`

## Current phase

Runtime reliability hardening and Display UI Port Phase 1 are integrated on `main`.

Fresh audit baseline on 2026-09-20:

- `main`: `ba4eb341139731189ac8706462020faa58bad1ae` (`Add isolated Clay UI host component`);
- runtime-hardening integration parent: `0b605d4c9f3f17f99b799a5248e5b4cfb3d740f6`;
- Local Agent architecture audit `20260920-architecture-agents-audit-v1`: terminal `done`, exact baseline `ba4eb341139731189ac8706462020faa58bad1ae`;
- Local Agent was idle after the audit.

The active implementation stage is Display UI Port Phase 2: reproduce the current Environment, Outputs, System and Diagnostics pages in the exact 296x128 host simulator while preserving the native SSD1680 backend and observer-only architecture.

The live implementation contract is `docs/DISPLAY_UI_PORT.md`.

## Architecture quality baseline

The repository already has a meaningful runtime split: composition/lifetime wiring, cycle coordination, output ownership, persistence, telemetry, display and RF transport are separated by subsystem.

The 2026-09-20 read-only architecture audit found no new obvious production mega-function in first-party C++ through the coarse long-function scan, but it identified growth-risk files that should not keep accumulating unrelated responsibility:

- `lib/environment_control/src/SafetySupervisor.cpp` — about 759 lines;
- `tools/panel/static/js/form.js` — about 1103 lines;
- `src/demo/protocol/ScenarioWireCodec.cpp` — about 1054 lines.

Generated code, pinned third-party Clay and large test fixtures are not refactor targets merely because of size.

`AGENTS.md` contains the repository-wide architecture-first implementation gate, dependency/ownership rules, anti-God-object rules and ESP32-S3 resource guardrails. New independent responsibilities should be placed in focused modules rather than appended to central coordinators/controllers for convenience.

## Production invariants

- deterministic Rule control is authoritative;
- ML remains shadow/research-only;
- `OutputSupervisor` is the only normal configured-output owner;
- unavailable transport never becomes fake execution success;
- one-way RF completion is not physical acknowledgement;
- lamp thermal trip remains `>= 28 C`;
- lamp recovery remains `<= 26 C` continuously for 10 minutes;
- UI/e-ink is observer-side and does not mutate control or safety state;
- slow display work stays outside the 1-second control hot path;
- global NVS lifecycle/recovery is owned by runtime composition, not BLE;
- failed durable-output writes are retried without falsely advancing persisted state.

## Current e-ink baseline

The CrowPanel 2.9-inch SSD1680 display path is operational.

Keep:

- 296x128 monochrome native framebuffer/backend;
- native ESP-IDF SPI/GPIO ownership;
- 180-degree rotation already established;
- asynchronous display worker;
- statically allocated display worker task/stack with stack high-water diagnostics;
- display success/confirmation transaction semantics;
- four operator pages: Environment, Outputs, System, Diagnostics;
- current `DisplaySnapshot` / presenter truth boundary.

The current partial-refresh path uses partial waveform handling but still transfers the full framebuffer. True reduced dirty-window transfer remains a later phase in `DISPLAY_UI_PORT.md`.

## SCD41 state

The production line retains the Sensirion-aligned clean-start sequence:

`wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`

and one bounded liveness recovery after 30 seconds without a new measurement, at most once per MCU boot. Do not restart broad SCD41 diagnosis unless new evidence shows a regression.

Historical exact-SHA physical evidence and the intermittent-failure reproduction are preserved in `HISTORY.md` / `CHANGELOG.md` and Git history.

## NVS/output persistence state

Global ESP-IDF NVS ownership is centralized before BLE/output persistence startup. Failed durable-output writes remain retryable with bounded runtime backoff, and fallback snapshot repair is covered by the hardening work already integrated into `main`.

Do not redesign this ownership unless a new regression or requirement demonstrates that the current boundary is insufficient.

## Current Clay boundary

The isolated Phase 1 component is integrated under `lib/growbox_clay_ui/`.

It provides:

- an isolated C++20 Clay implementation;
- a growbox-owned public boundary with no exposed `Clay_*` types;
- an exact 296x128 monochrome host simulator;
- component/renderer/clipping/smoke verification described in `DISPLAY_UI_PORT.md`.

The normal production application remains C++17. Firmware integration has not started; that belongs to later phases after Phase 2 reproduces the current pages on host.

## Verification state

Historical runtime-hardening verification and physical evidence remain in `HISTORY.md`, `CHANGELOG.md` and exact Local Agent result files.

The 2026-09-20 architecture audit was intentionally read-only. It confirmed the exact current `main` baseline and inspected repository structure/file-growth risk; it did not claim a fresh firmware build or hardware qualification.

For new implementation work, use the smallest relevant gate first and follow `AGENTS.md` / `DISPLAY_UI_PORT.md` for widening verification.

## Restart point

For a new ChatGPT window:

1. Read `AGENTS.md`, `docs/README.md`, this file, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` and the relevant subsystem document.
2. Fetch fresh `main` and `agent-control:.agent/status/daemon.json` before any write.
3. Continue from the first incomplete phase in `docs/DISPLAY_UI_PORT.md`; currently that is Phase 2.
4. Keep Phase 1 under `lib/growbox_clay_ui/`; do not re-create it under another directory or restart it from donor code.
5. Preserve the native SSD1680 backend, async observer transaction and C++17/C++20 boundary.
6. Apply the architecture completion gate before declaring substantial code changes complete.

## Next work

0. Runtime hardening integration and full CI — DONE.
1. Isolated C++20 Clay component plus exact 296x128 host simulator — DONE and integrated on `main`.
2. Reproduce the existing four growbox pages with deterministic host/golden checks — NEXT.
3. Attach Clay behind the existing async display transaction.
4. Add true dirty-region RAM-window transfer.
5. Add native ESP-IDF button input with host-tested debounce/long-press behavior.
6. Add only justified growbox menu/settings flows through existing domain APIs.
7. Qualify the final exact SHA on hardware when a physical claim is required.

Do not reopen completed panel pin mapping, SSD1680 backend ownership, rotation, async architecture, SCD41 recovery or runtime persistence ownership without new evidence.

## Hardware boundary

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`. Do not use `/dev/cu.usbserial-1120` without explicit authorization.

Physical outputs remain disabled during unattended debugging/qualification unless the operator explicitly changes that instruction.
