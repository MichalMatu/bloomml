# Current controller status

Updated: 2026-09-19
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`

## Current phase

The SCD41/e-ink recovery phase is closed for normal development. The active next stage remains the bounded display UI port: Clay layout, host simulator, menu/navigation and native buttons, while preserving the existing native SSD1680 backend and observer-only architecture.

Before starting that feature stage, the runtime reliability audit closed three cross-cutting issues: global NVS ownership is now centralized before BLE/output persistence startup; failed output-persistence writes remain retryable with bounded runtime backoff; and the e-ink worker uses a statically allocated FreeRTOS task with stack high-water instrumentation. Host-test build parallelism is also bounded by default to avoid memory-dependent gate failures.

The documentation/vendor re-audit was performed from `main` baseline `6d083e5b00a29b20a8a9bb6f2bb83a395634aff5`.

The curated donor snapshot is under `vendor/litegraph_epd_port/`, pinned to `esp32s3_LiteGraph@5b8c758c365547ddeaab65bbe9f849bdd071695d`. It is reference code only and is not part of any build.

The live implementation contract for the next stage is `docs/DISPLAY_UI_PORT.md`.

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

The current partial-refresh path uses partial waveform handling but still transfers the full framebuffer. True reduced dirty-window transfer is future optimization work and is explicitly staged in `DISPLAY_UI_PORT.md`.

## SCD41 state

The production line retains the Sensirion-aligned clean-start sequence:

`wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`

and one bounded liveness recovery after 30 seconds without a new measurement, at most once per MCU boot. Do not restart broad SCD41 diagnosis unless new evidence shows a regression.

Historical exact-SHA physical evidence and the intermittent-failure reproduction are preserved in `HISTORY.md` / `CHANGELOG.md` and Git history.

## Current software boundary for Clay

Current `src` compiles as C++17. The pinned Clay 0.14 header requires C++20.

Preferred implementation is an isolated C++20 `growbox_clay_ui` component with a plain growbox-owned API; `Clay_*` types must not escape into the normal C++17 runtime. The rest of the firmware should not be upgraded to C++20 merely to make the first Clay port easier.

## Verification state

The runtime-hardening branch has passed the focused persistence regression, the complete host C++ suite with bounded build parallelism, display host suites and ESP-IDF production build. `git diff --check` and the explicit NVS ownership guard are clean.

`make check-fast` was not executed successfully in the Local Agent workspace because that workspace does not currently contain the repository `.venv`; this is an environment limitation, not a recorded source/test failure.

The latest GitHub Actions run for functional code HEAD `b73aebc12f94261002bc1c297ab30d482a4198ec` is run `35404412014`. `web-tests` and `esp-idf-build` passed. `host-tests` stopped at the pre-commit gate because the `clang-format` hook modified files; later host steps were therefore skipped. Treat this as the current formatting gate, not as a firmware/runtime test regression.

## Chat handoff — 2026-09-19 01:30 CEST

This section is the live restart point for the next ChatGPT window. Replace it at the next handoff instead of accumulating retired session notes.

Exact repository binding:

- repository: `MichalMatu/growbox-ml-controller`;
- Local Agent repository id: `growbox-ml-controller`;
- Local Agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`;
- control branch: `agent-control`;
- current working branch: `agent/nvs-persistence-hardening`;
- canonical `main` HEAD at handoff: `fff9d3a8c024ca19dcf6fc39619c2b7559a546ac`;
- functional working-branch HEAD before this docs-only handoff commit: `b73aebc12f94261002bc1c297ab30d482a4198ec`;
- at that point the working branch was 28 commits ahead of `main` and 0 behind;
- the Local Agent daemon was `idle` with no active task.

The reliability work on the working branch includes NVS ownership centralization, retryable output persistence with bounded backoff, fallback snapshot repair, static e-ink worker allocation/high-water diagnostics, bounded host-build parallelism, PSRAM/profile clarification and the accompanying host regression tests/documentation.

Important terminal Local Agent evidence:

- `20260919-runtime-hardening-final-verify-v1` — terminal `done`; focused persistence tests, all 50 portable host tests, display host suites and ESP-IDF production build passed on the then-current hardening branch;
- `20260919-persistence-fallback-repair-verify-v1` — terminal `done`; `git diff --check`, persistence coordinator/store regression and final-clean checks all passed;
- do not claim a Local Agent task succeeded unless its exact `.agent/results/<task-id>.json` is terminal and read.

Immediate continuation order for a new chat:

1. Read `AGENTS.md`, `docs/README.md`, this file, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md` and the relevant subsystem doc before changing behavior.
2. Fetch fresh `agent/nvs-persistence-hardening`, fresh `main` and `agent-control:.agent/status/daemon.json`; do not assume the SHAs above are still current.
3. Keep working on `agent/nvs-persistence-hardening` until its quality gate is green; do not restart the hardening work from `main` and do not race direct writes with an active Local Agent task.
4. First close the GitHub CI formatting gate: reproduce `pre-commit run clang-format --all-files` (or the equivalent focused hook), inspect that the resulting diff is formatting-only, commit it on the working branch, then rerun the relevant local gates and GitHub CI.
5. Preserve the already-green functional evidence. Do not redesign NVS/persistence/display-worker ownership unless the formatter or a subsequent test exposes a real regression.
6. Once the hardening branch is fully green, review/integrate it into `main` before beginning the Clay feature stage unless the operator explicitly chooses a different sequencing.
7. After integration, continue the first incomplete phase in `docs/DISPLAY_UI_PORT.md`: isolated C++20 Clay component plus exact 296x128 host simulator, with no firmware behavior change.

## Next work

0. Close the working-branch `clang-format` CI gate and get the complete GitHub Actions run green.
1. Integrate the verified runtime-hardening branch into `main` once the complete gate is green.
2. Create the isolated C++20 Clay component and exact 296x128 host simulator without changing firmware behavior.
3. Reproduce the existing four growbox pages with host tests/golden checks.
4. Attach Clay behind the existing async display transaction.
5. Add true dirty-region RAM-window transfer.
6. Add native ESP-IDF button input with host-tested debounce/long-press behavior.
7. Add only justified growbox menu/settings flows through existing domain APIs.
8. Qualify the final exact SHA on hardware when a physical claim is required.

Do not reopen completed panel pin mapping, SSD1680 backend ownership, rotation, async architecture or the discarded display-brownout hypothesis without new evidence.

## Hardware boundary

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`. Do not use `/dev/cu.usbserial-1120` without explicit authorization.

Physical outputs remain disabled during unattended debugging/qualification unless the operator explicitly changes that instruction.
