# BloomML — handoff after branch consolidation (2026-11-07)

## Source and operating policy

- ONE development branch: `main` for production, offline research, tests and documentation. `agent-control` remains Local Agent's control branch; `gh-pages` remains the publishing branch.
- Merge checkpoint: `fadf6614e54264ad9f45ca19ce8699683322bb23` with original product parent `5e4db5af736e9c18d69d451496694fa6aff11743` and original research parent `73de11be282d40d03767275cc6a0891e58ed23f8`. Always fetch fresh current `main`.
- Historical research branch names in dated checkpoints are provenance, not active work instructions. Before Local Agent commands read `AGENTS.md` and `agent-control:.agent/status/daemon.json`. No Codex, Work or GitHub Actions.

## What is preserved

`main` contains Rule-authoritative ESP-IDF firmware and offline R4 physics, Kaggle GPU notebooks/runners, public dataset benchmarks, MPC devices/optimizer/lab, passive shadow DTO and tests. No runtime connection was made from MPC to hardware outputs; no v4/v5/v6 schema conversion, production training dependency or `OutputSupervisor` change.

Current research entry points: `docs/simulator/RESEARCH_STATUS.md`, `docs/simulator/MPC_ROADMAP.md`. Original Kaggle R4 work is historical DONE. MPC's older 60-minute episode PASS does not erase 19/22 FAIL challenging cases. 2026-11-04 Local Agent report gives 37/37 unit PASS on `9258ae41` while the hard-case matrix remains 19/22 FAIL. Empirical growbox calibration and physical qualification are absent.

## Single next product sprint

Design one versioned production `DeviceProfile` contract for configured sensor roles, output mappings, environmental targets, schedules and safety. Inventory the current hardcoded production I/O, ownership/persistence and validation/migration seams, then add focused tests before changing UI. Do not build a third panel.

## Open gates

Full exact-main clean-room C++/ESP-IDF build, complete current-source test suite, physical SCD41/buttons/outputs, ML hardware safety and own-growbox data are OPEN/UNKNOWN. Branch deletion and exact merge validation (if performed) require inspection of terminal Local Agent evidence in `agent-control:.agent/results/`. Do not assume task success from queue submission.
