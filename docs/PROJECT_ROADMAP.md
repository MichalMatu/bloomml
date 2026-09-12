# Growbox ML project roadmap

Updated: 2026-09-12
Repository: `MichalMatu/growbox-ml-controller`
Primary development branch after cleanup: `main`
Control branch: `agent-control`
Current status: `docs/CURRENT_STATUS.md`
Active implementation handoff: `docs/EINK_UI_HANDOFF.md`

## Source-of-truth order

For fresh context, read:

1. `AGENTS.md`
2. `docs/FRESH_CHAT_BOOTSTRAP.md`
3. `docs/CURRENT_STATUS.md`
4. `docs/ARCHITECTURE.md`
5. this roadmap
6. `docs/CONTINUATION_PLAN.md`
7. `docs/EINK_UI_HANDOFF.md` while the display task is active

Stage27/Stage28 handoffs, qualification documents and older audit plans are historical evidence unless a task specifically needs them.

## Current project direction

The project is a native ESP-IDF ESP32-S3 growbox controller using real sensors, deterministic rule control, local RF433 actuators, durable telemetry and an ML shadow/research path.

The architecture-cleanup workstream is finished. Do not start another broad refactor by default. New work should improve actual growbox behavior or operator value while preserving the current ownership and safety boundaries.

Latest code-bearing release-hardening and bounded hardware qualification identity before the e-ink task: `e03763d019af405087a5fa9c6713a7165d2e623f`. It passed local guards/tests/clang-tidy/ESP-IDF builds, GitHub CI #865, Sandbox Pack #63 and `20260912-final-main-hardware-qualification-v1`.

Historical full Physical H qualification remains evidence for executable `02208d23f403bca3540dbbd652eb55703a044833`; the 2026-09-12 bounded qualification above is the current safe real-input/fake-locked evidence and must not be mislabeled as the historical full Physical H run.

## Completed platform milestones

- climate-v6 deterministic controller and schema: DONE;
- Stage27C native real-input ESP-IDF baseline: FROZEN;
- Stage28A-C RF433 protocol/transport/physical identity: DONE/FROZEN;
- absolute-humidity ventilation policy: DONE;
- Stage28E diagnostics/architecture hardening A-G: DONE;
- OutputSupervisor execution architecture A1-A13: DONE;
- historical Physical H qualification: DONE for its exact historical executable;
- quality-freeze runtime/config/service-console/legacy refactor: DONE;
- post-refactor debt burn and compact software verification: DONE;
- final release-readiness parser/startup-safety hardening: DONE.

## Active product roadmap

### Next selected bounded task

Implement **E-ink operator visibility** before controller tuning. Port the proven Clay/e-ink/button/simulator stack from `MichalMatu/esp32s3_LiteGraph` using `docs/EINK_UI_HANDOFF.md`. The operator has confirmed the physical display/button module and pin map are 100% compatible.

The first growbox UI should provide a useful home/glance view plus bounded Environment, Outputs, System and Diagnostics pages. It must remain observer-side, preserve output truth semantics, avoid control-loop blocking and finish with exact-SHA hardware observation of logs, memory/PSRAM/stack headroom and panic/watchdog/reset behavior while physical outputs remain disabled.

### 0. E-ink operator visibility

Reuse the existing Clay menu, button navigation, refresh policy, simulator and host-test patterns rather than building a second UI system. Bind them to a small read-only growbox display snapshot/presenter boundary. Show real sensor/controller/safety/system state and cheap diagnostics. Complete software verification, canonical CI, flash and bounded hardware soak before moving on.

### 1. Controller behavior quality

After the e-ink task is complete, use replay/simulation and real telemetry to improve temperature/humidity interaction, absolute-humidity ventilation decisions, targets, deadbands, hysteresis and dwell. Start by building a baseline and choosing exactly one evidence-backed tuning candidate with explicit acceptance criteria.

### 2. Configuration and operator UX

Reduce friction in configuring sensors, targets, schedules, outputs and growbox parameters. Keep validation explicit and resource cost bounded for ESP32-S3.

### 3. Logging, history and explainability

Make it easier to answer: what did the controller observe, why did it choose an action, what was actually attempted, and how did the environment respond? Prefer compact device telemetry plus offline analysis.

### 4. ML shadow/research quality

Improve feature logging, labels, replay datasets and deterministic-vs-ML comparison metrics. ML remains observe-only in production until separately justified and qualified.

### 5. Useful device expansion

Add sensors/actuators only when a concrete growbox use case justifies code, RAM, UI and verification cost. Do not accumulate integrations for their own sake.

### 6. Release/qualification discipline

Use focused software verification during normal development. Run physical qualification only for a release/candidate where a fresh hardware-qualified executable identity is actually required or when changes materially affect output/safety/hardware behavior. The e-ink task gets a bounded exact-SHA board qualification because it changes the firmware image and adds a display/input runtime workload.

## Selection rule

Rank prospective work by:

1. practical growbox value;
2. safety risk;
3. architectural ownership clarity;
4. ESP32-S3 RAM/CPU/flash cost;
5. software-verification coverage;
6. implementation size and reversibility.

Prefer one small coherent improvement over another cross-cutting rewrite.

## Repository/work-mode roadmap

- `main` is the normal development branch after cleanup;
- `agent-control` is infrastructure/control state and is not a product-development branch;
- `gh-pages` is retained for publishing;
- sandbox/container first for analysis, replay, simulation and statistics;
- direct GitHub for bounded code/config/docs edits;
- Local Agent for Mac-local builds/toolchains, local network, USB/serial/flash, local simulator/tooling and devices;
- never invoke local Codex from Local Agent.

Every Local Agent task uses the exact repository binding, `work_branch: main`, and `resources: []`.

## Hardware boundaries

Growbox port: `/dev/cu.usbserial-1130`.

Never touch `/dev/cu.usbserial-10`.

Do not use `/dev/cu.usbserial-1120` without separate authorization.

Canonical Shelly host: `192.168.0.16`.
