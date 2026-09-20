# Current controller status

Updated: 2026-09-20
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`

## Current phase

Runtime reliability hardening and Display UI Port Phases 1-4 are integrated on `main`.

Display UI Port Phase 4 is complete, including exact-SHA physical qualification of true reduced SSD1680 dirty-window transfer. The current implementation stage is Phase 5: add growbox-owned native ESP-IDF button input with host-tested debounce/long-press behavior while keeping navigation state outside the GPIO layer.

The live implementation contract is `docs/DISPLAY_UI_PORT.md`.

## Current source baseline

Phase 4 code is merged on source `4c32466c6a0af798620faf0e85416ad39ea4f07a` and was physically qualified on that exact SHA. Subsequent documentation-only commits may advance `main`; always fetch fresh `main` before new work.

Current architecture state:

- `ClimatePolicy.*` owns stateless Rule generation/arbitration/safety while `ClimateRuntimeController.*` owns stateful runtime/ML/execution reconciliation;
- `lib/growbox_display_model/` owns the C++17-compatible semantic `DisplayLine` / `DisplayPageModel` DTO;
- `lib/growbox_clay_ui/` owns the isolated C++20 Clay 0.14 layout/rasterization implementation;
- production CrowPanel rendering now uses that Clay renderer behind the existing asynchronous display transaction;
- `CrowPanelSsd1680DisplayBackend` remains sole owner of the one 296x128 / 4736-byte framebuffer and all native SSD1680 hardware access;
- `CrowPanelDisplayService` owns one persistent Clay scratch arena allocated in PSRAM and the existing display worker/task transaction;
- navigation remains the growbox `DisplayNavigation`; Clay does not own navigation, sensor/output truth, control or safety state;
- Phase 4 adds semantic row-aware dirty planning plus reduced native SSD1680 RAM-window transfer; a one-row update is verified at 1776 bytes/plane versus 4736 bytes full-plane;
- the exact merged Phase 4 SHA was flashed to `/dev/cu.usbserial-120` with real outputs disabled and produced a real `physical_partial=1` refresh without crash/watchdog/brownout evidence.

## Architecture quality baseline

The repository has a meaningful runtime split: composition/lifetime wiring, cycle coordination, output ownership, persistence, telemetry, display and RF transport are separated by subsystem.

The follow-up production-path audit showed that the three largest files from the original coarse scan were not production climate-v6 hotspots: root `SafetySupervisor.cpp` is legacy-only, `tools/panel/static/js/form.js` is host tooling, and `src/demo/protocol/ScenarioWireCodec.cpp` is legacy/demo code. Do not prioritize refactors from line count alone.

The first confirmed production cohesion issue was `ClimateRuntimeController.cpp`, which mixed stateless Rule generation/arbitration/safety with stateful runtime, ML and execution reconciliation. That boundary is now split and covered by existing climate runtime/parity tests.

Display architecture was clarified before Clay integration. The historical Clay-named generic display seam was removed; the neutral semantic page DTO is under `lib/growbox_display_model/`, while real Clay remains isolated in `lib/growbox_clay_ui/`. The Phase 3 bridge did not introduce another framebuffer, worker, navigation owner or hardware backend.

Generated code, pinned third-party Clay and large test fixtures are not refactor targets merely because of size. `AGENTS.md` contains the architecture-first implementation gate, dependency/ownership rules, anti-God-object rules and ESP32-S3 resource guardrails.

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

## Current e-ink / Clay baseline

The CrowPanel 2.9-inch SSD1680 display path remains native ESP-IDF and now uses Clay for production page layout/rasterization.

Keep:

- 296x128 monochrome native framebuffer/backend;
- exactly one 4736-byte framebuffer, owned by `CrowPanelSsd1680DisplayBackend`;
- native ESP-IDF SPI/GPIO ownership;
- established rotation;
- asynchronous display worker and static 6144-byte worker stack with stack high-water diagnostics;
- display success/confirmation transaction semantics;
- four operator pages: Environment, Outputs, System, Diagnostics;
- current `DisplaySnapshot` / presenter truth boundary;
- neutral `DisplayPageModel` semantic contract;
- isolated C++20 Clay implementation behind a Clay-free C++17-compatible public boundary;
- one persistent caller-owned Clay arena allocated in PSRAM by the display service.

The backend exposes mutable framebuffer bytes only while a frame transaction is open. Clay fills that storage but never replaces, owns or retains it. Failed Clay/frame/backend transactions cancel the frame and do not cause `confirmRendered()` to advance observer state.

Framebuffer polarity is now identical in host and firmware: cleared bit = black, set bit = white (`0 = black`, `1 = white`). PBM export handles its own format inversion.

Production-polarity nominal page hashes:

- Environment / `Growbox status`: `3bb469ff366c69bf` — 2270 black pixels;
- Outputs: `2e43841abef23957` — 2416 black pixels;
- System: `262017750c71fa89` — 2453 black pixels;
- Diagnostics: `c22a38670d9064f1` — 2321 black pixels.

Each nominal page emits 25 Clay render commands in the integration fixture.

The partial-refresh path now transfers only the mapped dirty SSD1680 RAM window. Dirty planning is based on differences between the last physically successful semantic page and the next page; hardware padding/alignment remains in the backend. Full refresh remains the fallback when previous controller RAM is not known to be seeded.

## Phase 3 verification state

Local Agent `20260920-clay-phase3-v3` provided broad software regression evidence: formatting, dedicated display/Clay suites, full 50-test `make test-host` and generic ESP-IDF build passed.

The generic firmware build is not sufficient as the production display gate because the default app mode does not compile the CrowPanel real-input display path. That gap is now closed by the canonical `make build-crowpanel` target and a dedicated CI job.

Local Agent `20260920-clay-phase3-crowpanel-build-v4` verified exact source `76447b963bb1e3f4d9290bfb26380ac30a0a41ab` with the canonical `scripts/stage27c_crowpanel.sh` configuration and `GROWBOX_EINK_DISPLAY_ENABLED=1`:

- display/Clay host suites: PASS;
- CrowPanel N8R8 + Stage27 NimBLE + Stage27C + `climate-v6-real-inputs` + e-ink production build: PASS;
- `CrowPanelDisplayService.cpp`, native SSD1680 backend and `ClimateV6RealInputRuntime.cpp` were compiled in that build;
- firmware binary: `0xcc970` bytes;
- smallest app partition: `0x400000`, with `0x333690` bytes / 80% free;
- `idf.py size` total image size: 837885 bytes;
- `git diff --check` and clean working tree: PASS.

This is software/build evidence only. No new physical hardware qualification is claimed for Phase 3.

## Phase 4 verification state

Phase 4 merged as `4c32466c6a0af798620faf0e85416ad39ea4f07a`. Software gates passed display/Clay suites, full 50-test `make test-host`, canonical `make build-crowpanel`, exact 1776-byte single-row native-window mapping and `git diff --check`.

Physical qualification on authorized `/dev/cu.usbserial-120` used e-ink enabled with real outputs/RF loopback/thermal test sequence disabled. The captured firmware reported the exact merged SHA and produced a real partial refresh: `dirty=0,48,296,41`, native `6-11,0-295`, `window_bytes=1776`, `ram_payload_bytes=3552`, `physical_partial=1`. The same 120-second capture contained no Guru Meditation, brownout, task/interrupt watchdog timeout, assert, abort or backtrace. Application `INFO/WATCHDOG ... heartbeat` records remained healthy heartbeats.

The display worker reported `stack_min_free_bytes=2228` during the qualified refresh.

## SCD41 state

The production line retains the Sensirion-aligned clean-start sequence:

`wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`

and one bounded liveness recovery after 30 seconds without a new measurement, at most once per MCU boot. Do not restart broad SCD41 diagnosis unless new evidence shows a regression.

Historical exact-SHA physical evidence and intermittent-failure reproduction remain in `HISTORY.md` / `CHANGELOG.md` and Git history.

## NVS/output persistence state

Global ESP-IDF NVS ownership is centralized before BLE/output persistence startup. Failed durable-output writes remain retryable with bounded runtime backoff, and fallback snapshot repair is covered by the integrated hardening work.

Do not redesign this ownership unless a new regression or requirement demonstrates that the current boundary is insufficient.

## Restart point

For a new ChatGPT window:

1. Read `AGENTS.md`, `docs/README.md`, this file, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` and `docs/DISPLAY_UI_PORT.md`.
2. Fetch fresh `main` and `agent-control:.agent/status/daemon.json` before any write.
3. Continue from the first incomplete phase in `docs/DISPLAY_UI_PORT.md`; currently that is Phase 5.
4. Keep Clay under `lib/growbox_clay_ui/`; do not re-create it under another directory or restart it from donor code.
5. Keep `lib/growbox_display_model/` as the shared behavior-free semantic DTO boundary.
6. Preserve the backend-owned single framebuffer, native SSD1680 backend, async observer transaction and C++17/C++20 boundary.
7. For display production claims run `make build-crowpanel`; generic `make build` does not cover the CrowPanel real-input display path.
8. Apply the architecture completion gate before declaring substantial code changes complete.

## Next work

0. Runtime hardening integration and full CI — DONE.
1. Isolated C++20 Clay component plus exact 296x128 host simulator — DONE.
2. Reproduce the existing four growbox pages with deterministic host/golden checks — DONE.
3. Attach Clay behind the existing async display transaction — DONE.
4. Add true dirty-region RAM-window transfer — DONE.
5. Add native ESP-IDF button input with host-tested debounce/long-press behavior — NEXT.
6. Add only justified growbox menu/settings flows through existing domain APIs.
7. Qualify the final exact SHA on hardware when a physical claim is required.

Do not reopen completed panel pin mapping, SSD1680 backend ownership, rotation, async architecture, SCD41 recovery or runtime persistence ownership without new evidence.

## Hardware boundary

Authorized growbox serial device: `/dev/cu.usbserial-120`.

Never use `/dev/cu.usbserial-10`. Do not use `/dev/cu.usbserial-1120` without explicit authorization.

Physical outputs remain disabled during unattended debugging/qualification unless the operator explicitly changes that instruction.
