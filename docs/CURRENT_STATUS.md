# Current controller status

Updated: 2026-09-14
Repository: `MichalMatu/growbox-ml-controller`
Canonical source branch: `main`
Control branch: `agent-control`

## Current phase

The SCD41/e-ink recovery phase is closed for normal development. The active next stage is the bounded display UI port: Clay layout, host simulator, menu/navigation and native buttons, while preserving the existing native SSD1680 backend and observer-only architecture.

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
- slow display work stays outside the 1-second control hot path.

## Current e-ink baseline

The CrowPanel 2.9-inch SSD1680 display path is operational.

Keep:

- 296x128 monochrome native framebuffer/backend;
- native ESP-IDF SPI/GPIO ownership;
- 180-degree rotation already established;
- asynchronous display worker;
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

## Next work

1. Create the isolated C++20 Clay component and exact 296x128 host simulator without changing firmware behavior.
2. Reproduce the existing four growbox pages with host tests/golden checks.
3. Attach Clay behind the existing async display transaction.
4. Add true dirty-region RAM-window transfer.
5. Add native ESP-IDF button input with host-tested debounce/long-press behavior.
6. Add only justified growbox menu/settings flows through existing domain APIs.
7. Qualify the final exact SHA on hardware when a physical claim is required.

Do not reopen completed panel pin mapping, SSD1680 backend ownership, rotation, async architecture or the discarded display-brownout hypothesis without new evidence.

## Hardware boundary

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never use `/dev/cu.usbserial-10`. Do not use `/dev/cu.usbserial-1120` without explicit authorization.

Physical outputs remain disabled during unattended debugging/qualification unless the operator explicitly changes that instruction.
