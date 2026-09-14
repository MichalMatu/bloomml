# Project history

This is the compact historical record for milestones that still matter operationally. Detailed completed-phase handoffs, audit plans and temporary prompts were removed from live documentation during the 2026-09-14 repository cleanup; they remain available in Git history.

## Milestones

| Period | Milestone | Evidence / identity |
| --- | --- | --- |
| 2026-07 | Initial environment-controller / TinyML repository | `0.1.0` line in `docs/CHANGELOG.md` |
| 2026-09 | Climate-v6 native ESP-IDF runtime, deterministic Rule authority, ML shadow path | current `docs/ARCHITECTURE.md` and schema/runtime code |
| 2026-09 | Stage27 native real-input baseline and Stage28 RF433/output work | preserved in Git history and `docs/CHANGELOG.md` |
| 2026-09 | Historical full Physical H output qualification | exact executable `02208d23f403bca3540dbbd652eb55703a044833` |
| 2026-09-11 | Runtime/config/service-console architecture cleanup | code-bearing `1a599a58eb57841206ab92c7a5cacf50f7463f78` |
| 2026-09-12 | Structural cleanup closeout | code-bearing `0a7097a30280ec0f7bb408799c07093761d63e88` |
| 2026-09-12 | Release-readiness/startup-safety hardening | code-bearing `e03763d019af405087a5fa9c6713a7165d2e623f`; GitHub CI #865; Sandbox Pack #63; Local Agent task `20260912-final-main-hardware-qualification-v1` |
| 2026-09-13 | CrowPanel 2.9-inch e-ink physically operational | flashed/debug baseline `01db8228e6d822b5c64359abd8bf2d85341b5919` |
| 2026-09-13 | SCD41 regression isolated as current blocker | documentation head before cleanup `ceaaa88801568cccf2f1cbc86021ce3b2b2809d5` |

## Invariants carried forward

- Rule is authoritative in production; ML is shadow/research-only.
- `OutputSupervisor` owns normal configured physical output execution.
- One-way RF completion is not physical acknowledgement.
- Thermal trip is `>= 28 C`; recovery requires `<= 26 C` continuously for 10 minutes.
- UI/e-ink remains observer-side.
- Hardware qualification belongs to an exact executable SHA and is never automatically inherited by later commits.

For active work use `docs/CURRENT_STATUS.md`, not historical commits or deleted handoffs.
