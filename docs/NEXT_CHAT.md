# BloomML — next-chat handoff (2026-11-07)

## Read and verify first

Read `AGENTS.md`, `docs/README.md`, `docs/CURRENT_STATUS.md`, `docs/PRODUCT_DIRECTION.md`, `docs/AUDIT_STATUS.md`, `docs/ARCHITECTURE.md`, `docs/PROJECT_ROADMAP.md`; fetch fresh `main` and `agent-control:.agent/status/daemon.json`. Do not assume stored SHA is HEAD.

## What this sprint changed

- Product direction: ONE eventual operator workflow for hardware, targets, safe autonomy, monitoring. Current web v5 configurator/3D and serial v4 panel are research/developer surfaces, **not competing production admins**.
- Main panel request hardening against browser cross-origin POST, DNS rebinding for loopback-bound listener, non-JSON and oversized request bodies, with dedicated tests.
- Clarified v5 browser wording/filename and documentation around v4/v5/v6 boundaries; no schema conversion, production firmware or safety ownership was changed.
- Local test evidence and its exact snapshot limitations are in `AUDIT_STATUS.md`.

## Not done — do not call it done

- Full current-HEAD source/dependency clean-room gate, IDF build, physical SCD41/buttons/output testing.
- Unified device-profile and production setup UI (not yet implemented).
- Research MPC robustness: 19/22 adversarial scenarios FAIL; latest exact-SHA retest open; Kaggle MPC sweep not run.
- Research branch has not been merged. No Local Agent hardware tasks were queued.

## Single next closable sprint

**Production profile contract and one minimal integration plan, without implementing a second UI.**

1. Recheck fresh HEAD / Local Agent activity and review capability mapping and persistence boundary.
2. Specify one bounded versioned device profile with validation and migration tests, separate from ML feature schemas.
3. Choose the already-existing frontend components to reuse, and explicitly classify remaining demo/research panels.
4. Implement no actuator/control changes until profile tests and safety/ownership review pass.
5. Publish a tested source/documentation checkpoint with exact SHA. Preserve all historical research evidence without rerunning closed work.

Research resume instead: read `docs/simulator/RESEARCH_STATUS.md`, `docs/simulator/MPC_ROADMAP.md` and `docs/checkpoints/2026-11-04-bloomml-mpc-resume-complete.md` on the research branch; never treat that branch as production.
