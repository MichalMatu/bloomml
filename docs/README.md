# Documentation map

This directory is intentionally split into current product documentation, operational hardware references, research/legacy material and compact history. Start here instead of reading every file.

## Read first

For normal product work read, in order:

1. `../AGENTS.md` — repository/work-mode and safety rules.
2. `CURRENT_STATUS.md` — what is finished, what is active, and what must not be reopened without new evidence.
3. `ARCHITECTURE.md` — ownership and runtime boundaries.
4. `PROJECT_ROADMAP.md` — current order of work.
5. The subsystem document for the change. For the current display stage that is `DISPLAY_UI_PORT.md`.

`PROJECT_LAYOUT.md` is the repository map. `IO_MAP.md` is the compact I/O/runtime map.

## Current product documents

| Document | Role |
| --- | --- |
| `CURRENT_STATUS.md` | active baseline and next work |
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

**Current research continuation (2026-11-03):** [BloomML Kaggle T4×2 checkpoint and R0–R8 roadmap](checkpoints/2026-11-03-kaggle-bloomml-compute-handoff.md), [Kaggle setup](simulator/KAGGLE_EXECUTION.md) and [thermodynamic model checkpoint](checkpoints/2026-11-03-thermodynamics-convection-fidelity.md) belong to the separate research branch. The Kaggle notebook is notebooks/BloomML_Kaggle_T4x2.ipynb; do not confuse this work with the current production climate-v6 runtime.


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
