# Fresh-context continuation plan

Updated: 2026-09-12
Repository: `MichalMatu/growbox-ml-controller`
Primary development branch after cleanup: `main`
Control branch: `agent-control`
Local Agent binding: `815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5`
Fresh-chat entrypoint: `docs/FRESH_CHAT_BOOTSTRAP.md`
Active implementation handoff: `docs/EINK_UI_HANDOFF.md`
Ready new-chat prompt: `docs/EINK_UI_NEW_CHAT_PROMPT.md`

## Resume rule

A new chat should restore context from repository evidence instead of asking the operator to repeat project history.

Read:

1. `AGENTS.md`
2. `docs/FRESH_CHAT_BOOTSTRAP.md`
3. `docs/CURRENT_STATUS.md`
4. `docs/ARCHITECTURE.md`
5. `docs/PROJECT_ROADMAP.md`
6. this file
7. `docs/EINK_UI_HANDOFF.md` while the display task is active

Historical Stage27/Stage28 handoffs and qualification documents are evidence, not the active work sequence.

Fetch fresh `main` HEAD and fresh `agent-control:.agent/status/daemon.json` before writes or Local Agent work.

## Current transition

`Stage27/28 platform qualification -> OutputSupervisor architecture -> quality refactor -> release hardening -> E-INK OPERATOR UI -> Controller behavior quality`

Latest code-bearing release-hardening identity before the display task:

`e03763d019af405087a5fa9c6713a7165d2e623f`

It passed 500 Python tests (12 hardware/visual skips), `50/50` host C++ tests, all five architecture/config ownership guards, host clang-tidy, three ESP-IDF builds, GitHub CI #865, Sandbox Pack #63 and bounded current-board qualification `20260912-final-main-hardware-qualification-v1`.

Historical full Physical H remains frozen evidence for executable `02208d23f403bca3540dbbd652eb55703a044833`. The current bounded safe real-input/fake-locked qualification is separate evidence and must not be mislabeled as full Physical H.

## What is complete

- production runtime split into bootstrap, composition, coordinator and focused support boundaries;
- service console split into domain handlers/router;
- runtime/build configuration SSOT and typed generated config;
- production V6/legacy build isolation;
- `OutputSupervisor` normal configured-output ownership;
- unavailable transport no longer fabricates successful executed truth;
- production Rule authority explicitly fenced; ML remains shadow/research-only;
- architecture guards updated to enforce the final boundaries;
- retired Gate6 thermal test helper removed from production code;
- compact post-refactor software verification PASS;
- final release-hardening/parser/startup-safety closeout PASS and documented.

## Active development goal

Before controller tuning, implement the bounded **e-ink operator UI** from `docs/EINK_UI_HANDOFF.md`.

Reuse the proven Clay/e-ink/button/simulator implementation from `MichalMatu/esp32s3_LiteGraph`; the operator has confirmed the display/button module and pin map are 100% compatible. The growbox UI is observer-side and should expose real environment, controller/output, safety, system and bounded diagnostics state without changing control ownership.

The implementation is intended to run autonomously through software verification, exact-SHA flash and bounded hardware observation. During unattended qualification keep physical outputs disabled and inspect serial logs, heap/PSRAM/fragmentation/stack evidence and panic/watchdog/reset signals before completion.

After the display task is complete and documented, resume the selected **Controller behavior quality** task: build a real-telemetry/replay baseline, quantify temperature/humidity interaction and absolute-humidity ventilation behavior, then choose exactly one tuning candidate. Define baseline metrics and acceptance criteria before implementation.

After that bounded task, continue ranking work from `docs/PROJECT_ROADMAP.md` across configuration/operator UX, logging/history/explainability, ML-shadow quality and justified device expansion.

## Work-mode policy

Use sandbox/container for analysis, replay, simulation and statistics. Use direct GitHub for bounded repository edits. Use Local Agent only for Mac-local toolchains/builds, local network, serial/USB/flash, local simulator/tooling and physical devices.

Every Local Agent task must contain exactly:

```json
{
  "agent_binding": "815cf40f-8d2a-4e1f-b7cc-c0f4e37b6cb5",
  "work_branch": "main",
  "resources": []
}
```

Never invoke local Codex from Local Agent. Check active tasks before editing the same branch.

For the e-ink task, do not wait for the operator on ordinary implementation choices: diagnose, make bounded fixes, rerun focused verification and continue. Pause only for a genuine safety/hardware/repository blocker.

## Fixed hardware/network boundaries

- Shelly: `192.168.0.16`
- Growbox serial: `/dev/cu.usbserial-1130`
- Never touch: `/dev/cu.usbserial-10`
- Do not use `/dev/cu.usbserial-1120` without separate authorization

For unattended display work keep real physical outputs disabled; real sensors/RTC/SD/BLE may remain enabled for screen data and runtime observation.

## Safety/ownership invariants

- deterministic Rule remains authoritative in production;
- ML remains shadow/research-only;
- thermal trip `>=28 C`;
- thermal recovery `<=26 C` continuously for 10 minutes;
- one-way RF completion is not physical acknowledgement;
- raw RF remains restricted to `MaintenanceLocked`;
- `OutputSupervisor` remains the only normal owner of configured physical outputs;
- the display is read-only/observer-side and cannot become another control/output owner.

## Qualification policy

Do not rerun historical qualification workflows by default. Scope verification to the actual change. Because the e-ink task materially changes the exact firmware image and adds a hardware peripheral/runtime workload, finish it with a fresh bounded exact-SHA board qualification using physical outputs disabled, plus explicit log/memory/panic observation as defined in `docs/EINK_UI_HANDOFF.md`.
