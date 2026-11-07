# BloomML — documentation map

**Resume point (2026-11-07): connect authorized CrowPanel on `/dev/cu.usbserial-120` and prove physical output isolation.** Until this is confirmed, hardware qualification and IPO v2 activation API are blocked. Start with [NEXT_CHAT.md](NEXT_CHAT.md) and the [final checkpoint](checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md).

Current product docs are authoritative; dated checkpoints preserve implementation evidence. The latest [repository repair checkpoint](checkpoints/2026-11-07-bloomml-general-repository-repair.md) records exact-source web/panel gates and the physical gate still blocked. Historical plans do not reopen closed work.

## Read first

For normal product work read, in order:

1. `../AGENTS.md` — repository/work-mode and safety rules.
2. `CURRENT_STATUS.md` — what is finished, what is active, and what must not be reopened without new evidence.
3. `CONFIGURATION_FLOW.md` — one canonical sensor/parameter/control/output matrix and DeviceProfile v2 draft/driver graph state.
4. `PRODUCT_DIRECTION.md` — what the operator product is and which panels are only research/developer tools.
5. `ARCHITECTURE.md` — ownership and runtime boundaries.
6. `PROJECT_ROADMAP.md` — current order of work.
7. The subsystem document for the change. For the current display stage that is `DISPLAY_UI_PORT.md`.

`PROJECT_LAYOUT.md` is the repository map. `IO_MAP.md` is the compact I/O/runtime map.

## Current product documents

| Document | Role |
| --- | --- |
| `CURRENT_STATUS.md` | current implemented/tested/blocked truth and next hardware action |
| `CONFIGURATION_FLOW.md` | canonical current IPO contract, runtime owners and activation gate (not phase history) |
| `PRODUCT_DIRECTION.md` | canonical product UX/contract direction and ML qualification gates |
| `AUDIT_STATUS.md` | scoped audit results, evidence limits and open full-audit gates |
| `checkpoints/2026-11-07-bloomml-general-repository-repair.md` | new source audit, repairs, test evidence and next-chat boundary |
| `NEXT_CHAT.md` | exact resume procedure: hardware connection/isolation first |
| `checkpoints/2026-11-07-bloomml-documentation-freeze-handoff.md` | final consolidated checkpoint and restart dependency |
| `checkpoints/2026-11-07-bloomml-ipo-v2-hardware-qualification-gate.md` | new CrowPanel build and NVS adapter fault-matrix evidence; authorized device absent |
| `checkpoints/2026-11-07-bloomml-ipo-v2-activation-transaction.md` | qualified host-only transaction owner, explicit closed hardware gate and remaining device gates |
| `checkpoints/2026-11-07-bloomml-ipo-quality-gate-b5.md` | prior B5 change record, now host-qualified at the later transaction checkpoint |
| `ARCHITECTURE.md` | stable ownership/safety architecture |
| `PROJECT_ROADMAP.md` | ordered product priorities |
| `DISPLAY_UI_PORT.md` | live plan/contract for Clay/menu/button/simulator work |
| `PROJECT_LAYOUT.md` | repository structure and where new code belongs |
| `IO_MAP.md` | compact production I/O and truth boundaries |
| `SANDBOX_EXECUTION_FLOW.md` | repository-only execution workflow |

## Hardware/operations references

These are current reference documents, not active roadmaps:

- `RF433_DEVICE_CODES.md`
- `SHELLY_POWER_FEEDBACK.md`
- `ESP32_S3_SERIAL_PORT_RESET.md`
- `HARDWARE_BRINGUP_CHECKLIST.md`

Hardware evidence belongs to exact executable SHAs. A software build or simulator result is never physical qualification.

## Research and legacy-contract documentation

The repository contains two different ML/data-contract generations. Do not mix them:

- production controller: `schemas/environment-controller.v6.json`, 44 climate features / 6 climate outputs, Rule-authoritative runtime;
- older broad growbox simulator/tooling contract: `schemas/environment-controller.json` (v4), 128 features / 15 outputs including pots.

The following documents primarily describe the research/tooling side and are not production runtime contracts:

- `DATA_CONTRACT.md`
- `CONFIG_MATRIX.md` / `CONFIG_MATRIX.csv`
- `MODEL_PIPELINE.md`
- `ML_DECISION_REPORT.md` / `.json`
- `simulator/*`

They remain because tools and research workflows still use them. They must not be used to infer current firmware ownership, active output authority or the current product roadmap.

## Integrated research tooling (separate from production authority)

The former research branch content is now present on `main`; the research work remains logically isolated, not production-qualified. Current MPC entry point: [research status](simulator/RESEARCH_STATUS.md), [MPC roadmap](simulator/MPC_ROADMAP.md), [Kaggle execution](simulator/KAGGLE_EXECUTION.md), [archived 2026-11-04 checkpoint](checkpoints/2026-11-04-bloomml-mpc-resume-complete.md). Historical branch names inside dated checkpoints are provenance only.

## History

- `HISTORY.md` — compact milestone/evidence index.
- `CHANGELOG.md` — concise change history.

Detailed retired handoffs, phase plans and superseded audits stay in Git history instead of live docs.

## Documentation rules

- `CURRENT_STATUS.md` must describe the current phase, not a completed blocker.
- `PROJECT_ROADMAP.md` must not reopen work already marked closed in `CURRENT_STATUS.md`.
- architecture docs describe stable contracts; exact transient CI/task identities belong in status/history.
- research docs must say which contract generation they describe.
- vendor/reference code is never automatically production code.
- when a subsystem gets a dedicated live document, keep implementation detail there instead of duplicating it across README/status/roadmap.
