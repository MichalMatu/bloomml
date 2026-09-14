# Project history

Compact record of milestones that still matter operationally. Detailed retired plans/audits remain in Git history.

## Milestones

| Period | Milestone | Evidence / identity |
| --- | --- | --- |
| 2026-07 | Initial environment-controller / TinyML repository | `0.1.0` line in `CHANGELOG.md` |
| 2026-08 | Climate-v6 ML research Stages 12-16; Rule retained as authority | `ML_DECISION_REPORT.md` |
| 2026-09 | Climate-v6 native ESP-IDF runtime, Rule authority, ML shadow path | current architecture/runtime code |
| 2026-09 | Historical full Physical H output qualification | exact executable `02208d23f403bca3540dbbd652eb55703a044833` |
| 2026-09-11 | Runtime/config/service-console architecture cleanup | `1a599a58eb57841206ab92c7a5cacf50f7463f78` |
| 2026-09-12 | Structural cleanup closeout | `0a7097a30280ec0f7bb408799c07093761d63e88` |
| 2026-09-12 | Release-readiness/startup-safety hardening | `e03763d019af405087a5fa9c6713a7165d2e623f`; CI #865; Sandbox Pack #63 |
| 2026-09-13 | CrowPanel 2.9-inch e-ink physically operational | flashed/debug baseline `01db8228e6d822b5c64359abd8bf2d85341b5919` |
| 2026-09-14 | SCD41 clean-start + e-ink hardware baseline | `a92074b74b055c58c0949c7c38f4896638ebf227`; 90 s + 5/5 MCU-reset cycles |
| 2026-09-14 | Bounded SCD41 liveness recovery closeout | code-bearing `c720c0a1d6d6d9c80daeb04b2dc69efa53d2c8a4` |
| 2026-09-14 | Four-page Environment/Outputs/System/Diagnostics operator UI established | current `src/climate/display` line |
| 2026-09-14 | LiteGraph Clay/EPD donor snapshot curated for later port | growbox vendor line through `6d083e5b00a29b20a8a9bb6f2bb83a395634aff5`; donor `5b8c758c365547ddeaab65bbe9f849bdd071695d` |

## Invariants carried forward

- Rule is authoritative in production; ML is shadow/research-only.
- `OutputSupervisor` owns normal configured physical output execution.
- One-way RF completion is not physical acknowledgement.
- Thermal trip is `>= 28 C`; recovery requires `<= 26 C` continuously for 10 minutes.
- UI/e-ink remains observer-side.
- Hardware qualification belongs to an exact executable SHA and is never inherited automatically by later commits.

## Branch policy

Long-lived branches are `main`, `agent-control` and `gh-pages`. Temporary implementation branches are disposable after verified integration.

For active work use `CURRENT_STATUS.md`, not historical commits or retired handoffs.
