# Current controller status

Updated: 2026-09-12
Repository: `MichalMatu/growbox-ml-controller`
Primary development branch: `main`
Control branch: `agent-control`
Publishing branch: `gh-pages`
Fresh-chat entrypoint: `docs/FRESH_CHAT_BOOTSTRAP.md`
Product roadmap: `docs/PROJECT_ROADMAP.md`
Active implementation handoff: `docs/EINK_UI_HANDOFF.md`
Ready new-chat prompt: `docs/EINK_UI_NEW_CHAT_PROMPT.md`

## Current phase

The architecture/quality refactor, structural cleanup and final release-readiness hardening are complete. Before the previously selected Controller behavior quality task starts, one bounded operator-visibility task is now active: **port the proven Clay/e-ink/button/simulator stack from `MichalMatu/esp32s3_LiteGraph` and show real growbox state directly on the board display**.

The operator has explicitly confirmed that the growbox uses the same physical display/button module and that the pin map is 100% compatible with the LiteGraph implementation. The active execution plan, unattended work policy, verification matrix and hardware safety rules are in `docs/EINK_UI_HANDOFF.md`.

The display must remain observer-side. It may present sensor/controller/output/safety/system/diagnostics state, but it must not become a second control owner, bypass `OutputSupervisor`, weaken safety or fabricate physical actuator acknowledgement.

Final release-readiness hardening before this display task is closed on code-bearing executable `e03763d019af405087a5fa9c6713a7165d2e623f`. That exact identity passed repository guards, host/Python tests, clang-tidy, ESP-IDF builds, canonical GitHub checks and bounded hardware task `20260912-final-main-hardware-qualification-v1`. The strict 120 s `soak_v=3` run completed with zero violations, and the first valid SCD41 sample released startup fail-closed state immediately (`safety_latched=0`, `safety_reason=0`) instead of entering the historical false recovery hold.

## Structural cleanup closeout

The 2026-09-12 cleanup removed retired or unused climate code, tightened source layout and reduced header coupling without changing production behavior:

- removed retired `BleOutsideSource`, `Stage27SdDataLogger` and obsolete `Stage27Telemetry.cpp` implementation;
- removed unused `ClimateObservabilityMetrics` and its standalone test target;
- moved `LampSafety` and `OutputBindings` under `src/climate/output/`;
- reduced `RealInputRuntimeCoordinator.h` to two direct includes by moving concrete dependencies to the implementation file;
- preserved namespaces, runtime ownership and output/safety behavior.

The final structure re-audit on `0a7097a30280ec0f7bb408799c07093761d63e88` found:

- 82 climate headers and 135 internal include edges;
- zero include cycles;
- zero climate `.cpp` files without build/reference wiring;
- zero `TODO` / `FIXME` / `HACK` / `XXX` markers in the audited climate/core scope;
- no further structural refactor with a clear benefit-to-churn justification.

## Architecture state

The production real-input path is split into clear boundaries:

- `ClimateV6RealInputRuntime` — thin bootstrap/composition entry;
- `RealInputRuntimeComposition` — dependency ownership and lifetime wiring;
- `RealInputRuntimeCoordinator` — one-cycle orchestration;
- `RuntimeOutputTransport` — explicit physical-transport truth boundary;
- `RuntimeOutputTelemetryLog` — output telemetry formatting/logging;
- domain service-console handlers behind a thin router/transport layer.

`OutputSupervisor` remains the only normal production owner allowed to execute configured physical outputs.

The runtime configuration source of truth is CMake/profile based and exported to C++ through the generated typed `RuntimeBuildConfig.h` interface. Production V6 builds do not rely on duplicated fallback defaults.

The production controller is compile-time fenced to deterministic `Rule` authority. ML remains shadow/research-only unless a separate research build explicitly opts into another mode.

When physical transport is unavailable (`fake-locked`), the runtime reports `NotAttempted/Unavailable`; it must never fabricate executed or physical output truth.

For the active display task, UI code should consume a bounded read-only snapshot/presenter seam rather than reaching through runtime ownership boundaries. Slow e-ink work must not block the controller hot path.

## Frozen safety/output invariants

- deterministic rule control remains authoritative;
- ML remains shadow/research-only for production;
- lamp thermal trip remains `>= 28 C`;
- lamp recovery remains `<= 26 C` continuously for 10 minutes;
- safety remains active while normal automation is disabled;
- one-way RF completion is transport evidence, not physical acknowledgement;
- raw RF remains restricted to explicit `MaintenanceLocked` handling;
- configured physical output execution remains owned by `OutputSupervisor`;
- e-ink UI remains observer-side/read-only with respect to controller/output ownership.

## Hardware qualification status

Historical full Physical H remains valid evidence only for its exact executable identity:

`02208d23f403bca3540dbbd652eb55703a044833`

Terminal historical evidence: `20260910-output-supervisor-physical-h-v3`.

`0a7097a30280ec0f7bb408799c07093761d63e88` is the structural-cleanup baseline. Final code-bearing hardening identity `e03763d019af405087a5fa9c6713a7165d2e623f` is hardware-qualified: Local Agent task `20260912-final-main-hardware-qualification-v1` finished PASS on `/dev/cu.usbserial-1130`, with real inputs and physical outputs/RF loopback/thermal-test sequence disabled. GitHub CI #865 and Sandbox Pack #63 also passed on the same code-bearing SHA. A later documentation-only descendant does not change firmware source and does not replace the exact executable qualification identity above.

The e-ink implementation must finish with a new bounded exact-SHA board qualification because it changes the firmware image and adds display/input runtime work. During unattended qualification physical outputs remain disabled. Required evidence includes boot/soak logs plus explicit observation of heap/PSRAM/fragmentation/stack headroom and panic/watchdog/reset signals; see `docs/EINK_UI_HANDOFF.md`.

Qualified Growbox serial device: `/dev/cu.usbserial-1130`.

Never touch `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

## Repository workflow

The repository is cleaned to the expected long-lived branches:

- `main` — normal source and documentation work;
- `agent-control` — Local Agent control plane;
- `gh-pages` — publishing branch.

Local Agent tasks must use:

```json
{
  "agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5",
  "work_branch": "main",
  "resources": []
}
```

Use direct GitHub for bounded source/config/docs changes. Use Local Agent when Mac-local builds/toolchains, local simulator assets, local network, serial/USB/flash or physical devices are materially required. Never invoke local Codex from Local Agent.

## Immediate next work

Implement the **e-ink operator UI** from `docs/EINK_UI_HANDOFF.md` and use `docs/EINK_UI_NEW_CHAT_PROMPT.md` to start the dedicated autonomous implementation chat. Reuse the LiteGraph Clay menu/button/simulator/refresh infrastructure, expose real growbox environment/output/safety/system/diagnostics state, verify software and memory cost, flash the exact candidate to `/dev/cu.usbserial-1130` with physical outputs disabled, and inspect logs/memory/panic behavior before declaring completion.

Only after that task is fully closed and documented should work resume on **Controller behavior quality**: build a replay/telemetry baseline from real growbox data and identify one measurable tuning improvement in temperature/humidity interaction, absolute-humidity ventilation, targets, deadbands, hysteresis or dwell. Define baseline metrics and acceptance criteria before changing production behavior. Avoid another broad architecture rewrite unless concrete evidence exposes a new responsibility or ownership problem.
