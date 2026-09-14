# Changelog

Only milestones that still help understand the current codebase are kept here. Detailed phase-by-phase notes remain available in Git history; exact operational identities are summarized in [HISTORY.md](HISTORY.md).

## Unreleased

### SCD41/e-ink closeout — 2026-09-14

- Final code-bearing closeout commit: `c720c0a1d6d6d9c80daeb04b2dc69efa53d2c8a4`.
- Preserved the Sensirion-aligned SCD41 clean-start sequence `wake_up -> stop_periodic_measurement -> reinit -> start_periodic_measurement`.
- Reproduced the intermittent SCD41 condition on one later boot: one valid sample followed by more than 70 seconds without a new sample, while read/invalid counters stayed zero and e-ink continued refreshing. A subsequent boot of the same SHA sampled normally.
- Added a bounded runtime liveness mitigation: after 30 seconds without a new SCD41 measurement, perform the clean-start sequence at most once per MCU boot. No ESP restart and no infinite retry loop are introduced.
- Added/extended source-policy regression coverage for the clean-start order, availability semantics and one-shot delayed recovery.
- Restored the e-ink main quick-glance page to show temperature, RH, CO2, SCD41 freshness, time, mode, lamp, fan, humidifier and safety while retaining the four-page Environment / Outputs / System / Diagnostics model.
- Canonical CI now runs the five display host suites; final CI #1023 passed web, host and ESP-IDF jobs for the closeout line.
- The exact `c720...` firmware built successfully locally with e-ink enabled. A fresh physical flash/monitor gate could not run because `/dev/cu.usbserial-1130` was not enumerated; the task stopped before touching hardware.
- Historical hardware evidence remains valid for the clean-start/e-ink line: `a92074b...` passed a 90-second run with 8 SCD41 samples and 5 e-ink refreshes plus 5/5 bounded MCU-reset cycles.
- The earlier missing-board incident was a loose USB cable, not a firmware boot loop; `/dev/cu.usbserial-10` remained untouched throughout diagnosis.

### Repository consolidation — 2026-09-14

- Promoted the active e-ink/SCD41 development line to canonical `main`.
- Reduced live documentation by removing superseded phase handoffs, audit plans, temporary prompts and Stage27/28 continuation files.
- A second clean-baseline sweep removed the obsolete autonomous-session/LiteGraph migration plans and retired one-off Stage11/Stage27C workflows.
- Fixed the panel documentation contract, removed a dead architecture link and moved exact verification identities out of the architecture contract.
- Hardware audit helpers now require an explicit serial port instead of guessing a device, and serial-capture help no longer names a forbidden port.
- Added `HISTORY.md` as the compact milestone/evidence index.
- Reduced `README.md`, `AGENTS.md`, `CURRENT_STATUS.md`, `PROJECT_ROADMAP.md`, `IO_MAP.md`, hardware bring-up docs and frontend docs to current responsibilities.
- Long-lived branch policy is `main` + `agent-control` + `gh-pages`; temporary implementation branches are not part of the canonical development line.
- Cleanup intentionally does not change controller behavior.

### CrowPanel e-ink integration / SCD41 handoff — 2026-09-13

- Added observer-only CrowPanel 2.9-inch SSD1680 e-paper runtime integration.
- Added compact status rendering, rotation, asynchronous display worker and partial-refresh support.
- Physical display operation was confirmed on the board; known flashed/debug baseline: `01db8228e6d822b5c64359abd8bf2d85341b5919`.
- Historical regression evidence showed SCD41 available but producing zero successful samples while BLE/RTC/SD remained healthy.
- Pre-cleanup branch/documentation head: `ceaaa88801568cccf2f1cbc86021ce3b2b2809d5`.

### Release-readiness and structural hardening — 2026-09-11 to 2026-09-12

- Split real-input runtime, service-console and output ownership into narrower production boundaries.
- Centralized board/runtime configuration and retained Rule-authoritative production behavior with ML shadow/research only.
- Removed retired BLE/storage/telemetry code and stale observability helpers.
- Preserved `OutputSupervisor` as the only normal configured physical-output execution owner.
- Hardened startup/fake-output/thermal semantics and current soak parsing.
- Structural cleanup identity: `0a7097a30280ec0f7bb408799c07093761d63e88`.
- Release-readiness identity: `e03763d019af405087a5fa9c6713a7165d2e623f`; GitHub CI #865, Sandbox Pack #63 and bounded board qualification passed for that exact executable line.

### Climate-v6 / native ESP-IDF platform

- Added Rule / ML_SHADOW / ML_ACTIVE runtime modes with Rule as default authority and deterministic safety final.
- Added climate-v6 runtime, trace/replay/counterfactual evaluation and generated C inference.
- Migrated standalone firmware to native ESP-IDF 5.5.4.
- Added real-input adapters, RF433/output execution architecture, diagnostics, host/HIL tests and scientific simulator/twin tooling.
- Historical full Physical H qualification belongs only to exact executable `02208d23f403bca3540dbbd652eb55703a044833`.

## 0.1.0 — 2026-07-11

- Bootstrapped the schema-driven portable environment-controller library.
- Added deterministic simulation/training/export pipeline and ESP32-S3 demonstration firmware.
- Added serial replay, CI, scenarios, host tests and firmware builds.
