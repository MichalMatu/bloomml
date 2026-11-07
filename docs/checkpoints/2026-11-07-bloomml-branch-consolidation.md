# BloomML — branch consolidation checkpoint (2026-11-07)

## Git preservation

- Product head before merge: `5e4db5af736e9c18d69d451496694fa6aff11743`.
- Experimental research head before merge: `73de11be282d40d03767275cc6a0891e58ed23f8`.
- Two-parent merge commit: `fadf6614e54264ad9f45ca19ce8699683322bb23` — preserves entire 110-commit research history as well as original main. No force-push, rebase or squash.
- Source tree audit: 75 research paths differed from the common base; 72 unconflicted paths copied with identical Git blob SHAs. Three colliding files (`docs/CURRENT_STATUS.md`, `docs/PROJECT_ROADMAP.md`, `docs/README.md`) were reconciled to preserve active product instructions and research links.
- Production runtime, Rule, OutputSupervisor and frontend changes from previous main were not overwritten.

## Repository policy

New source and offline research work directly on `main`. `agent-control` continues to be required by Local Agent, `gh-pages` remains the publication branch. Old research checkpoints are immutable historical evidence and may mention now-retired branch names.

## Safety and test status

Offline inclusion of the R4 physics and MPC solver does NOT make MPC fit for physical actuation. Historical matrix **19/22 MPC FAIL**. Exact historical test log `agent-control:.agent/results/la-bloomml-mpc-safety-regression-20261104-v1.json`: 37 unit tests PASS on `9258ae41`, and same hard-case matrix 19/22 FAIL. This is not a full current-main qualification. No firmware/physical E2E performed as part of the Git merge.

Post-merge branch removal requires explicit remote ancestry/content checks and successful focused regression. The Local Agent task evidence must be read before claiming remote deletion.

See `../NEXT_CHAT.md`, `../PRODUCT_DIRECTION.md`, `../AUDIT_STATUS.md` and `../simulator/RESEARCH_STATUS.md`.
