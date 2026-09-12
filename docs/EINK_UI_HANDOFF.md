# E-ink UI implementation handoff

Updated: 2026-09-12
Repository: `MichalMatu/growbox-ml-controller`
Primary branch: `main`
Control branch: `agent-control`
Local Agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`
Preparation chat bridge id (provenance only): `chat-e23bf59b`; a new window must use its own `LA_CHAT` identifier if the bridge provides one.
Starting documentation HEAD: `5b985f40f60a8856a3efbdddf036e4b17d2447b1`
Last code-bearing hardware-qualified baseline: `e03763d019af405087a5fa9c6713a7165d2e623f`

## Mission

Before resuming Controller behavior quality work, add an operator-facing e-ink UI to the growbox controller so the board itself shows what the controller sees and what it is doing.

The user has explicitly confirmed that the display/button module used by `MichalMatu/esp32s3_LiteGraph` is the **same physical module and the pin map is 100% compatible**. Treat that as an operator-confirmed hardware fact. Reuse the known working implementation instead of redesigning the display stack.

The implementation should be complete, tested and flashed to the growbox board before the next roadmap task starts.

## Reference implementation to port from

Reference repository: `MichalMatu/esp32s3_LiteGraph`.

Pinned reference observed during handoff preparation: `50716ee5cc4f4c426ba19ea5f137d944db2970c3`.

Primary source area:

`lib/framework/epd2_9/clay_ui/`

Useful reusable pieces identified in the reference:

- Clay view/controller/layout infrastructure;
- `modules/menu` menu model/layout/navigation;
- physical button navigation/debounce handling used by the display UI;
- `modules/glance` for compact at-a-glance pages;
- `modules/status` for overview/status presentation patterns;
- `modules/time` for RTC/time presentation;
- `modules/common/ActionConfirmationDialog` for bounded confirmation UX where useful;
- display refresh/runtime policy and `ClayViewModeManager` behavior;
- host-side Clay UI tests under `test/host/clay_ui`;
- the existing Clay menu/display simulator and its event/navigation model.

Do not blindly import project-specific Wi-Fi/settings/application behavior from LiteGraph. Reuse the generic display, input, layout, refresh, simulator and test infrastructure, then bind it to growbox-owned data.

## Target growbox UI

Keep the first implementation useful and bounded. The default/home view should answer at a glance:

- current temperature;
- relative humidity;
- CO2;
- current time / RTC validity;
- current controller/automation state;
- lamp, exhaust/fan and humidifier intent/state in a way that preserves the existing intent/executed/transport truth distinction;
- active safety state/reason when one exists.

The button-driven menu should expose compact pages for at least:

1. **Environment** — temperature, RH, CO2 and any already-computed environmental values that are cheap and authoritative to expose.
2. **Outputs** — lamp/fan/humidifier intent, supervisor result/transport availability and safety override state; never display an unavailable physical transport as confirmed physical execution.
3. **System** — uptime, RTC, SD/storage logger, BLE/input health and firmware short SHA where already available.
4. **Diagnostics** — free internal heap, minimum free heap if available, PSRAM free/usage if enabled, and reset/panic-relevant runtime information that can be exposed cheaply.

Use the LiteGraph menu navigation and simulator patterns rather than inventing a second UI framework.

## Architecture constraints

The display is an observer/operator surface, not a controller owner.

It must not:

- execute RF433 commands directly;
- bypass `OutputSupervisor`;
- mutate safety state;
- become a second source of truth for controller configuration;
- change Rule-vs-ML authority;
- fabricate physical actuator acknowledgement;
- introduce long blocking display work into the control hot path.

Prefer a small read-only display snapshot/presenter boundary fed from already-owned runtime state. If synchronization is required, use a bounded snapshot/message seam rather than letting UI code reach through the runtime object graph.

Preserve existing frozen safety/output invariants from `docs/CURRENT_STATUS.md` and `docs/ARCHITECTURE.md`.

## E-ink behavior

Port the proven refresh policy from LiteGraph, including the existing handling intended to limit unnecessary refreshes/ghosting. Avoid refreshing the panel on every control-loop tick.

The display may update on a bounded cadence and immediately on meaningful page/navigation changes. Keep control timing independent from slow e-ink refresh operations.

Reuse the existing simulator so layout and menu navigation can be exercised without repeatedly flashing hardware.

## Autonomous work mode

This task is intended to run autonomously while the operator is away/asleep.

Before any write:

1. Read `AGENTS.md`, `docs/FRESH_CHAT_BOOTSTRAP.md`, `docs/CURRENT_STATUS.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md`, `docs/CONTINUATION_PLAN.md` and this file.
2. Fetch fresh `main` HEAD.
3. Fetch `agent-control:.agent/status/daemon.json` and do not edit the same branch while another Local Agent task is active.
4. Keep repository identity hard-bound to `MichalMatu/growbox-ml-controller`.

Every Local Agent task created by the chat must contain exactly:

```json
{
  "agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5",
  "work_branch": "main",
  "resources": []
}
```

Use direct GitHub only for bounded repository edits whose exact diff and CI can verify the result. Use Local Agent for Mac-local builds/toolchains, the LiteGraph reference checkout if already locally available, simulator execution, serial/USB, flash and physical-board observation.

**Never invoke, delegate to or launch local Codex from a Local Agent task.** ChatGPT plans; Local Agent executes deterministic commands.

Do not wait for the operator for normal engineering decisions. Make conservative bounded choices, test them, inspect evidence, fix failures and continue. Pause only for a real safety blocker, repository identity conflict, unavailable required hardware, or an ambiguity that cannot be resolved from repository/reference evidence.

## Recommended implementation sequence

1. Audit current growbox board/profile/runtime seams and record pre-change flash/RAM metrics.
2. Port the smallest generic Clay/e-ink/button/simulator substrate from LiteGraph.
3. Add a growbox-specific read-only display snapshot/presenter rather than coupling Clay directly to climate internals.
4. Implement home/environment/output/system/diagnostic pages.
5. Add/port host tests for menu navigation, page rendering state, unavailable values, safety state and output-truth semantics.
6. Run simulator tests and inspect representative screen states.
7. Run repository guards, full host C++ tests, Python tests as applicable, clang-tidy and the relevant ESP-IDF board build.
8. Compare firmware size and memory cost against the pre-change baseline.
9. Flash only the authorized growbox board and perform a bounded hardware soak with physical outputs disabled.
10. Inspect serial logs, memory behavior and reset/panic signals before declaring completion.
11. Update `CURRENT_STATUS.md`, `CHANGELOG.md`, `PROJECT_ROADMAP.md`, `CONTINUATION_PLAN.md` and this handoff with the exact final SHA and qualification result.
12. Leave `main` clean and remove any temporary development branch if one was created outside the normal Local Agent `work_branch: main` convention.

## Hardware safety for unattended work

Authorized growbox serial device: `/dev/cu.usbserial-1130`.

Never touch `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

For unattended display qualification keep real physical outputs disabled unless the operator explicitly changes that instruction. Retain safe build/runtime fences equivalent to:

- `GROWBOX_RF433_LOOPBACK_ENABLED=0`;
- `GROWBOX_STAGE28_REAL_OUTPUTS_ENABLED=0`;
- `GROWBOX_STAGE28_THERMAL_TEST_SEQUENCE_ENABLED=0`.

Real sensors/RTC/SD/BLE inputs may remain enabled so the screen shows real growbox state.

## Required log, memory and panic observation

A successful compile/flash is not enough. After flashing the exact candidate SHA, capture and inspect a meaningful boot + soak log.

Reject/fix the candidate if logs show any new:

- `Guru Meditation` / panic / abort;
- watchdog reset;
- stack overflow;
- brownout that is not already explained by the hardware setup;
- reboot loop or unexpected reset reason;
- repeated display-driver/SPI errors;
- sensor/SD/BLE regressions attributable to the display integration;
- malformed telemetry or control-loop starvation.

Record memory before and after the port, using available ESP-IDF metrics. At minimum capture internal free heap; also capture minimum-ever free heap, largest free block and PSRAM free/usage when available. Where task stack high-water marks are already accessible, inspect the display/input task and any control/runtime task materially touched by the integration.

Do not invent an arbitrary absolute RAM threshold. Compare against the pre-change baseline and require:

- enough stable headroom for the existing runtime;
- no monotonic heap loss during the soak;
- no suspicious fragmentation collapse;
- no stack margin regression that approaches exhaustion;
- no runaway display refresh allocation behavior.

If memory logging needed for this evidence does not exist, add a small bounded diagnostics hook rather than a large monitoring subsystem. The diagnostics page may reuse those same cheap metrics.

## Verification expectations

Software evidence should include, as applicable:

- pre-commit / format checks;
- existing architecture/config/output-ownership guards;
- all host C++ tests;
- relevant Python tests;
- new Clay/menu/presenter tests;
- simulator run/screens or deterministic render-state evidence;
- host clang-tidy;
- ESP-IDF build for the real growbox board/profile;
- canonical GitHub CI/Sandbox checks for the exact final commit.

Hardware evidence should include:

- exact flashed firmware SHA;
- authorized serial port identity;
- successful boot and real sensor/RTC/SD/BLE operation;
- e-ink initialization and periodic refresh without starving control;
- safe fake-locked/real-output-disabled state during unattended qualification;
- bounded soak with no panic/reset/watchdog/memory-leak evidence.

Physical button UX should be covered by simulator/host input tests. If unattended automation cannot physically press the board buttons, do not fabricate a claim that tactile interaction was human-verified; leave that as a tiny morning visual/button sanity check while still completing and flashing the implementation.

## Completion definition

The overnight implementation is complete only when:

- the e-ink UI code is committed on `main`;
- required tests/builds and canonical CI are green on the exact final SHA;
- that exact candidate is flashed to `/dev/cu.usbserial-1130` with physical outputs disabled;
- the board survives the bounded observation period without panic/watchdog/reset/memory regression;
- screen refresh logs confirm the display is active with real input data;
- documentation contains exact final evidence;
- working tree is clean;
- the next roadmap task, Controller behavior quality, has **not** been started.

If an unrecoverable blocker prevents one of these conditions, stop with exact evidence instead of claiming completion.
